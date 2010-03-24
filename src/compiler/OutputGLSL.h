//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef CROSSCOMPILERGLSL_OUTPUTGLSL_H_
#define CROSSCOMPILERGLSL_OUTPUTGLSL_H_

#include "intermediate.h"
#include "ParseHelper.h"

class TOutputGLSL : public TIntermTraverser
{
public:
    TOutputGLSL(TParseContext &context);

    void header();

protected:
    TInfoSinkBase& objSink() { return parseContext.infoSink.obj; }
    void writeTriplet(Visit visit, const char* preStr, const char* inStr, const char* postStr);

    virtual void visitSymbol(TIntermSymbol* node);
    virtual void visitConstantUnion(TIntermConstantUnion* node);
    virtual bool visitBinary(Visit visit, TIntermBinary* node);
    virtual bool visitUnary(Visit visit, TIntermUnary* node);
    virtual bool visitSelection(Visit visit, TIntermSelection* node);
    virtual bool visitAggregate(Visit visit, TIntermAggregate* node);
    virtual bool visitLoop(Visit visit, TIntermLoop* node);
    virtual bool visitBranch(Visit visit, TIntermBranch* node);

private:
    bool writeFullSymbol;
    TParseContext &parseContext;
};

#endif  // CROSSCOMPILERGLSL_OUTPUTGLSL_H_
