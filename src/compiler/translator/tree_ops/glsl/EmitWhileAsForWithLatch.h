//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EmitWhileAsForWithLatch.h: Rewrite while(cond) loops as for(mediump uint s = 0u; cond; s++) to
// work around driver shader compiler bugs.
//

#ifndef COMPILER_TRANSLATOR_TREEOPS_GLSL_EMITWHILEASFORWITHLATCH_H_
#define COMPILER_TRANSLATOR_TREEOPS_GLSL_EMITWHILEASFORWITHLATCH_H_

#include "common/angleutils.h"
#include "common/debug.h"

namespace sh
{

class TCompiler;
class TIntermBlock;

#ifdef ANGLE_ENABLE_GLSL
[[nodiscard]] bool EmitWhileAsForWithLatch(TCompiler *compiler, TIntermBlock *root);
#else
[[nodiscard]] ANGLE_INLINE bool EmitWhileAsForWithLatch(TCompiler *compiler, TIntermBlock *root)
{
    UNREACHABLE();
    return false;
}
#endif

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_TREEOPS_GLSL_EMITWHILEASFORWITHLATCH_H_
