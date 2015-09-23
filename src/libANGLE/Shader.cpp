//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Shader.cpp: Implements the gl::Shader class and its  derived classes
// VertexShader and FragmentShader. Implements GL shader objects and related
// functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section 3.8 page 84.

#include "libANGLE/Shader.h"

#include <sstream>

#include "common/utilities.h"
#include "GLSLANG/ShaderLang.h"
#include "libANGLE/Constants.h"
#include "libANGLE/renderer/Renderer.h"
#include "libANGLE/renderer/ShaderImpl.h"
#include "libANGLE/ResourceManager.h"

namespace gl
{

Shader::Data::Data(GLenum shaderType) : mShaderType(shaderType), mShaderVersion(100)
{
}

Shader::Data::~Data()
{
}

Shader::Shader(ResourceManager *manager, rx::ImplFactory *implFactory, GLenum type, GLuint handle)
    : mData(type),
      mImplementation(implFactory->createShader(&mData)),
      mHandle(handle),
      mType(type),
      mRefCount(0),
      mDeleteStatus(false),
      mCompiled(false),
      mResourceManager(manager)
{
    ASSERT(mImplementation);
}

Shader::~Shader()
{
    SafeDelete(mImplementation);
}

GLuint Shader::getHandle() const
{
    return mHandle;
}

void Shader::setSource(GLsizei count, const char *const *string, const GLint *length)
{
    std::ostringstream stream;

    for (int i = 0; i < count; i++)
    {
        if (length == nullptr || length[i] < 0)
        {
            stream.write(string[i], strlen(string[i]));
        }
        else
        {
            stream.write(string[i], length[i]);
        }
    }

    mSource = stream.str();
}

int Shader::getInfoLogLength() const
{
    if (mData.mInfoLog.empty())
    {
        return 0;
    }

    return (static_cast<int>(mData.mInfoLog.length()) + 1);
}

void Shader::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog) const
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(mData.mInfoLog.length()));
        memcpy(infoLog, mData.mInfoLog.c_str(), index);

        infoLog[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

int Shader::getSourceLength() const
{
    return mSource.empty() ? 0 : (static_cast<int>(mSource.length()) + 1);
}

int Shader::getTranslatedSourceLength() const
{
    if (mData.mTranslatedSource.empty())
    {
        return 0;
    }

    return (static_cast<int>(mData.mTranslatedSource.length()) + 1);
}

void Shader::getSourceImpl(const std::string &source, GLsizei bufSize, GLsizei *length, char *buffer)
{
    int index = 0;

    if (bufSize > 0)
    {
        index = std::min(bufSize - 1, static_cast<GLsizei>(source.length()));
        memcpy(buffer, source.c_str(), index);

        buffer[index] = '\0';
    }

    if (length)
    {
        *length = index;
    }
}

void Shader::getSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mSource, bufSize, length, buffer);
}

void Shader::getTranslatedSource(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    getSourceImpl(mData.mTranslatedSource, bufSize, length, buffer);
}

void Shader::getTranslatedSourceWithDebugInfo(GLsizei bufSize, GLsizei *length, char *buffer) const
{
    std::string debugInfo(mImplementation->getDebugInfo());
    getSourceImpl(debugInfo, bufSize, length, buffer);
}

void Shader::compile(Compiler *compiler)
{
    mData.mTranslatedSource.clear();
    mData.mInfoLog.clear();
    mData.mShaderVersion = 100;
    mData.mVaryings.clear();
    mData.mUniforms.clear();
    mData.mInterfaceBlocks.clear();
    mData.mActiveAttributes.clear();
    mData.mActiveOutputVariables.clear();

    mCompiled = mImplementation->compile(compiler, mSource, 0);
}

void Shader::addRef()
{
    mRefCount++;
}

void Shader::release()
{
    mRefCount--;

    if (mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteShader(mHandle);
    }
}

unsigned int Shader::getRefCount() const
{
    return mRefCount;
}

bool Shader::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Shader::flagForDeletion()
{
    mDeleteStatus = true;
}

int Shader::getShaderVersion() const
{
    return mData.mShaderVersion;
}

const std::vector<sh::Varying> &Shader::getVaryings() const
{
    return mData.getVaryings();
}

const std::vector<sh::Uniform> &Shader::getUniforms() const
{
    return mData.getUniforms();
}

const std::vector<sh::InterfaceBlock> &Shader::getInterfaceBlocks() const
{
    return mData.getInterfaceBlocks();
}

const std::vector<sh::Attribute> &Shader::getActiveAttributes() const
{
    return mData.getActiveAttributes();
}

const std::vector<sh::OutputVariable> &Shader::getActiveOutputVariables() const
{
    return mData.getActiveOutputVariables();
}

int Shader::getSemanticIndex(const std::string &attributeName) const
{
    if (!attributeName.empty())
    {
        const auto &activeAttributes = mData.getActiveAttributes();

        int semanticIndex = 0;
        for (size_t attributeIndex = 0; attributeIndex < activeAttributes.size(); attributeIndex++)
        {
            const sh::ShaderVariable &attribute = activeAttributes[attributeIndex];

            if (attribute.name == attributeName)
            {
                return semanticIndex;
            }

            semanticIndex += gl::VariableRegisterCount(attribute.type);
        }
    }

    return -1;
}

}
