//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderGL.cpp: Implements the class methods for ShaderGL.

#include "libANGLE/renderer/gl/ShaderGL.h"

#include "common/debug.h"
#include "libANGLE/Compiler.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/RendererGL.h"

namespace rx
{

ShaderGL::ShaderGL(gl::Shader::Data *data,
                   const gl::Limitations &rendererLimitations,
                   const FunctionsGL *functions)
    : ShaderSh(data, rendererLimitations), mFunctions(functions), mShaderID(0)
{
    ASSERT(mFunctions);
}

ShaderGL::~ShaderGL()
{
    if (mShaderID != 0)
    {
        mFunctions->deleteShader(mShaderID);
        mShaderID = 0;
    }
}

bool ShaderGL::compile(gl::Compiler *compiler, const std::string &source, int additionalOptionsIn)
{
    // Reset the previous state
    if (mShaderID != 0)
    {
        mFunctions->deleteShader(mShaderID);
        mShaderID = 0;
    }

    int additionalOptions = (additionalOptionsIn | SH_INIT_GL_POSITION);
    if (!ShaderSh::compile(compiler, source, additionalOptions))
    {
        return false;
    }

    // Translate the ESSL into GLSL
    const char *translatedSourceCString = mData->getTranslatedSource().c_str();

    // Generate a shader object and set the source
    mShaderID = mFunctions->createShader(mData->getShaderType());
    mFunctions->shaderSource(mShaderID, 1, &translatedSourceCString, nullptr);
    mFunctions->compileShader(mShaderID);

    // Check for compile errors from the native driver
    GLint compileStatus = GL_FALSE;
    mFunctions->getShaderiv(mShaderID, GL_COMPILE_STATUS, &compileStatus);
    ASSERT(compileStatus == GL_TRUE);
    if (compileStatus == GL_FALSE)
    {
        // Compilation failed, put the error into the info log
        GLint infoLogLength = 0;
        mFunctions->getShaderiv(mShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> buf(infoLogLength);
        mFunctions->getShaderInfoLog(mShaderID, infoLogLength, nullptr, &buf[0]);

        mFunctions->deleteShader(mShaderID);
        mShaderID = 0;

        // TODO(jmadill): possibly pass in info log?
        mData->getMutableInfoLog() = &buf[0];
        TRACE("\n%s", mData->getMutableInfoLog().c_str());
        return false;
    }

    return true;
}

std::string ShaderGL::getDebugInfo() const
{
    return std::string();
}

GLuint ShaderGL::getShaderID() const
{
    return mShaderID;
}

}
