//
// Copyright 2025 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RewriteMultielemntSwizzleAssignment: WGSL does not support assignment to multielement swizzles.
// This splits them into mutliple assignments to single-element swizzles.
//

#include "compiler/translator/tree_ops/wgsl/RewriteMultielementSwizzleAssignment.h"
#include "common/log_utils.h"
#include "compiler/translator/BaseTypes.h"
#include "compiler/translator/IntermNode.h"

#include "compiler/translator/Operator_autogen.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/DriverUniform.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{
namespace
{
bool IsMultielementSwizzleAssignment(TOperator op, TIntermTyped *assignedNode)
{

    if (!IsAssignment(op))
    {
        return false;
    }

    TIntermSwizzle *leftSwizzleNode = assignedNode->getAsSwizzleNode();
    if (!leftSwizzleNode || leftSwizzleNode->getSwizzleOffsets().size() <= 1)
    {
        return false;
    }

    return true;
}

// Splits multielement swizzles into single-element swizzles.
class MultielementSwizzleAssignmentTraverser : public TIntermTraverser
{
  public:
    MultielementSwizzleAssignmentTraverser(TCompiler *compiler)
        : TIntermTraverser(true, false, false), mCompiler(compiler)
    {}

    bool visitBinary(Visit visit, TIntermBinary *node) override;
    bool visitUnary(Visit visit, TIntermUnary *node) override;

  private:
    TCompiler *mCompiler;
};

bool MultielementSwizzleAssignmentTraverser::visitUnary(Visit vist, TIntermUnary *unaryNode)
{
    if (!IsMultielementSwizzleAssignment(unaryNode->getOp(), unaryNode->getOperand()))
    {
        return true;
    }

    // TODO(anglebug.com/42267100): increments and decrements should be handled by a pre-pass that
    // converts them into additions of component-wise vectors filled with 1s.
    // Then this can be converted into UNREACHABLE().
    switch (unaryNode->getOp())
    {
        case EOpPostIncrement:
        case EOpPostDecrement:
        case EOpPreIncrement:
        case EOpPreDecrement:
            UNIMPLEMENTED();
            break;
        default:
            break;
    }
    return true;
}

bool MultielementSwizzleAssignmentTraverser::visitBinary(Visit visit, TIntermBinary *parentBinNode)
{
    ASSERT(visit == Visit::PreVisit);

    if (!IsMultielementSwizzleAssignment(parentBinNode->getOp(), parentBinNode->getLeft()))
    {
        return true;
    }

    if (!getParentNode()->getAsBlock())
    {
        // TODO(anglebug.com/42267100): Cannot yet handle assignments to swizzles that are nested
        // inside other expressions.
        UNIMPLEMENTED();
        return false;
    }

    TIntermSequence insertionsBefore;

    TIntermSwizzle *leftSwizzleNode = parentBinNode->getLeft()->getAsSwizzleNode();

    // TODO(anglebug.com/42267100): The deepCopied node below would need to be traversed, but
    // this doesn't yet handle assignments to swizzles that are nested in other expressions anyway.

    // Store the RHS in a temp in case of side effects, since it will be duplicated.
    TIntermDeclaration *rhsTempDeclaration;
    TVariable *rhsTempVariable =
        DeclareTempVariable(&mCompiler->getSymbolTable(), parentBinNode->getRight()->deepCopy(),
                            EvqTemporary, &rhsTempDeclaration);
    insertionsBefore.push_back(rhsTempDeclaration);

    TIntermSequence singleElementSwizzleAssignments;
    for (uint32_t i = 0; i < leftSwizzleNode->getSwizzleOffsets().size(); i++)
    {
        // TODO(anglebug.com/42267100): Doing a deepCopy of the swizzle operand can cause problems
        // if it has side effects. E.g.
        //
        // x[i++].xy = vec(0.0, 0.0);
        //
        // will be transformed to something like
        //
        // x[i++].x = vec(0.0, 0.0).x;
        // x[i++].y = vec(0.0, 0.0).y;
        //
        // Which obviously increments `i` an extra time.
        if (leftSwizzleNode->getOperand()->hasSideEffects())
        {
            UNIMPLEMENTED();
        }
        TIntermTyped *swizzleOperandCopy = leftSwizzleNode->getOperand()->deepCopy();
        TIntermSwizzle *leftSideNewSwizzle =
            new TIntermSwizzle(swizzleOperandCopy, {leftSwizzleNode->getSwizzleOffsets()[i]});
        // The resulting swizzle doesn't need folding here because it would've already been folded,
        // this just turned it from a multielement into a single element swizzle.

        // Most binary assignments are component-wise and so we just swizzle the right side to
        // select one component for this particular single-element swizzle assignment. The only
        // special case is multiplication by a matrix, which is not component-wise obviously.
        TIntermTyped *newRightSide = CreateTempSymbolNode(rhsTempVariable);
        TOperator newOp            = parentBinNode->getOp();

        if (newRightSide->getType().isMatrix() || newRightSide->getType().isVector())
        {
            if (newRightSide->getType().isMatrix())
            {
                ASSERT(parentBinNode->getOp() == EOpVectorTimesMatrixAssign);

                // The `vec.xy *= mat` is being converted to something like `vec.x = (vec.xy *
                // mat).x; vec.y = (vec.xy * mat).y;`
                newOp = EOpAssign;

                // TODO(anglebug.com/42267100): this matrix multiplication could be kept in a temp
                // var.
                // TODO(anglebug.com/42267100): as above this does not work if the deepCopied AST
                // subtrees have side effects.
                newRightSide = new TIntermBinary(EOpVectorTimesMatrix, leftSwizzleNode->deepCopy(),
                                                 newRightSide);
            }

            ASSERT(newRightSide->getType().isVector());

            // Now swizzle to select the i-th element.
            newRightSide = new TIntermSwizzle(newRightSide, {i});
            // No need to fold newRightSide because we are swizzling either a temporary or the
            // binary matrix multiplication, so it won't be a swizzle.
        }
        else if (newRightSide->getType().isScalar())
        {
            // Needs no special handling, and can just be unmodified as the right hand side.
        }
        else
        {
            UNREACHABLE();
        }

        // At this point the right hand side should match the type of the left hand side.
        ASSERT(newRightSide->isScalar());
        ASSERT(newRightSide->getBasicType() == leftSideNewSwizzle->getBasicType());

        singleElementSwizzleAssignments.emplace_back(
            new TIntermBinary(newOp, leftSideNewSwizzle, newRightSide));
    }

    mMultiReplacements.emplace_back(getParentNode()->getAsBlock(), parentBinNode,
                                    std::move(singleElementSwizzleAssignments));
    insertStatementsInParentBlock(insertionsBefore);

    // Already traversed the left and right sides above.
    return false;
}
}  // anonymous namespace

bool RewriteMultielementSwizzleAssignment(TCompiler *compiler, TIntermBlock *root)
{
    MultielementSwizzleAssignmentTraverser traverser(compiler);
    root->traverse(&traverser);
    return traverser.updateTree(compiler, root);
}
}  // namespace sh
