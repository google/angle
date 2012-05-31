//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Contains analysis utilities for dealing with HLSL's lack of support for
// the use of intrinsic functions which (implicitly or explicitly) compute
// gradients of functions with discontinuities. 
//

#ifndef COMPILER_DETECTDISCONTINUITY_H_
#define COMPILER_DETECTDISCONTINUITY_H_

#include "compiler/intermediate.h"

namespace sh
{
class DetectDiscontinuity : public TIntermTraverser
{
  public:
    bool traverse(TIntermNode *node);

  protected:
    bool visitBranch(Visit visit, TIntermBranch *node);

    bool mDiscontinuity;
};

bool containsDiscontinuity(TIntermNode *node)
{
    DetectDiscontinuity detectDiscontinuity;
    return detectDiscontinuity.traverse(node);
}
}

#endif   // COMPILER_DETECTDISCONTINUITY_H_
