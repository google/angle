//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Shader.cpp: Implements the gl::Shader class and its  derived classes
// VertexShader and FragmentShader. Implements GL shader objects and related
// functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section 3.8 page 84.

#include "Shader.h"

#include "main.h"
#include "Shaderlang.h"
#include "OutputHLSL.h"
#include "debug.h"

namespace gl
{
void *Shader::mFragmentCompiler = NULL;
void *Shader::mVertexCompiler = NULL;

Shader::Shader()
{
    mSource = NULL;
    mHlsl = NULL;
    mErrors = NULL;

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
    delete[] mErrors;
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

bool Shader::isCompiled()
{
    return mHlsl != NULL;
}

const char *Shader::linkHLSL()
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

    delete[] mErrors;
    mErrors = NULL;

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
        mErrors = new char[strlen(info) + 1];
        strcpy(mErrors, info);

        TRACE("\n%s", mErrors);
    }
}

InputMapping::InputMapping()
{
    mAttribute = NULL;
    mSemanticIndex = -1;
}

InputMapping::InputMapping(const char *attribute, int semanticIndex)
{
    mAttribute = new char[strlen(attribute) + 1];
    strcpy(mAttribute, attribute);
    mSemanticIndex = semanticIndex;
}

InputMapping &InputMapping::operator=(const InputMapping &inputMapping)
{
    delete[] mAttribute;

    mAttribute = new char[strlen(inputMapping.mAttribute) + 1];
    strcpy(mAttribute, inputMapping.mAttribute);
    mSemanticIndex = inputMapping.mSemanticIndex;

    return *this;
}

InputMapping::~InputMapping()
{
    delete[] mAttribute;
}

VertexShader::VertexShader()
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

const char *VertexShader::linkHLSL(const char *pixelHLSL)
{
    if (mHlsl && pixelHLSL)
    {
        const char *input = strstr(pixelHLSL, "struct PS_INPUT");
        char *output = strstr(mHlsl, "struct VS_OUTPUT");

        while (*input != '}' && output)
        {
            char varyingName[100];
            unsigned int semanticIndex;
            int matches = sscanf(input, "%s : TEXCOORD%d;", varyingName, &semanticIndex);

            if (matches == 2 && semanticIndex != sh::HLSL_FRAG_COORD_SEMANTIC)
            {
                ASSERT(semanticIndex < MAX_VARYING_VECTORS);
                char *varying = strstr(output, varyingName);

                if (varying)
                {
                    ASSERT(semanticIndex <= 9);   // Single character
                    varying = strstr(varying, " : TEXCOORD0;");
                    varying[11] = '0' + semanticIndex;
                }
                else
                {
                    return NULL;
                }

                input = strstr(input, ";");
            }

            input++;
        }
    }

    return mHlsl;
}

const char *VertexShader::getAttributeName(unsigned int attributeIndex)
{
    if (attributeIndex < MAX_VERTEX_ATTRIBS)
    {
        return mInputMapping[attributeIndex].mAttribute;
    }

    return 0;
}

bool VertexShader::isActiveAttribute(const char *attributeName)
{
    return getInputMapping(attributeName) != -1;
}

int VertexShader::getInputMapping(const char *attributeName)
{
    if (attributeName)
    {
        for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
        {
            if (mInputMapping[index].mAttribute && strcmp(mInputMapping[index].mAttribute, attributeName) == 0)
            {
                return mInputMapping[index].mSemanticIndex;
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
            char attributeName[100];
            int semanticIndex;

            int matches = sscanf(input, "%s : TEXCOORD%d;", attributeName, &semanticIndex);

            if (matches == 2)
            {
                ASSERT(attributeIndex < MAX_VERTEX_ATTRIBS);
                mInputMapping[attributeIndex] = InputMapping(attributeName, semanticIndex);
                attributeIndex++;

                input = strstr(input, ";");
            }
        }
    }
}

FragmentShader::FragmentShader()
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
