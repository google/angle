//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RewriteExpressionsWithShaderStorageBlock rewrites the expressions that contain shader storage
// block calls into several simple ones that can be easily handled in the HLSL translator. After the
// AST pass, all ssbo related blocks will be like below:
//     ssbo_access_chain = ssbo_access_chain;
//     ssbo_access_chain = expr_no_ssbo;
//     lvalue_no_ssbo    = ssbo_access_chain;
//

#include "compiler/translator/tree_ops/RewriteExpressionsWithShaderStorageBlock.h"

#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/util.h"

namespace sh
{
namespace
{
bool IsCompoundAssignment(TOperator op)
{
    switch (op)
    {
        case EOpAddAssign:
        case EOpSubAssign:
        case EOpMulAssign:
        case EOpVectorTimesMatrixAssign:
        case EOpVectorTimesScalarAssign:
        case EOpMatrixTimesScalarAssign:
        case EOpMatrixTimesMatrixAssign:
        case EOpDivAssign:
        case EOpIModAssign:
        case EOpBitShiftLeftAssign:
        case EOpBitShiftRightAssign:
        case EOpBitwiseAndAssign:
        case EOpBitwiseXorAssign:
        case EOpBitwiseOrAssign:
            return true;
        default:
            return false;
    }
}

// EOpIndexDirect, EOpIndexIndirect, EOpIndexDirectStruct, EOpIndexDirectInterfaceBlock belong to
// operators in SSBO access chain.
bool IsReadonlyBinaryOperatorNotInSSBOAccessChain(TOperator op)
{
    switch (op)
    {
        case EOpComma:
        case EOpAdd:
        case EOpSub:
        case EOpMul:
        case EOpDiv:
        case EOpIMod:
        case EOpBitShiftLeft:
        case EOpBitShiftRight:
        case EOpBitwiseAnd:
        case EOpBitwiseXor:
        case EOpBitwiseOr:
        case EOpEqual:
        case EOpNotEqual:
        case EOpLessThan:
        case EOpGreaterThan:
        case EOpLessThanEqual:
        case EOpGreaterThanEqual:
        case EOpVectorTimesScalar:
        case EOpMatrixTimesScalar:
        case EOpVectorTimesMatrix:
        case EOpMatrixTimesVector:
        case EOpMatrixTimesMatrix:
        case EOpLogicalOr:
        case EOpLogicalXor:
        case EOpLogicalAnd:
            return true;
        default:
            return false;
    }
}

class RewriteExpressionsWithShaderStorageBlockTraverser : public TIntermTraverser
{
  public:
    RewriteExpressionsWithShaderStorageBlockTraverser(TSymbolTable *symbolTable);
    void nextIteration();
    bool foundSSBO() const { return mFoundSSBO; }

