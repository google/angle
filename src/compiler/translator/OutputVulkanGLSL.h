//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// OutputVulkanGLSL:
//   Code that outputs shaders that fit GL_KHR_vulkan_glsl.
//   The shaders are then fed into glslang to spit out SPIR-V (libANGLE-side).
//   See: https://www.khronos.org/registry/vulkan/specs/misc/GL_KHR_vulkan_glsl.txt
//

#include "compiler/translator/OutputGLSL.h"

namespace sh
{

class TOutputVulkanGLSL : public TOutputGLSL
{
  public:
    TOutputVulkanGLSL(TInfoSinkBase &objSink,
                      ShArrayIndexClampingStrategy clampingStrategy,
                      ShHashFunction64 hashFunction,
                      NameMap &nameMap,
                      TSymbolTable *symbolTable,
                      sh::GLenum shaderType,
                      int shaderVersion,
                      ShShaderOutput output,
                      ShCompileOptions compileOptions);

    void writeStructType(const TStructure *structure);

    uint32_t nextUnusedBinding() { return mNextUnusedBinding++; }

  protected:
    void writeLayoutQualifier(TIntermTyped *variable) override;
    void writeQualifier(TQualifier qualifier, const TType &type, const TSymbol *symbol) override;
    void writeVariableType(const TType &type,
                           const TSymbol *symbol,
                           bool isFunctionArgument) override;

    // Every resource that requires set & binding layout qualifiers is assigned set 0 and an
    // arbitrary binding when outputting GLSL.  Glslang wrapper modifies set and binding decorations
    // in SPIR-V directly.
    uint32_t mNextUnusedBinding;
};

}  // namespace sh
