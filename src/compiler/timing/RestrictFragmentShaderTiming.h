//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TIMING_RESTRICT_FRAGMENT_SHADER_TIMING_H_
#define COMPILER_TIMING_RESTRICT_FRAGMENT_SHADER_TIMING_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/depgraph/DependencyGraph.h"

class TInfoSinkBase;

class RestrictFragmentShaderTiming : TDependencyGraphTraverser {
public:
    RestrictFragmentShaderTiming(TInfoSinkBase& sink, const TString& restrictedSymbol)
        : mSink(sink)
        , mRestrictedSymbol(restrictedSymbol)
        , mNumErrors(0) {}

    void enforceRestrictions(const TDependencyGraph& graph);
    int numErrors() const { return mNumErrors; }

    virtual void visitArgument(TGraphArgument* parameter);
    virtual void visitSelection(TGraphSelection* selection);
    virtual void visitLoop(TGraphLoop* loop);
    virtual void visitLogicalOp(TGraphLogicalOp* logicalOp);

private:
    void beginError(const TIntermNode* node);
    void validateUserDefinedFunctionCallUsage(const TDependencyGraph& graph);

	TInfoSinkBase& mSink;
    const TString mRestrictedSymbol;
    int mNumErrors;
};

#endif  // COMPILER_TIMING_RESTRICT_FRAGMENT_SHADER_TIMING_H_
