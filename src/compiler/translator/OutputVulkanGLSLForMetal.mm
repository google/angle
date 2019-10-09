//
// Copyright (c) 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TOutputVulkanGLSLForMetal:
//    This is a special version Vulkan GLSL output that will make some special
//    considerations for Metal backend limitations.
//

#include "compiler/translator/OutputVulkanGLSLForMetal.h"

#include "common/debug.h"

namespace sh
{

TOutputVulkanGLSLForMetal::TOutputVulkanGLSLForMetal(TInfoSinkBase &objSink,
                                                     ShArrayIndexClampingStrategy clampingStrategy,
                                                     ShHashFunction64 hashFunction,
                                                     NameMap &nameMap,
                                                     TSymbolTable *symbolTable,
                                                     sh::GLenum shaderType,
                                                     int shaderVersion,
                                                     ShShaderOutput output,
                                                     ShCompileOptions compileOptions)
    : TOutputVulkanGLSL(objSink,
                        clampingStrategy,
                        hashFunction,
                        nameMap,
                        symbolTable,
                        shaderType,
                        shaderVersion,
                        output,
                        compileOptions)
{
    UNIMPLEMENTED();
}

}  // namespace sh
