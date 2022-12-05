//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The ValidateClipCullDistance function checks if the sum of array sizes for gl_ClipDistance and
// gl_CullDistance exceeds gl_MaxCombinedClipAndCullDistances
//

#ifndef COMPILER_TRANSLATOR_VALIDATECLIPCULLDISTANCE_H_
#define COMPILER_TRANSLATOR_VALIDATECLIPCULLDISTANCE_H_

#include "GLSLANG/ShaderVars.h"

namespace sh
{

class TIntermBlock;
class TDiagnostics;

bool ValidateClipCullDistance(TIntermBlock *root,
                              TDiagnostics *diagnostics,
                              const unsigned int maxCombinedClipAndCullDistances,
                              const bool limitSimultaneousClipAndCullDistanceUsage,
                              uint8_t *clipDistanceSizeOut,
                              uint8_t *cullDistanceSizeOut,
                              int8_t *clipDistanceMaxIndexOut,
                              int8_t *cullDistanceMaxIndexOut);

}  // namespace sh

#endif
