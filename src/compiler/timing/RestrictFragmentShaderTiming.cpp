//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/InfoSink.h"
#include "compiler/ParseHelper.h"
#include "compiler/depgraph/DependencyGraphOutput.h"
#include "compiler/timing/RestrictFragmentShaderTiming.h"

// FIXME(mvujovic): We do not know if the execution time of built-in operations like sin, pow, etc.
// can vary based on the value of the input arguments. If so, we should restrict those as well.
void RestrictFragmentShaderTiming::enforceRestrictions(const TDependencyGraph& graph)
{
    mNumErrors = 0;

    // FIXME(mvujovic): The dependency graph does not support user defined function calls right now,
    // so we generate errors for them.
    validateUserDefinedFunctionCallUsage(graph);

    // Traverse the dependency graph starting at s_texture and generate an error each time we hit a
    // condition node.
    TGraphSymbol* uTextureGraphSymbol = graph.getGlobalSymbolByName(mRestrictedSymbol);
    if (uTextureGraphSymbol &&
        uTextureGraphSymbol->getIntermSymbol()->getQualifier() == EvqUniform &&
        uTextureGraphSymbol->getIntermSymbol()->getBasicType() == EbtSampler2D)
        uTextureGraphSymbol->traverse(this);
}

void RestrictFragmentShaderTiming::validateUserDefinedFunctionCallUsage(const TDependencyGraph& graph)
{
    for (TFunctionCallVector::const_iterator iter = graph.beginUserDefinedFunctionCalls();
         iter != graph.endUserDefinedFunctionCalls();
         ++iter)
    {
        TGraphFunctionCall* functionCall = *iter;
        beginError(functionCall->getIntermFunctionCall());
        mSink << "A call to a user defined function is not permitted.\n";
    }
}

void RestrictFragmentShaderTiming::beginError(const TIntermNode* node)
{
    ++mNumErrors;
    mSink.prefix(EPrefixError);
    mSink.location(node->getLine());
}

void RestrictFragmentShaderTiming::visitArgument(TGraphArgument* parameter)
{
    // FIXME(mvujovic): We should restrict sampler dependent values from being texture coordinates 
    // in all available sampling operationsn supported in GLSL ES.
    // This includes overloaded signatures of texture2D, textureCube, and others.
    if (parameter->getIntermFunctionCall()->getName() != "texture2D(s21;vf2;" ||
        parameter->getArgumentNumber() != 1)
        return;

    beginError(parameter->getIntermFunctionCall());
    mSink << "An expression dependent on a uniform sampler2D by the name '" << mRestrictedSymbol
          << "' is not permitted to be the second argument of a texture2D call.\n";
}

void RestrictFragmentShaderTiming::visitSelection(TGraphSelection* selection)
{
    beginError(selection->getIntermSelection());
    mSink << "An expression dependent on a uniform sampler2D by the name '" << mRestrictedSymbol
          << "' is not permitted in a conditional statement.\n";
}

void RestrictFragmentShaderTiming::visitLoop(TGraphLoop* loop)
{
    beginError(loop->getIntermLoop());
    mSink << "An expression dependent on a uniform sampler2D by the name '" << mRestrictedSymbol
          << "' is not permitted in a loop condition.\n";
}

void RestrictFragmentShaderTiming::visitLogicalOp(TGraphLogicalOp* logicalOp)
{
    beginError(logicalOp->getIntermLogicalOp());
    mSink << "An expression dependent on a uniform sampler2D by the name '" << mRestrictedSymbol
          << "' is not permitted on the left hand side of a logical " << logicalOp->getOpString()
          << " operator.\n";
}
