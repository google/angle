//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_INITIALIZEVARIABLES_H_
#define COMPILER_TRANSLATOR_INITIALIZEVARIABLES_H_

#include <GLSLANG/ShaderLang.h>

#include "compiler/translator/IntermNode.h"

namespace sh
{
class TSymbolTable;

typedef std::vector<sh::ShaderVariable> InitVariableList;

// Return a sequence of assignment operations to initialize "initializedSymbol". initializedSymbol
// may be an array, struct or any combination of these, as long as it contains only basic types.
TIntermSequence *CreateInitCode(const TIntermSymbol *initializedSymbol);

// Initialize all uninitialized local variables, so that undefined behavior is avoided.
void InitializeUninitializedLocals(TIntermBlock *root, int shaderVersion);

// This function can initialize all the types that CreateInitCode is able to initialize. For struct
// typed variables it requires that the struct is found from the symbolTable, which is usually not
// the case for locally defined struct types.
// For now it is used for the following two scenarios:
//   1. initializing gl_Position;
//   2. initializing ESSL 3.00 shaders' output variables.
void InitializeVariables(TIntermBlock *root,
                         const InitVariableList &vars,
                         const TSymbolTable &symbolTable);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_INITIALIZEVARIABLES_H_
