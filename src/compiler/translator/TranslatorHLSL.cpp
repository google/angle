//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/TranslatorHLSL.h"

#include "compiler/translator/InitializeParseContext.h"
#include "compiler/translator/OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(sh::GLenum type, ShShaderSpec spec, ShShaderOutput output)
    : TCompiler(type, spec, output)
{
}

void TranslatorHLSL::translate(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext, getResources(), getOutputType());

    outputHLSL.output();

    attributes      = outputHLSL.getAttributes();
    outputVariables = outputHLSL.getOutputVariables();
    uniforms        = outputHLSL.getUniforms();
    varyings        = outputHLSL.getVaryings();
    interfaceBlocks = outputHLSL.getInterfaceBlocks();

    mInterfaceBlockRegisterMap = outputHLSL.getInterfaceBlockRegisterMap();
}

bool TranslatorHLSL::hasInterfaceBlock(const std::string &interfaceBlockName) const
{
    return (mInterfaceBlockRegisterMap.count(interfaceBlockName) > 0);
}

unsigned int TranslatorHLSL::getInterfaceBlockRegister(const std::string &interfaceBlockName) const
{
    ASSERT(hasInterfaceBlock(interfaceBlockName));
    return mInterfaceBlockRegisterMap.find(interfaceBlockName)->second;
}
