//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EmulateGLDrawID is an AST traverser to convert the gl_DrawID builtin
// to a uniform int
//
// EmulateGLBaseVertex is an AST traverser to convert the gl_BaseVertex builtin
// to a uniform int
//
// EmulateGLBaseInstance is an AST traverser to convert the gl_BaseInstance builtin
// to a uniform int
//

#ifndef COMPILER_TRANSLATOR_TREEOPS_EMULATEMULTIDRAWSHADERBUILTINS_H_
#define COMPILER_TRANSLATOR_TREEOPS_EMULATEMULTIDRAWSHADERBUILTINS_H_

#include <GLSLANG/ShaderLang.h>
#include <vector>

#include "common/angleutils.h"
#include "compiler/translator/HashNames.h"

namespace sh
{
struct Uniform;
class TCompiler;
class TIntermBlock;
class TSymbolTable;

ANGLE_NO_DISCARD bool EmulateGLDrawID(TCompiler *compiler,
                                      TIntermBlock *root,
                                      TSymbolTable *symbolTable,
                                      std::vector<sh::Uniform> *uniforms,
                                      bool shouldCollect);

ANGLE_NO_DISCARD bool EmulateGLBaseVertex(TCompiler *compiler,
                                          TIntermBlock *root,
                                          TSymbolTable *symbolTable,
                                          std::vector<sh::Uniform> *uniforms,
                                          bool shouldCollect);

ANGLE_NO_DISCARD bool EmulateGLBaseInstance(TCompiler *compiler,
                                            TIntermBlock *root,
                                            TSymbolTable *symbolTable,
                                            std::vector<sh::Uniform> *uniforms,
                                            bool shouldCollect);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_TREEOPS_EMULATEMULTIDRAWSHADERBUILTINS_H_
