//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORGLSL_H_
#define COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORGLSL_H_

#include "compiler/translator/BuiltInFunctionEmulator.h"

//
// This class is only a workaround for OpenGL driver bugs, and isn't needed in general.
//
class BuiltInFunctionEmulatorGLSL : public BuiltInFunctionEmulator
{
  public:
    BuiltInFunctionEmulatorGLSL(sh::GLenum shaderType);

    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out, bool withPrecision) const;
};

#endif  // COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORGLSL_H_
