//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Shader.cpp: Implements the gl::Shader class and its  derived classes
// VertexShader and FragmentShader. Implements GL shader objects and related
// functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section 3.8 page 84.

#include "libGLESv2/Shader.h"

#include "GLSLANG/Shaderlang.h"
#include "compiler/OutputHLSL.h"
#include "common/debug.h"

#include "libGLESv2/main.h"

namespace gl
{
void *Shader::mFragmentCompiler = NULL;
void *Shader::mVertexCompiler = NULL;

Shader::Shader(GLuint handle) : mHandle(handle)
{
    mSource = NULL;
    mHlsl = NULL;
    mInfoLog = NULL;

    // Perform a one-time initialization of the shader compiler (or after being destructed by releaseCompiler)
    if (!mFragmentCompiler)
    {
        int result = ShInitialize();

        if (result)
        {
            mFragmentCompiler = ShConstructCompiler(EShLangFragment, EDebugOpObjectCode);
            mVertexCompiler = ShConstructCompiler(EShLangVertex, EDebugOpObjectCode);
        }
    }

    mAttachCount = 0;
    mDeleteStatus = false;
}

Shader::~Shader()
{
    delete[] mSource;
    delete[] mHlsl;
    delete[] mInfoLog;
}

GLuint Shader::getHandle() const
{
    return mHandle;
}

void Shader::setSource(GLsizei count, const char **string, const GLint *length)
{
    delete[] mSource;
    int totalLength = 0;

    for (int i = 0; i < count; i++)
    {
        if (length && length[i] >= 0)
        {
            totalLength += length[i];
        }
        else
        {
            totalLength += (int)strlen(string[i]);
        }
    }

    mSource = new char[totalLength + 1];
    char *code = mSource;

    for (int i = 0; i < count; i++)
    {
        int stringLength;

        if (length && length[i] >= 0)
        {
            stringLength = length[i];
        }
        else
        {
            stringLength = (int)strlen(string[i]);
        }

        strncpy(code, string[i], stringLength);
        code += stringLength;
    }

    mSource[totalLength] = '\0';
}

int Shader::getInfoLogLength() const
{
    if (!mInfoLog)
    {
        return 0;
    }
    else
    {
       return strlen(mInfoLog) + 1;
    }
}

void Shader::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog)
{
    int index = 0;

    if (mInfoLog)
    {
        while (index < bufSize - 1 && index < (int)strlen(mInfoLog))
        {
            infoLog[index] = mInfoLog[index];
            index++;
        }
    }

    if (bufSize)
    {
        infoLog[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

int Shader::getSourceLength() const
{
    if (!mSource)
    {
        return 0;
    }
    else
    {
       return strlen(mSource) + 1;
    }
}

void Shader::getSource(GLsizei bufSize, GLsizei *length, char *source)
{
    int index = 0;

    if (mSource)
    {
        while (index < bufSize - 1 && index < (int)strlen(mSource))
        {
            source[index] = mSource[index];
            index++;
        }
    }

    if (bufSize)
    {
        source[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

bool Shader::isCompiled()
{
    return mHlsl != NULL;
}

const char *Shader::getHLSL()
{
    return mHlsl;
}

void Shader::attach()
{
    mAttachCount++;
}

void Shader::detach()
{
    mAttachCount--;
}

bool Shader::isAttached() const
{
    return mAttachCount > 0;
}

bool Shader::isDeletable() const
{
    return mDeleteStatus == true && mAttachCount == 0;
}

bool Shader::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Shader::flagForDeletion()
{
    mDeleteStatus = true;
}

void Shader::releaseCompiler()
{
    ShDestruct(mFragmentCompiler);
    ShDestruct(mVertexCompiler);

    mFragmentCompiler = NULL;
    mVertexCompiler = NULL;
}

void Shader::compileToHLSL(void *compiler)
{
    if (isCompiled() || !mSource)
    {
        return;
    }

    TRACE("\n%s", mSource);

    delete[] mInfoLog;
    mInfoLog = NULL;

    TBuiltInResource resources;

    resources.maxVertexAttribs = MAX_VERTEX_ATTRIBS;
    resources.maxVertexUniformVectors = MAX_VERTEX_UNIFORM_VECTORS;
    resources.maxVaryingVectors = MAX_VARYING_VECTORS;
    resources.maxVertexTextureImageUnits = MAX_VERTEX_TEXTURE_IMAGE_UNITS;
    resources.maxCombinedTextureImageUnits = MAX_COMBINED_TEXTURE_IMAGE_UNITS;
    resources.maxTextureImageUnits = MAX_TEXTURE_IMAGE_UNITS;
    resources.maxFragmentUniformVectors = MAX_FRAGMENT_UNIFORM_VECTORS;
    resources.maxDrawBuffers = MAX_DRAW_BUFFERS;

    int result = ShCompile(compiler, &mSource, 1, EShOptNone, &resources, EDebugOpObjectCode);
    const char *obj = ShGetObjectCode(compiler);
    const char *info = ShGetInfoLog(compiler);

    if (result)
    {
        mHlsl = new char[strlen(obj) + 1];
        strcpy(mHlsl, obj);

        TRACE("\n%s", mHlsl);
    }
    else
    {
        mInfoLog = new char[strlen(info) + 1];
        strcpy(mInfoLog, info);

        TRACE("\n%s", mInfoLog);
    }
}

VertexShader::VertexShader(GLuint handle) : Shader(handle)
{
}

VertexShader::~VertexShader()
{
}

GLenum VertexShader::getType()
{
    return GL_VERTEX_SHADER;
}

void VertexShader::compile()
{
    compileToHLSL(mVertexCompiler);
    parseAttributes();
}

const Attribute &VertexShader::getAttribute(unsigned int semanticIndex)
{
    ASSERT(semanticIndex < MAX_VERTEX_ATTRIBS + 1);

    return mAttribute[semanticIndex];
}

int VertexShader::getSemanticIndex(const std::string &attributeName)
{
    if (!attributeName.empty())
    {
        for (int semanticIndex = 0; semanticIndex < MAX_VERTEX_ATTRIBS; semanticIndex++)
        {
            if (mAttribute[semanticIndex].name == attributeName)
            {
                return semanticIndex;
            }
        }
    }

    return -1;
}

void VertexShader::parseAttributes()
{
    if (mHlsl)
    {
        const char *input = strstr(mHlsl, "struct VS_INPUT");

        for (int attributeIndex = 0; *input != '}'; input++)
        {
            char attributeType[100];
            char attributeName[100];
            int semanticIndex;

            int matches = sscanf(input, "%s _%s : TEXCOORD%d;", attributeType, attributeName, &semanticIndex);

            if (matches == 3)
            {
                if (semanticIndex < MAX_VERTEX_ATTRIBS + 1)
                {
                    mAttribute[semanticIndex].type = attributeType;
                    mAttribute[semanticIndex].name = attributeName;
                }
                else
                {
                    break;
                }

                input = strstr(input, ";");
            }
        }
    }
}

FragmentShader::FragmentShader(GLuint handle) : Shader(handle)
{
}

FragmentShader::~FragmentShader()
{
}

GLenum FragmentShader::getType()
{
    return GL_FRAGMENT_SHADER;
}

void FragmentShader::compile()
{
    compileToHLSL(mFragmentCompiler);
}
}
