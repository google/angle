//
// Copyright (c) 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TranslatorMetal:
//   Translator for Metal backend.
//

#include "compiler/translator/TranslatorMetal.h"

#include "common/debug.h"
#include "compiler/translator/OutputVulkanGLSLForMetal.h"

namespace sh
{

TranslatorMetal::TranslatorMetal(sh::GLenum type, ShShaderSpec spec)
    : TCompiler(type, spec, SH_GLSL_450_CORE_OUTPUT)
{}

bool TranslatorMetal::translate(TIntermBlock *root,
                                ShCompileOptions compileOptions,
                                PerformanceDiagnostics * /*perfDiagnostics*/)
{
    TInfoSinkBase &sink = getInfoSink().obj;
    TOutputVulkanGLSLForMetal outputGLSL(sink, getArrayIndexClampingStrategy(), getHashFunction(),
                                         getNameMap(), &getSymbolTable(), getShaderType(),
                                         getShaderVersion(), getOutputType(), compileOptions);

    UNIMPLEMENTED();

    root->traverse(&outputGLSL);
    return false;
}

bool TranslatorMetal::shouldFlattenPragmaStdglInvariantAll()
{
    UNIMPLEMENTED();
    return false;
}

}  // namespace sh
