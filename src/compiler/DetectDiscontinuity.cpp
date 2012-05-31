//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Contains analysis utilities for dealing with HLSL's lack of support for
// the use of intrinsic functions which (implicitly or explicitly) compute
// gradients of functions with discontinuities. 
//

#include "compiler/DetectDiscontinuity.h"

namespace sh
{
bool DetectDiscontinuity::traverse(TIntermNode *node)
{
    mDiscontinuity = false;
    node->traverse(this);
    return mDiscontinuity;
}

bool DetectDiscontinuity::visitBranch(Visit visit, TIntermBranch *node)
{
    switch (node->getFlowOp())
    {
      case EOpKill:
        break;
      case EOpBreak:
      case EOpContinue:
        mDiscontinuity = true;
      case EOpReturn:
        break;
      default: UNREACHABLE();
    }

    return !mDiscontinuity;
}
}
