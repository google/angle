//
// Copyright 2002 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/ValidateSwitch.h"

#include "compiler/translator/Diagnostics.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{

namespace
{

const int kMaxAllowedTraversalDepth = 256;

class ValidateSwitch : public TIntermTraverser
{
  public:
    static bool validate(TBasicType switchType,
                         TDiagnostics *diagnostics,
                         TIntermBlock *statementList,
                         const TSourceLoc &loc);

    bool visitBlock(Visit visit, TIntermBlock *) override;
    bool visitIfElse(Visit visit, TIntermIfElse *) override;
    bool visitSwitch(Visit, TIntermSwitch *) override;
    bool visitCase(Visit, TIntermCase *node) override;
    bool visitLoop(Visit visit, TIntermLoop *) override;

  private:
    ValidateSwitch(TBasicType switchType, TDiagnostics *context);

    bool validateInternal(const TSourceLoc &loc);

    TDiagnostics *mDiagnostics;
    int mControlFlowDepth;
    bool mCaseInsideControlFlow;
};

bool ValidateSwitch::validate(TBasicType switchType,
                              TDiagnostics *diagnostics,
                              TIntermBlock *statementList,
                              const TSourceLoc &loc)
{
    ValidateSwitch validate(switchType, diagnostics);
    ASSERT(statementList);
    statementList->traverse(&validate);
    return validate.validateInternal(loc);
}

ValidateSwitch::ValidateSwitch(TBasicType switchType, TDiagnostics *diagnostics)
    : TIntermTraverser(true, false, true, nullptr),
      mDiagnostics(diagnostics),
      mControlFlowDepth(0),
      mCaseInsideControlFlow(false)
{
    setMaxAllowedDepth(kMaxAllowedTraversalDepth);
}

bool ValidateSwitch::visitBlock(Visit visit, TIntermBlock *)
{
    if (getParentNode() != nullptr)
    {
        if (visit == PreVisit)
            ++mControlFlowDepth;
        if (visit == PostVisit)
            --mControlFlowDepth;
    }
    return true;
}

bool ValidateSwitch::visitIfElse(Visit visit, TIntermIfElse *)
{
    if (visit == PreVisit)
        ++mControlFlowDepth;
    if (visit == PostVisit)
        --mControlFlowDepth;
    return true;
}

bool ValidateSwitch::visitSwitch(Visit, TIntermSwitch *)
{
    // Don't go into nested switch statements
    return false;
}

bool ValidateSwitch::visitCase(Visit, TIntermCase *node)
{
    const char *nodeStr = node->hasCondition() ? "case" : "default";
    if (mControlFlowDepth > 0)
    {
        mDiagnostics->error(node->getLine(), "label statement nested inside control flow", nodeStr);
        mCaseInsideControlFlow = true;
    }
    // Don't traverse the condition of the case statement
    return false;
}

bool ValidateSwitch::visitLoop(Visit visit, TIntermLoop *)
{
    if (visit == PreVisit)
        ++mControlFlowDepth;
    if (visit == PostVisit)
        --mControlFlowDepth;
    return true;
}

bool ValidateSwitch::validateInternal(const TSourceLoc &loc)
{
    if (getMaxDepth() >= kMaxAllowedTraversalDepth)
    {
        mDiagnostics->error(loc, "too complex expressions inside a switch statement", "switch");
    }
    return !mCaseInsideControlFlow && getMaxDepth() < kMaxAllowedTraversalDepth;
}

}  // anonymous namespace

bool ValidateSwitchStatementList(TBasicType switchType,
                                 TDiagnostics *diagnostics,
                                 TIntermBlock *statementList,
                                 const TSourceLoc &loc)
{
    return ValidateSwitch::validate(switchType, diagnostics, statementList, loc);
}

}  // namespace sh
