//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_OUTPUTHLSL_H_
#define COMPILER_OUTPUTHLSL_H_

#include "intermediate.h"
#include "ParseHelper.h"

namespace sh
{
class OutputHLSL : public TIntermTraverser
{
  public:
    OutputHLSL(TParseContext &context);

    void output();

  protected:
    void header();
    void footer();

    // Visit AST nodes and output their code to the body stream
    void visitSymbol(TIntermSymbol*);
    void visitConstantUnion(TIntermConstantUnion*);
    bool visitBinary(Visit visit, TIntermBinary*);
    bool visitUnary(Visit visit, TIntermUnary*);
    bool visitSelection(Visit visit, TIntermSelection*);
    bool visitAggregate(Visit visit, TIntermAggregate*);
    bool visitLoop(Visit visit, TIntermLoop*);
    bool visitBranch(Visit visit, TIntermBranch*);

    bool handleExcessiveLoop(TIntermLoop *node);
    void outputTriplet(Visit visit, const char *preString, const char *inString, const char *postString);

    static TString argumentString(const TIntermSymbol *symbol);
    static TString qualifierString(TQualifier qualifier);
    static TString typeString(const TType &type);
    static TString arrayString(const TType &type);
    static TString initializer(const TType &type);

    TParseContext &mContext;

    // Output streams
    TInfoSinkBase mHeader;
    TInfoSinkBase mBody;
    TInfoSinkBase mFooter;
};
}

#endif   // COMPILER_OUTPUTHLSL_H_
