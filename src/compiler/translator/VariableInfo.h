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
                      std::vector<Varying> *varyings,
                      std::vector<InterfaceBlock> *interfaceBlocks,
                      ShHashFunction64 hashFunction,
                      const TSymbolTable &symbolTable,
                      int shaderVersion,
                      const TExtensionBehavior &extensionBehavior);

void ExpandVariable(const ShaderVariable &variable,
                    const std::string &name,
                    const std::string &mappedName,
                    bool markStaticUse,
                    std::vector<ShaderVariable> *expanded);

// Expand struct uniforms to flattened lists of split variables
void ExpandUniforms(const std::vector<Uniform> &compact, std::vector<ShaderVariable> *expanded);
}

#endif  // COMPILER_TRANSLATOR_VARIABLEINFO_H_
