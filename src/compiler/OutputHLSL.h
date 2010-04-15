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
class UnfoldSelect;

class OutputHLSL : public TIntermTraverser
{
  public:
    explicit OutputHLSL(TParseContext &context);
    ~OutputHLSL();

    void output();

    TInfoSinkBase &getBodyStream();

    static TString argumentString(const TIntermSymbol *symbol);
    static TString qualifierString(TQualifier qualifier);
    static TString typeString(const TType &type);
    static TString arrayString(const TType &type);
    static TString initializer(const TType &type);
    static TString decorate(const TString &string);   // Prepend an underscore to avoid naming clashes

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

    bool isSingleStatement(TIntermNode *node);
    bool handleExcessiveLoop(TIntermLoop *node);
    void outputTriplet(Visit visit, const char *preString, const char *inString, const char *postString);

    TParseContext &mContext;
    UnfoldSelect *mUnfoldSelect;

    // Output streams
    TInfoSinkBase mHeader;
    TInfoSinkBase mBody;
    TInfoSinkBase mFooter;

    // Parameters determining what goes in the header output
    bool mUsesEqualMat2;
    bool mUsesEqualMat3;
    bool mUsesEqualMat4;
    bool mUsesEqualVec2;
    bool mUsesEqualVec3;
    bool mUsesEqualVec4;
    bool mUsesEqualIVec2;
    bool mUsesEqualIVec3;
    bool mUsesEqualIVec4;
    bool mUsesEqualBVec2;
    bool mUsesEqualBVec3;
    bool mUsesEqualBVec4;
};
}

#endif   // COMPILER_OUTPUTHLSL_H_
