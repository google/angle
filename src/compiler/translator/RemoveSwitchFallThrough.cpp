//
// Copyright (c) 2002-2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RemoveSwitchFallThrough.cpp: Remove fall-through from switch statements.
// Note that it is unsafe to do further AST transformations on the AST generated
// by this function. It leaves duplicate nodes in the AST making replacements
// unreliable.

#include "compiler/translator/RemoveSwitchFallThrough.h"

#include "compiler/translator/IntermTraverse.h"

namespace sh
{

namespace
{

class RemoveSwitchFallThroughTraverser : public TIntermTraverser
{
  public:
    static TIntermBlock *removeFallThrough(TIntermBlock *statementList);

  private:
    RemoveSwitchFallThroughTraverser(TIntermBlock *statementList);

    void visitSymbol(TIntermSymbol *node) override;
    void visitConstantUnion(TIntermConstantUnion *node) override;
    bool visitDeclaration(Visit, TIntermDeclaration *node) override;
    bool visitBinary(Visit, TIntermBinary *node) override;
    bool visitUnary(Visit, TIntermUnary *node) override;
    bool visitTernary(Visit visit, TIntermTernary *node) override;
    bool visitSwizzle(Visit, TIntermSwizzle *node) override;
    bool visitIfElse(Visit visit, TIntermIfElse *node) override;
    bool visitSwitch(Visit, TIntermSwitch *node) override;
    bool visitCase(Visit, TIntermCase *node) override;
    bool visitAggregate(Visit, TIntermAggregate *node) override;
    bool visitBlock(Visit, TIntermBlock *node) override;
    bool visitLoop(Visit, TIntermLoop *node) override;
    bool visitBranch(Visit, TIntermBranch *node) override;

    void outputSequence(TIntermSequence *sequence, size_t startIndex);
    void handlePreviousCase();

    TIntermBlock *mStatementList;
    TIntermBlock *mStatementListOut;
    bool mLastStatementWasBreak;
    TIntermBlock *mPreviousCase;
    std::vector<TIntermBlock *> mCasesSharingBreak;
};

TIntermBlock *RemoveSwitchFallThroughTraverser::removeFallThrough(TIntermBlock *statementList)
{
    RemoveSwitchFallThroughTraverser rm(statementList);
    ASSERT(statementList);
    statementList->traverse(&rm);
    ASSERT(rm.mPreviousCase || statementList->getSequence()->empty());
    if (!rm.mLastStatementWasBreak && rm.mPreviousCase)
    {
        // Make sure that there's a branch at the end of the final case inside the switch statement.
        // This also ensures that any cases that fall through to the final case will get the break.
        TIntermBranch *finalBreak = new TIntermBranch(EOpBreak, nullptr);
        rm.mPreviousCase->getSequence()->push_back(finalBreak);
        rm.mLastStatementWasBreak = true;
    }
    rm.handlePreviousCase();
    return rm.mStatementListOut;
}

RemoveSwitchFallThroughTraverser::RemoveSwitchFallThroughTraverser(TIntermBlock *statementList)
    : TIntermTraverser(true, false, false),
      mStatementList(statementList),
      mLastStatementWasBreak(false),
      mPreviousCase(nullptr)
{
    mStatementListOut = new TIntermBlock();
}

void RemoveSwitchFallThroughTraverser::visitSymbol(TIntermSymbol *node)
{
    // Note that this assumes that switch statements which don't begin by a case statement
    // have already been weeded out in validation.
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
}

void RemoveSwitchFallThroughTraverser::visitConstantUnion(TIntermConstantUnion *node)
{
    // Conditions of case labels are not traversed, so this is some other constant
    // Could be just a statement like "0;"
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
}

bool RemoveSwitchFallThroughTraverser::visitDeclaration(Visit, TIntermDeclaration *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitBinary(Visit, TIntermBinary *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitUnary(Visit, TIntermUnary *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitTernary(Visit, TIntermTernary *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitSwizzle(Visit, TIntermSwizzle *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitIfElse(Visit, TIntermIfElse *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitSwitch(Visit, TIntermSwitch *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    // Don't go into nested switch statements
    return false;
}

void RemoveSwitchFallThroughTraverser::outputSequence(TIntermSequence *sequence, size_t startIndex)
{
    for (size_t i = startIndex; i < sequence->size(); ++i)
    {
        mStatementListOut->getSequence()->push_back(sequence->at(i));
    }
}

void RemoveSwitchFallThroughTraverser::handlePreviousCase()
{
    if (mPreviousCase)
        mCasesSharingBreak.push_back(mPreviousCase);
    if (mLastStatementWasBreak)
    {
        bool labelsWithNoStatements = true;
        for (size_t i = 0; i < mCasesSharingBreak.size(); ++i)
        {
            if (mCasesSharingBreak.at(i)->getSequence()->size() > 1)
            {
                labelsWithNoStatements = false;
            }
            if (labelsWithNoStatements)
            {
                // Fall-through is allowed in case the label has no statements.
                outputSequence(mCasesSharingBreak.at(i)->getSequence(), 0);
            }
            else
            {
                // Include all the statements that this case can fall through under the same label.
                for (size_t j = i; j < mCasesSharingBreak.size(); ++j)
                {
                    size_t startIndex =
                        j > i ? 1 : 0;  // Add the label only from the first sequence.
                    outputSequence(mCasesSharingBreak.at(j)->getSequence(), startIndex);
                }
            }
        }
        mCasesSharingBreak.clear();
    }
    mLastStatementWasBreak = false;
    mPreviousCase          = nullptr;
}

bool RemoveSwitchFallThroughTraverser::visitCase(Visit, TIntermCase *node)
{
    handlePreviousCase();
    mPreviousCase = new TIntermBlock();
    mPreviousCase->getSequence()->push_back(node);
    // Don't traverse the condition of the case statement
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitAggregate(Visit, TIntermAggregate *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitBlock(Visit, TIntermBlock *node)
{
    if (node != mStatementList)
    {
        mPreviousCase->getSequence()->push_back(node);
        mLastStatementWasBreak = false;
        return false;
    }
    return true;
}

bool RemoveSwitchFallThroughTraverser::visitLoop(Visit, TIntermLoop *node)
{
    mPreviousCase->getSequence()->push_back(node);
    mLastStatementWasBreak = false;
    return false;
}

bool RemoveSwitchFallThroughTraverser::visitBranch(Visit, TIntermBranch *node)
{
    mPreviousCase->getSequence()->push_back(node);
    // TODO: Verify that accepting return or continue statements here doesn't cause problems.
    mLastStatementWasBreak = true;
    return false;
}

}  // anonymous namespace

TIntermBlock *RemoveSwitchFallThrough(TIntermBlock *statementList)
{
    return RemoveSwitchFallThroughTraverser::removeFallThrough(statementList);
}

}  // namespace sh
