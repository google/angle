//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORHLSL_H_
#define COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORHLSL_H_

#include "compiler/translator/BuiltInFunctionEmulator.h"

//
// This class is needed to emulate GLSL functions that don't exist in HLSL.
//
class BuiltInFunctionEmulatorHLSL : public BuiltInFunctionEmulator
{
  public:
    BuiltInFunctionEmulatorHLSL();

    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out) const;
};

#endif  // COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORHLSL_H_