  private:
    bool visitBinary(Visit, TIntermBinary *node) override;
    TIntermSymbol *insertInitStatementAndReturnTempSymbol(TIntermTyped *node,
                                                          TIntermSequence *insertions);
    bool mFoundSSBO;
};

RewriteExpressionsWithShaderStorageBlockTraverser::
    RewriteExpressionsWithShaderStorageBlockTraverser(TSymbolTable *symbolTable)
    : TIntermTraverser(true, false, false, symbolTable), mFoundSSBO(false)
{}

TIntermSymbol *
RewriteExpressionsWithShaderStorageBlockTraverser::insertInitStatementAndReturnTempSymbol(
    TIntermTyped *node,
    TIntermSequence *insertions)
{
    TIntermDeclaration *variableDeclaration;
    TVariable *tempVariable =
        DeclareTempVariable(mSymbolTable, node, EvqTemporary, &variableDeclaration);

    insertions->push_back(variableDeclaration);
    return CreateTempSymbolNode(tempVariable);
}

bool RewriteExpressionsWithShaderStorageBlockTraverser::visitBinary(Visit, TIntermBinary *node)
{
    if (mFoundSSBO)
    {
        return false;
    }

    bool rightSSBO = IsInShaderStorageBlock(node->getRight());
    bool leftSSBO  = IsInShaderStorageBlock(node->getLeft());
    if (!leftSSBO && !rightSSBO)
    {
        return true;
    }

    // case 1: Compound assigment operator
    //  original:
    //      lssbo += expr_no_ssbo;
    //  new:
    //      var temp = lssbo;
    //      temp += expr_no_ssbo;
    //      lssbo = temp;
    //
    //  original:
    //      lssbo += rssbo;
    //  new:
    //      var rvalue = rssbo;
    //      var temp = lssbo;
    //      temp += rvalue;
    //      lssbo = temp;
    //
    //  original:
    //      lvalue_no_ssbo += rssbo;
    //  new:
    //      var rvalue = rssbo;
    //      lvalue_no_ssbo += rvalue;
    if (IsCompoundAssignment(node->getOp()))
    {
        mFoundSSBO = true;
        TIntermSequence insertions;
        TIntermTyped *rightNode = node->getRight();
        if (rightSSBO)
        {
            rightNode = insertInitStatementAndReturnTempSymbol(node->getRight(), &insertions);
        }

        if (leftSSBO)
        {
            TIntermSymbol *tempSymbol =
                insertInitStatementAndReturnTempSymbol(node->getLeft(), &insertions);
            TIntermBinary *tempCompoundOperate =
                new TIntermBinary(node->getOp(), tempSymbol, rightNode);
            insertions.push_back(tempCompoundOperate);
            insertStatementsInParentBlock(insertions);

            TIntermBinary *assignTempValueToSSBO =
                new TIntermBinary(EOpAssign, node->getLeft(), tempSymbol);
            queueReplacement(assignTempValueToSSBO, OriginalNode::IS_DROPPED);
        }
        else
        {
            insertStatementsInParentBlock(insertions);
            TIntermBinary *assignTempValueToLValue =
                new TIntermBinary(node->getOp(), node->getLeft(), rightNode);
            queueReplacement(assignTempValueToLValue, OriginalNode::IS_DROPPED);
        }
    }
    // case 2: Readonly binary operator
    //  original:
    //      ssbo0 + ssbo1 + ssbo2;
    //  new:
    //      var temp0 = ssbo0;
    //      var temp1 = ssbo1;
    //      var temp2 = ssbo2;
    //      temp0 + temp1 + temp2;
    else if (IsReadonlyBinaryOperatorNotInSSBOAccessChain(node->getOp()) && (leftSSBO || rightSSBO))
    {
        mFoundSSBO              = true;
        TIntermTyped *rightNode = node->getRight();
        TIntermTyped *leftNode  = node->getLeft();
        TIntermSequence insertions;
        if (rightSSBO)
        {
            rightNode = insertInitStatementAndReturnTempSymbol(node->getRight(), &insertions);
        }
        if (leftSSBO)
        {
            leftNode = insertInitStatementAndReturnTempSymbol(node->getLeft(), &insertions);
        }

        insertStatementsInParentBlock(insertions);
        TIntermBinary *newExpr = new TIntermBinary(node->getOp(), leftNode, rightNode);
        queueReplacement(newExpr, OriginalNode::IS_DROPPED);
    }
    return !mFoundSSBO;
}

void RewriteExpressionsWithShaderStorageBlockTraverser::nextIteration()
{
    mFoundSSBO = false;
}

}  // anonymous namespace

void RewriteExpressionsWithShaderStorageBlock(TIntermNode *root, TSymbolTable *symbolTable)
{
    RewriteExpressionsWithShaderStorageBlockTraverser traverser(symbolTable);
    do
    {
        traverser.nextIteration();
        root->traverse(&traverser);
        if (traverser.foundSSBO())
            traverser.updateTree();
    } while (traverser.foundSSBO());
}
}  // namespace sh
