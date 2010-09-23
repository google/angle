//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/VariableInfo.h"

CollectAttribsUniforms::CollectAttribsUniforms(TVariableInfoList& attribs,
                                               TVariableInfoList& uniforms)
    : mAttribs(attribs),
      mUniforms(uniforms)
{
}

// TODO(alokp): Implement these functions.
void CollectAttribsUniforms::visitSymbol(TIntermSymbol*)
{
}

void CollectAttribsUniforms::visitConstantUnion(TIntermConstantUnion*)
{
}

bool CollectAttribsUniforms::visitBinary(Visit visit, TIntermBinary*)
{
    return false;
}

bool CollectAttribsUniforms::visitUnary(Visit visit, TIntermUnary*)
{
    return false;
}

bool CollectAttribsUniforms::visitSelection(Visit visit, TIntermSelection*)
{
    return false;
}

bool CollectAttribsUniforms::visitAggregate(Visit visit, TIntermAggregate*)
{
    return false;
}

bool CollectAttribsUniforms::visitLoop(Visit visit, TIntermLoop*)
{
    return false;
}

bool CollectAttribsUniforms::visitBranch(Visit visit, TIntermBranch*)
{
    return false;
}

