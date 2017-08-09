//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_VARIABLEINFO_H_
#define COMPILER_TRANSLATOR_VARIABLEINFO_H_

#include <GLSLANG/ShaderLang.h>

#include "compiler/translator/ExtensionBehavior.h"

namespace sh
{

class TIntermBlock;
class TSymbolTable;

void CollectVariables(TIntermBlock *root,
                      std::vector<Attribute> *attributes,
                      std::vector<OutputVariable> *outputVariables,
                      std::vector<Uniform> *uniforms,
                      std::vector<Varying> *inputVaryings,
                      std::vector<Varying> *outputVaryings,
                      std::vector<InterfaceBlock> *uniformBlocks,
                      std::vector<InterfaceBlock> *shaderStorageBlocks,
                      ShHashFunction64 hashFunction,
                      TSymbolTable *symbolTable,
                      int shaderVersion,
                      const TExtensionBehavior &extensionBehavior);
}

#endif  // COMPILER_TRANSLATOR_VARIABLEINFO_H_
