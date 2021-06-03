//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ClampIndirectIndices.h: Add clamp to the indirect indices used on arrays.
//

#include "compiler/translator/tree_ops/ClampIndirectIndices.h"

#include "compiler/translator/StaticType.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{
namespace
{
// Traverser that finds EOpIndexIndirect nodes and applies a clamp to their right-hand side
// expression.
class ClampIndirectIndicesTraverser : public TIntermTraverser
{
  public:
    ClampIndirectIndicesTraverser(TCompiler *compiler, TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable), mCompiler(compiler)
    {}

    bool visitBinary(Visit visit, TIntermBinary *node) override
    {
        ASSERT(visit == PreVisit);

        // Only interested in EOpIndexIndirect nodes.
        if (node->getOp() != EOpIndexIndirect)
        {
            return true;
        }

        // Apply the transformation to the left and right nodes
        bool valid = ClampIndirectIndices(mCompiler, node->getLeft(), mSymbolTable);
        ASSERT(valid);
        valid = ClampIndirectIndices(mCompiler, node->getRight(), mSymbolTable);
        ASSERT(valid);

        // Generate clamp(right, 0, N), where N is the size of the array being indexed minus 1.  If
        // the array is runtime-sized, the length() method is called on it.
        const TType &leftType      = node->getLeft()->getType();
        const TType &rightType     = node->getRight()->getType();
        TIntermConstantUnion *zero = CreateIndexNode(0);
        TIntermTyped *max;

        if (leftType.isUnsizedArray())
        {
            max = new TIntermUnary(EOpArrayLength, node->getLeft(), nullptr);
            max = new TIntermBinary(EOpSub, max, CreateIndexNode(1));
        }
        else if (leftType.isArray())
        {
            max = CreateIndexNode(static_cast<int>(leftType.getOutermostArraySize()) - 1);
        }
        else
        {
            ASSERT(leftType.isVector() || leftType.isMatrix());
            max = CreateIndexNode(leftType.getNominalSize() - 1);
        }

        TIntermTyped *index = node->getRight();
        // If the index node is not an int (i.e. it's a uint), cast it.
        if (rightType.getBasicType() != EbtInt)
        {
            TIntermSequence constructorArgs = {index};
            index = TIntermAggregate::CreateConstructor(*StaticType::GetBasic<EbtInt>(),
                                                        &constructorArgs);
        }

        // min(gl_PointSize, maxPointSize)
        TIntermSequence args;
        args.push_back(index);
        args.push_back(zero);
        args.push_back(max);
        TIntermTyped *clamped = CreateBuiltInFunctionCallNode("clamp", &args, *mSymbolTable, 300);

        // Replace the right node (the index) with the clamped result.
        queueReplacementWithParent(node, node->getRight(), clamped, OriginalNode::IS_DROPPED);

        // Don't recurse as left and right nodes are already processed.
        return false;
    }

  private:
    TCompiler *mCompiler;
};
}  // anonymous namespace

bool ClampIndirectIndices(TCompiler *compiler, TIntermNode *root, TSymbolTable *symbolTable)
{
    ClampIndirectIndicesTraverser traverser(compiler, symbolTable);
    root->traverse(&traverser);
    return traverser.updateTree(compiler, root);
}

}  // namespace sh
