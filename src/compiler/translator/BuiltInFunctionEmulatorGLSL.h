//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORGLSL_H_
#define COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORGLSL_H_

#include "GLSLANG/ShaderLang.h"

class BuiltInFunctionEmulator;

//
// This is only a workaround for OpenGL driver bugs, and isn't needed in general.
//
void InitBuiltInFunctionEmulatorForGLSL(BuiltInFunctionEmulator *emu, sh::GLenum shaderType);

//
// This function is emulating built-in functions missing from OpenGL 4.1.
//
void InitBuiltInFunctionEmulatorForGLSL4_1(BuiltInFunctionEmulator *emu, sh::GLenum shaderType);

#endif  // COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATORGLSL_H_
