//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderSh:
//   Common class representing a shader compile with ANGLE's translator.
//

#include "libANGLE/renderer/ShaderSh.h"

#include "common/utilities.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Compiler.h"

namespace rx
{

namespace
{
template <typename VarT>
std::vector<VarT> GetActiveShaderVariables(const std::vector<VarT> *variableList)
{
    ASSERT(variableList);
    std::vector<VarT> result;
    for (size_t varIndex = 0; varIndex < variableList->size(); varIndex++)
    {
        const VarT &var = variableList->at(varIndex);
        if (var.staticUse)
        {
            result.push_back(var);
        }
    }
    return result;
}

template <typename VarT>
const std::vector<VarT> &GetShaderVariables(const std::vector<VarT> *variableList)
{
    ASSERT(variableList);
    return *variableList;
}

// true if varying x has a higher priority in packing than y
bool CompareVarying(const sh::Varying &x, const sh::Varying &y)
{
    if (x.type == y.type)
    {
        return x.arraySize > y.arraySize;
    }

    // Special case for handling structs: we sort these to the end of the list
    if (x.type == GL_STRUCT_ANGLEX)
    {
        return false;
    }

    if (y.type == GL_STRUCT_ANGLEX)
    {
        return true;
    }

    return gl::VariableSortOrder(x.type) < gl::VariableSortOrder(y.type);
}

}  // anonymous namespace

ShaderSh::ShaderSh(GLenum type, const gl::Limitations &rendererLimitations)
    : mShaderType(type), mRendererLimitations(rendererLimitations)
{
}

ShaderSh::~ShaderSh()
{
}

bool ShaderSh::compile(gl::Compiler *compiler, const std::string &source, int additionalOptions)
{
    // Reset the previous state
    mActiveAttributes.clear();
    mVaryings.clear();
    mUniforms.clear();
    mInterfaceBlocks.clear();
    mActiveOutputVariables.clear();
    mTranslatedSource.clear();

    ShHandle compilerHandle = compiler->getCompilerHandle(mShaderType);

    int compileOptions = (SH_OBJECT_CODE | SH_VARIABLES | additionalOptions);

    // Some targets (eg D3D11 Feature Level 9_3 and below) do not support non-constant loop indexes
    // in fragment shaders. Shader compilation will fail. To provide a better error message we can
    // instruct the compiler to pre-validate.
    if (mRendererLimitations.shadersRequireIndexedLoopValidation)
    {
        compileOptions |= SH_VALIDATE_LOOP_INDEXING;
    }

    const char *sourceCString = source.c_str();
    bool result               = ShCompile(compilerHandle, &sourceCString, 1, compileOptions);

    if (!result)
    {
        mInfoLog = ShGetInfoLog(compilerHandle);
        TRACE("\n%s", mInfoLog.c_str());
        return false;
    }

    mTranslatedSource = ShGetObjectCode(compilerHandle);

#ifndef NDEBUG
    // Prefix translated shader with commented out un-translated shader.
    // Useful in diagnostics tools which capture the shader source.
    std::ostringstream shaderStream;
    shaderStream << "// GLSL\n";
    shaderStream << "//\n";

    size_t curPos = 0;
    while (curPos != std::string::npos)
    {
        size_t nextLine = source.find("\n", curPos);
        size_t len      = (nextLine == std::string::npos) ? std::string::npos : (nextLine - curPos + 1);

        shaderStream << "// " << source.substr(curPos, len);

        curPos = (nextLine == std::string::npos) ? std::string::npos : (nextLine + 1);
    }
    shaderStream << "\n\n";
    shaderStream << mTranslatedSource;
    mTranslatedSource = shaderStream.str();
#endif

    // Gather the shader information
    mShaderVersion = ShGetShaderVersion(compilerHandle);

    mVaryings        = GetShaderVariables(ShGetVaryings(compilerHandle));
    mUniforms        = GetShaderVariables(ShGetUniforms(compilerHandle));
    mInterfaceBlocks = GetShaderVariables(ShGetInterfaceBlocks(compilerHandle));

    if (mShaderType == GL_VERTEX_SHADER)
    {
        mActiveAttributes = GetActiveShaderVariables(ShGetAttributes(compilerHandle));
    }
    else
    {
        ASSERT(mShaderType == GL_FRAGMENT_SHADER);

        // TODO(jmadill): Figure out why we only sort in the FS, and if we need to.
        std::sort(mVaryings.begin(), mVaryings.end(), CompareVarying);
        mActiveOutputVariables = GetActiveShaderVariables(ShGetOutputVariables(compilerHandle));
    }

    ASSERT(!mTranslatedSource.empty());
    return true;
}

}  // namespace rx
