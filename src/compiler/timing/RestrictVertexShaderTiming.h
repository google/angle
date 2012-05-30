//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TIMING_RESTRICT_VERTEX_SHADER_TIMING_H_
#define COMPILER_TIMING_RESTRICT_VERTEX_SHADER_TIMING_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/InfoSink.h"

class TInfoSinkBase;

class RestrictVertexShaderTiming : public TIntermTraverser {
public:
    RestrictVertexShaderTiming(TInfoSinkBase& sink, const TString& restrictedSymbol)
        : TIntermTraverser(true, false, false)
        , mSink(sink)
        , mRestrictedSymbol(restrictedSymbol)
        , mFoundRestrictedSymbol(false) {}

    void enforceRestrictions(TIntermNode* root) { root->traverse(this); }
    int numErrors() { return mFoundRestrictedSymbol ? 1 : 0; }

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitBinary(Visit visit, TIntermBinary*) { return false; }
    virtual bool visitUnary(Visit visit, TIntermUnary*) { return false; }
    virtual bool visitSelection(Visit visit, TIntermSelection*) { return false; }
    virtual bool visitAggregate(Visit visit, TIntermAggregate*);
    virtual bool visitLoop(Visit visit, TIntermLoop*) { return false; };
    virtual bool visitBranch(Visit visit, TIntermBranch*) { return false; };
private:
    TInfoSinkBase& mSink;
    const TString mRestrictedSymbol;
    bool mFoundRestrictedSymbol;
};

#endif  // COMPILER_TIMING_RESTRICT_VERTEX_SHADER_TIMING_H_
