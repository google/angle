//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "libGLESv2/Program.h"
#include "libGLESv2/ProgramBinary.h"

#include "common/debug.h"

#include "libGLESv2/main.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/utilities.h"

#include <string>

namespace gl
{
unsigned int Program::mCurrentSerial = 1;

AttributeBindings::AttributeBindings()
{
}

AttributeBindings::~AttributeBindings()
{
}

Program::Program(ResourceManager *manager, GLuint handle) : mResourceManager(manager), mHandle(handle), mSerial(issueSerial())
{
    mFragmentShader = NULL;
    mVertexShader = NULL;
    mProgramBinary = NULL;
    mDeleteStatus = false;
    mRefCount = 0;
}

Program::~Program()
{
    unlink(true);

    if (mVertexShader != NULL)
    {
        mVertexShader->release();
    }

    if (mFragmentShader != NULL)
    {
        mFragmentShader->release();
    }
}

bool Program::attachShader(Shader *shader)
{
    if (shader->getType() == GL_VERTEX_SHADER)
    {
        if (mVertexShader)
        {
            return false;
        }

        mVertexShader = (VertexShader*)shader;
        mVertexShader->addRef();
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mFragmentShader)
        {
            return false;
        }

        mFragmentShader = (FragmentShader*)shader;
        mFragmentShader->addRef();
    }
    else UNREACHABLE();

    return true;
}

bool Program::detachShader(Shader *shader)
{
    if (shader->getType() == GL_VERTEX_SHADER)
    {
        if (mVertexShader != shader)
        {
            return false;
        }

        mVertexShader->release();
        mVertexShader = NULL;
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mFragmentShader != shader)
        {
            return false;
        }

        mFragmentShader->release();
        mFragmentShader = NULL;
    }
    else UNREACHABLE();

    return true;
}

int Program::getAttachedShadersCount() const
{
    return (mVertexShader ? 1 : 0) + (mFragmentShader ? 1 : 0);
}

void AttributeBindings::bindAttributeLocation(GLuint index, const char *name)
{
    if (index < MAX_VERTEX_ATTRIBS)
    {
        for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
        {
            mAttributeBinding[i].erase(name);
        }

        mAttributeBinding[index].insert(name);
    }
}

void Program::bindAttributeLocation(GLuint index, const char *name)
{
    mAttributeBindings.bindAttributeLocation(index, name);
}

// Links the HLSL code of the vertex and pixel shader by matching up their varyings,
// compiling them into binaries, determining the attribute mappings, and collecting
// a list of uniforms
void Program::link()
{
    unlink(false);

    mProgramBinary = new ProgramBinary;
    if (!mProgramBinary->link(mAttributeBindings, mFragmentShader, mVertexShader))
    {
        unlink(false);
    }
}

int AttributeBindings::getAttributeBinding(const std::string &name) const
{
    for (int location = 0; location < MAX_VERTEX_ATTRIBS; location++)
    {
        if (mAttributeBinding[location].find(name) != mAttributeBinding[location].end())
        {
            return location;
        }
    }

    return -1;
}

// Returns the program object to an unlinked state, before re-linking, or at destruction
void Program::unlink(bool destroy)
{
    if (destroy)   // Object being destructed
    {
        if (mFragmentShader)
        {
            mFragmentShader->release();
            mFragmentShader = NULL;
        }

        if (mVertexShader)
        {
            mVertexShader->release();
            mVertexShader = NULL;
        }
    }

    if (mProgramBinary)
    {
        delete mProgramBinary;
        mProgramBinary = NULL;
    }
}

ProgramBinary* Program::getProgramBinary()
{
    return mProgramBinary;
}

void Program::release()
{
    mRefCount--;

    if (mRefCount == 0 && mDeleteStatus)
    {
        mResourceManager->deleteProgram(mHandle);
    }
}

void Program::addRef()
{
    mRefCount++;
}

unsigned int Program::getRefCount() const
{
    return mRefCount;
}

unsigned int Program::getSerial() const
{
    return mSerial;
}

unsigned int Program::issueSerial()
{
    return mCurrentSerial++;
}

int Program::getInfoLogLength() const
{
    if (mProgramBinary)
    {
        return mProgramBinary->getInfoLogLength();
    }
    else
    {
       return 0;
    }
}

void Program::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog)
{
    if (mProgramBinary)
    {
        return mProgramBinary->getInfoLog(bufSize, length, infoLog);
    }
    else
    {
        if (bufSize > 0)
        {
            infoLog[0] = '\0';
        }

        if (length)
        {
            *length = 0;
        }
    }
}

void Program::getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders)
{
    int total = 0;

    if (mVertexShader)
    {
        if (total < maxCount)
        {
            shaders[total] = mVertexShader->getHandle();
        }

        total++;
    }

    if (mFragmentShader)
    {
        if (total < maxCount)
        {
            shaders[total] = mFragmentShader->getHandle();
        }

        total++;
    }

    if (count)
    {
        *count = total;
    }
}

void Program::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    if (mProgramBinary)
    {
        mProgramBinary->getActiveAttribute(index, bufsize, length, size, type, name);
    }
    else
    {
        if (bufsize > 0)
        {
            name[0] = '\0';
        }
        
        if (length)
        {
            *length = 0;
        }

        *type = GL_NONE;
        *size = 1;
    }
}

GLint Program::getActiveAttributeCount()
{
    if (mProgramBinary)
    {
        return mProgramBinary->getActiveAttributeCount();
    }
    else
    {
        return 0;
    }
}

GLint Program::getActiveAttributeMaxLength()
{
    if (mProgramBinary)
    {
        return mProgramBinary->getActiveAttributeMaxLength();
    }
    else
    {
        return 0;
    }
}

void Program::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    if (mProgramBinary)
    {
        return mProgramBinary->getActiveUniform(index, bufsize, length, size, type, name);
    }
    else
    {
        if (bufsize > 0)
        {
            name[0] = '\0';
        }

        if (length)
        {
            *length = 0;
        }

        *size = 0;
        *type = GL_NONE;
    }
}

GLint Program::getActiveUniformCount()
{
    if (mProgramBinary)
    {
        return mProgramBinary->getActiveUniformCount();
    }
    else
    {
        return 0;
    }
}

GLint Program::getActiveUniformMaxLength()
{
    if (mProgramBinary)
    {
        return mProgramBinary->getActiveUniformMaxLength();
    }
    else
    {
        return 0;
    }
}

void Program::flagForDeletion()
{
    mDeleteStatus = true;
}

bool Program::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

bool Program::isValidated() const
{
    if (mProgramBinary)
    {
        return mProgramBinary->isValidated();
    }
    else
    {
        return false;
    }
}

}
