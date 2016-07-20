//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// IntermNodePatternMatcher is a helper class for matching node trees to given patterns.
// It can be used whenever the same checks for certain node structures are common to multiple AST
// traversers.
//

#include "compiler/translator/IntermNodePatternMatcher.h"

#include "compiler/translator/IntermNode.h"

namespace
{

bool IsNodeBlock(TIntermNode *node)
{
    ASSERT(node != nullptr);
    return (node->getAsAggregate() && node->getAsAggregate()->getOp() == EOpSequence);
}

}  // anonymous namespace

IntermNodePatternMatcher::IntermNodePatternMatcher(const unsigned int mask) : mMask(mask)
{
}

bool IntermNodePatternMatcher::match(TIntermBinary *node, TIntermNode *parentNode)
{
    if ((mMask & kExpressionReturningArray) != 0)
    {
        if (node->isArray() && node->getOp() == EOpAssign && parentNode != nullptr &&
            !IsNodeBlock(parentNode))
        {
            return true;
        }
    }

    if ((mMask & kUnfoldedShortCircuitExpression) != 0)
    {
        if (node->getRight()->hasSideEffects() &&
            (node->getOp() == EOpLogicalOr || node->getOp() == EOpLogicalAnd))
        {
            return true;
        }
    }
    return false;
}

bool IntermNodePatternMatcher::match(TIntermAggregate *node, TIntermNode *parentNode)
{
    if ((mMask & kExpressionReturningArray) != 0)
    {
        if (parentNode != nullptr)
        {
            TIntermBinary *parentBinary = parentNode->getAsBinaryNode();
            bool parentIsAssignment =
                (parentBinary != nullptr &&
                 (parentBinary->getOp() == EOpAssign || parentBinary->getOp() == EOpInitialize));

            if (node->getType().isArray() && !parentIsAssignment &&
                (node->isConstructor() || node->getOp() == EOpFunctionCall) &&
                !IsNodeBlock(parentNode))
            {
                return true;
            }
        }
    }
    return false;
}

bool IntermNodePatternMatcher::match(TIntermSelection *node)
{
    if ((mMask & kUnfoldedShortCircuitExpression) != 0)
    {
        if (node->usesTernaryOperator())
        {
            return true;
        }
    }
    return false;
}
