//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/timing/RestrictVertexShaderTiming.h"

void RestrictVertexShaderTiming::visitSymbol(TIntermSymbol* node)
{
    if (node->getQualifier() == EvqUniform &&
        node->getBasicType() == EbtSampler2D &&
        node->getSymbol() == mRestrictedSymbol) {
        mFoundRestrictedSymbol = true;
        mSink.prefix(EPrefixError);
        mSink.location(node->getLine());
        mSink << "Definition of a uniform sampler2D by the name '" << mRestrictedSymbol
              << "' is not permitted in vertex shaders.\n";
    }
}

bool RestrictVertexShaderTiming::visitAggregate(Visit visit, TIntermAggregate* node)
{
    // Don't keep exploring if we've found the restricted symbol, and don't explore anything besides
    // the global scope (i.e. don't explore function definitions).
    if (mFoundRestrictedSymbol || node->getOp() == EOpFunction)
        return false;

    return true;
}
