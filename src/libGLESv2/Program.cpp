//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "libGLESv2/Program.h"

#include "common/debug.h"

#include "libGLESv2/main.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/utilities.h"

namespace gl
{
Uniform::Uniform(GLenum type, const std::string &name, unsigned int arraySize) : type(type), name(name), arraySize(arraySize)
{
    int bytes = UniformTypeSize(type) * arraySize;

    this->data = new unsigned char[bytes];
    memset(this->data, 0, bytes);
}

Uniform::~Uniform()
{
    delete[] data;
}

UniformLocation::UniformLocation(const std::string &name, unsigned int element, unsigned int index) 
    : name(name), element(element), index(index)
{
}

Program::Program()
{
    mFragmentShader = NULL;
    mVertexShader = NULL;

    mPixelExecutable = NULL;
    mVertexExecutable = NULL;
    mConstantTablePS = NULL;
    mConstantTableVS = NULL;

    mPixelHLSL = NULL;
    mVertexHLSL = NULL;
    mInfoLog = NULL;
    mValidated = false;

    unlink();

    mDeleteStatus = false;
}

Program::~Program()
{
    unlink(true);

    if (mVertexShader != NULL)
    {
        mVertexShader->detach();
    }

    if (mFragmentShader != NULL)
    {
        mFragmentShader->detach();
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
        mVertexShader->attach();
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mFragmentShader)
        {
            return false;
        }

        mFragmentShader = (FragmentShader*)shader;
        mFragmentShader->attach();
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

        mVertexShader->detach();
        mVertexShader = NULL;
    }
    else if (shader->getType() == GL_FRAGMENT_SHADER)
    {
        if (mFragmentShader != shader)
        {
            return false;
        }

        mFragmentShader->detach();
        mFragmentShader = NULL;
    }
    else UNREACHABLE();

    unlink();

    return true;
}

int Program::getAttachedShadersCount() const
{
    return (mVertexShader ? 1 : 0) + (mFragmentShader ? 1 : 0);
}

IDirect3DPixelShader9 *Program::getPixelShader()
{
    return mPixelExecutable;
}

IDirect3DVertexShader9 *Program::getVertexShader()
{
    return mVertexExecutable;
}

void Program::bindAttributeLocation(GLuint index, const char *name)
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

GLuint Program::getAttributeLocation(const char *name)
{
    if (name)
    {
        for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
        {
            if (mLinkedAttribute[index].name == std::string(name))
            {
                return index;
            }
        }
    }

    return -1;
}

int Program::getSemanticIndex(int attributeIndex)
{
    if (attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS)
    {
        return mSemanticIndex[attributeIndex];
    }

    return -1;
}

// Returns the index of the texture unit corresponding to a Direct3D 9 sampler
// index referenced in the compiled HLSL shader
GLint Program::getSamplerMapping(unsigned int samplerIndex)
{
    assert(samplerIndex < sizeof(mSamplers)/sizeof(mSamplers[0]));

    if (mSamplers[samplerIndex].active)
    {
        return mSamplers[samplerIndex].logicalTextureUnit;
    }

    return -1;
}

SamplerType Program::getSamplerType(unsigned int samplerIndex)
{
    assert(samplerIndex < sizeof(mSamplers)/sizeof(mSamplers[0]));
    assert(mSamplers[samplerIndex].active);

    return mSamplers[samplerIndex].type;
}

GLint Program::getUniformLocation(const char *name)
{
    std::string nameStr(name);
    int subscript = 0;
    size_t beginB = nameStr.find('[');
    size_t endB = nameStr.find(']');
    if (beginB != std::string::npos && endB != std::string::npos)
    {
        std::string subscrStr = nameStr.substr(beginB + 1, beginB - endB - 1);
        nameStr.erase(beginB);
        subscript = atoi(subscrStr.c_str());
    }

    for (unsigned int location = 0; location < mUniformIndex.size(); location++)
    {
        if (mUniformIndex[location].name == decorate(nameStr) &&
            mUniformIndex[location].element == subscript)
        {
            return location;
        }
    }

    return -1;
}

bool Program::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_FLOAT)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat),
               v, sizeof(GLfloat) * count);
    }
    else if (targetUniform->type == GL_BOOL)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);
        GLboolean *boolParams = new GLboolean[count];

        for (int i = 0; i < count; ++i)
        {
            if (v[i] == 0.0f)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean),
               boolParams, sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else
    {
        return false;
    }

    return true;
}

bool Program::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_FLOAT_VEC2)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * 2,
               v, 2 * sizeof(GLfloat) * count);
    }
    else if (targetUniform->type == GL_BOOL_VEC2)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        GLboolean *boolParams = new GLboolean[count * 2];

        for (int i = 0; i < count * 2; ++i)
        {
            if (v[i] == 0.0f)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean) * 2,
               boolParams, 2 * sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else 
    {
        return false;
    }

    return true;
}

bool Program::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_FLOAT_VEC3)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * 3,
               v, 3 * sizeof(GLfloat) * count);
    }
    else if (targetUniform->type == GL_BOOL_VEC3)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);
        GLboolean *boolParams = new GLboolean[count * 3];

        for (int i = 0; i < count * 3; ++i)
        {
            if (v[i] == 0.0f)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean) * 3,
               boolParams, 3 * sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else 
    {
        return false;
    }

    return true;
}

bool Program::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_FLOAT_VEC4)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * 4,
               v, 4 * sizeof(GLfloat) * count);
    }
    else if (targetUniform->type == GL_BOOL_VEC4)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);
        GLboolean *boolParams = new GLboolean[count * 4];

        for (int i = 0; i < count * 4; ++i)
        {
            if (v[i] == 0.0f)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean) * 4,
               boolParams, 4 * sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else 
    {
        return false;
    }

    return true;
}

bool Program::setUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type != GL_FLOAT_MAT2)
    {
        return false;
    }

    int arraySize = targetUniform->arraySize;

    if (arraySize == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(arraySize - (int)mUniformIndex[location].element, count);

    memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * 4,
           value, 4 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type != GL_FLOAT_MAT3)
    {
        return false;
    }

    int arraySize = targetUniform->arraySize;

    if (arraySize == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(arraySize - (int)mUniformIndex[location].element, count);

    memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * 9,
           value, 9 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type != GL_FLOAT_MAT4)
    {
        return false;
    }

    int arraySize = targetUniform->arraySize;

    if (arraySize == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(arraySize - (int)mUniformIndex[location].element, count);

    memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * 16,
           value, 16 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_INT)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLint),
               v, sizeof(GLint) * count);
    }
    else if (targetUniform->type == GL_BOOL)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);
        GLboolean *boolParams = new GLboolean[count];

        for (int i = 0; i < count; ++i)
        {
            if (v[i] == 0)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean),
               boolParams, sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else
    {
        return false;
    }

    return true;
}

bool Program::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_INT_VEC2)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLint) * 2,
               v, 2 * sizeof(GLint) * count);
    }
    else if (targetUniform->type == GL_BOOL_VEC2)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);
        GLboolean *boolParams = new GLboolean[count * 2];

        for (int i = 0; i < count * 2; ++i)
        {
            if (v[i] == 0)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean) * 2,
               boolParams, 2 * sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else
    {
        return false;
    }

    return true;
}

bool Program::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_INT_VEC3)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLint) * 3,
               v, 3 * sizeof(GLint) * count);
    }
    else if (targetUniform->type == GL_BOOL_VEC3)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);
        GLboolean *boolParams = new GLboolean[count * 3];

        for (int i = 0; i < count * 3; ++i)
        {
            if (v[i] == 0)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean) * 3,
               boolParams, 3 * sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else
    {
        return false;
    }

    return true;
}

bool Program::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (targetUniform->type == GL_INT_VEC4)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLint) * 4,
               v, 4 * sizeof(GLint) * count);
    }
    else if (targetUniform->type == GL_BOOL_VEC4)
    {
        int arraySize = targetUniform->arraySize;

        if (arraySize == 1 && count > 1)
            return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

        count = std::min(arraySize - (int)mUniformIndex[location].element, count);
        GLboolean *boolParams = new GLboolean[count * 4];

        for (int i = 0; i < count * 4; ++i)
        {
            if (v[i] == 0)
            {
                boolParams[i] = GL_FALSE;
            }
            else
            {
                boolParams[i] = GL_TRUE;
            }
        }

        memcpy(targetUniform->data + mUniformIndex[location].element * sizeof(GLboolean) * 4,
               boolParams, 4 * sizeof(GLboolean) * count);

        delete [] boolParams;
    }
    else
    {
        return false;
    }

    return true;
}

bool Program::getUniformfv(GLint location, GLfloat *params)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    unsigned int count = UniformComponentCount(targetUniform->type);

    switch (UniformComponentType(targetUniform->type))
    {
      case GL_BOOL:
        {
            GLboolean *boolParams = (GLboolean*)targetUniform->data + mUniformIndex[location].element * count;

            for (unsigned int i = 0; i < count; ++i)
            {
                params[i] = (boolParams[i] == GL_FALSE) ? 0.0f : 1.0f;
            }
        }
        break;
      case GL_FLOAT:
        memcpy(params, targetUniform->data + mUniformIndex[location].element * count * sizeof(GLfloat),
               count * sizeof(GLfloat));
        break;
      case GL_INT:
        {
            GLint *intParams = (GLint*)targetUniform->data + mUniformIndex[location].element * count;

            for (unsigned int i = 0; i < count; ++i)
            {
                params[i] = (float)intParams[i];
            }
        }
        break;
      default: UNREACHABLE();
    }

    return true;
}

bool Program::getUniformiv(GLint location, GLint *params)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    unsigned int count = UniformComponentCount(targetUniform->type);

    switch (UniformComponentType(targetUniform->type))
    {
      case GL_BOOL:
        {
            GLboolean *boolParams = targetUniform->data + mUniformIndex[location].element * count;

            for (unsigned int i = 0; i < count; ++i)
            {
                params[i] = (GLint)boolParams[i];
            }
        }
        break;
      case GL_FLOAT:
        {
            GLfloat *floatParams = (GLfloat*)targetUniform->data + mUniformIndex[location].element * count;

            for (unsigned int i = 0; i < count; ++i)
            {
                params[i] = (GLint)floatParams[i];
            }
        }
        break;
      case GL_INT:
        memcpy(params, targetUniform->data + mUniformIndex[location].element * count * sizeof(GLint),
               count * sizeof(GLint));
        break;
      default: UNREACHABLE();
    }

    return true;
}

// Applies all the uniforms set for this program object to the Direct3D 9 device
void Program::applyUniforms()
{
    for (unsigned int location = 0; location < mUniformIndex.size(); location++)
    {
        if (mUniformIndex[location].element != 0)
        {
            continue;
        }

        Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

        int arraySize = targetUniform->arraySize;
        GLfloat *f = (GLfloat*)targetUniform->data;
        GLint *i = (GLint*)targetUniform->data;
        GLboolean *b = (GLboolean*)targetUniform->data;

        switch (targetUniform->type)
        {
          case GL_BOOL:       applyUniform1bv(location, arraySize, b);       break;
          case GL_BOOL_VEC2:  applyUniform2bv(location, arraySize, b);       break;
          case GL_BOOL_VEC3:  applyUniform3bv(location, arraySize, b);       break;
          case GL_BOOL_VEC4:  applyUniform4bv(location, arraySize, b);       break;
          case GL_FLOAT:      applyUniform1fv(location, arraySize, f);       break;
          case GL_FLOAT_VEC2: applyUniform2fv(location, arraySize, f);       break;
          case GL_FLOAT_VEC3: applyUniform3fv(location, arraySize, f);       break;
          case GL_FLOAT_VEC4: applyUniform4fv(location, arraySize, f);       break;
          case GL_FLOAT_MAT2: applyUniformMatrix2fv(location, arraySize, f); break;
          case GL_FLOAT_MAT3: applyUniformMatrix3fv(location, arraySize, f); break;
          case GL_FLOAT_MAT4: applyUniformMatrix4fv(location, arraySize, f); break;
          case GL_INT:        applyUniform1iv(location, arraySize, i);       break;
          case GL_INT_VEC2:   applyUniform2iv(location, arraySize, i);       break;
          case GL_INT_VEC3:   applyUniform3iv(location, arraySize, i);       break;
          case GL_INT_VEC4:   applyUniform4iv(location, arraySize, i);       break;
          default:
            UNIMPLEMENTED();   // FIXME
            UNREACHABLE();
        }
    }
}

// Compiles the HLSL code of the attached shaders into executable binaries
ID3DXBuffer *Program::compileToBinary(const char *hlsl, const char *profile, ID3DXConstantTable **constantTable)
{
    if (!hlsl)
    {
        return NULL;
    }

    ID3DXBuffer *binary = NULL;
    ID3DXBuffer *errorMessage = NULL;

    HRESULT result = D3DXCompileShader(hlsl, (UINT)strlen(hlsl), NULL, 0, "main", profile, 0, &binary, &errorMessage, constantTable);

    if (SUCCEEDED(result))
    {
        return binary;
    }

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY, (ID3DXBuffer*)NULL);
    }

    if (errorMessage)
    {
        const char *message = (const char*)errorMessage->GetBufferPointer();

        TRACE("\n%s", hlsl);
        TRACE("\n%s", message);
    }

    return NULL;
}

void Program::parseVaryings(const char *structure, char *hlsl, VaryingArray &varyings)
{
    char *input = strstr(hlsl, structure);
    input += strlen(structure);

    while (input && *input != '}')
    {
        char varyingType[256];
        char varyingName[256];
        unsigned int semanticIndex;

        int matches = sscanf(input, "    %s %s : TEXCOORD%d;", varyingType, varyingName, &semanticIndex);

        if (matches == 3)
        {
            ASSERT(semanticIndex <= 9);   // Single character

            char *array = strstr(varyingName, "[");

            if (array)
            {
                *array = '\0';
            }

            varyings.push_back(Varying(varyingName, input));
        }

        input = strstr(input, ";");
        input += 2;
    }
}

bool Program::linkVaryings()
{
    if (!mPixelHLSL || !mVertexHLSL)
    {
        return false;
    }

    VaryingArray vertexVaryings;
    VaryingArray pixelVaryings;

    parseVaryings("struct VS_OUTPUT\n{\n", mVertexHLSL, vertexVaryings);
    parseVaryings("struct PS_INPUT\n{\n", mPixelHLSL, pixelVaryings);

    for (unsigned int out = 0; out < vertexVaryings.size(); out++)
    {
        unsigned int in;
        for (in = 0; in < pixelVaryings.size(); in++)
        {
            if (vertexVaryings[out].name == pixelVaryings[in].name)
            {
                pixelVaryings[in].link = out;
                vertexVaryings[out].link = in;

                break;
            }
        }

        if (in != pixelVaryings.size())
        {
            // FIXME: Verify matching type and qualifiers

            char *outputSemantic = strstr(vertexVaryings[out].declaration, " : TEXCOORD");
            char *inputSemantic = strstr(pixelVaryings[in].declaration, " : TEXCOORD");
            outputSemantic[11] = inputSemantic[11];
        }
        else
        {
            // Comment out the declaration and output assignment
            vertexVaryings[out].declaration[0] = '/';
            vertexVaryings[out].declaration[1] = '/';

            char outputString[256];
            sprintf(outputString, "    output.%s = ", vertexVaryings[out].name.c_str());
            char *varyingOutput = strstr(mVertexHLSL, outputString);

            varyingOutput[0] = '/';
            varyingOutput[1] = '/';
        }
    }

    // Verify that each pixel varying has been linked to a vertex varying
    for (unsigned int in = 0; in < pixelVaryings.size(); in++)
    {
        if (pixelVaryings[in].link < 0)
        {
            appendToInfoLog("Pixel varying (%s) does not match any vertex varying", pixelVaryings[in].name);

            return false;
        }
    }

    return true;
}

// Links the HLSL code of the vertex and pixel shader by matching up their varyings,
// compiling them into binaries, determining the attribute mappings, and collecting
// a list of uniforms
void Program::link()
{
    unlink();

    if (!mFragmentShader || !mFragmentShader->isCompiled())
    {
        return;
    }

    if (!mVertexShader || !mVertexShader->isCompiled())
    {
        return;
    }

    Context *context = getContext();
    const char *vertexProfile = context->getVertexShaderProfile();
    const char *pixelProfile = context->getPixelShaderProfile();

    const char *ps = mFragmentShader->getHLSL();
    const char *vs = mVertexShader->getHLSL();

    mPixelHLSL = new char[strlen(ps) + 1];
    strcpy(mPixelHLSL, ps);
    mVertexHLSL = new char[strlen(vs) + 1];
    strcpy(mVertexHLSL, vs);

    if (!linkVaryings())
    {
        return;
    }

    ID3DXBuffer *vertexBinary = compileToBinary(mVertexHLSL, vertexProfile, &mConstantTableVS);
    ID3DXBuffer *pixelBinary = compileToBinary(mPixelHLSL, pixelProfile, &mConstantTablePS);

    if (vertexBinary && pixelBinary)
    {
        IDirect3DDevice9 *device = getDevice();
        HRESULT vertexResult = device->CreateVertexShader((DWORD*)vertexBinary->GetBufferPointer(), &mVertexExecutable);
        HRESULT pixelResult = device->CreatePixelShader((DWORD*)pixelBinary->GetBufferPointer(), &mPixelExecutable);

        if (vertexResult == D3DERR_OUTOFVIDEOMEMORY || vertexResult == E_OUTOFMEMORY || pixelResult == D3DERR_OUTOFVIDEOMEMORY || pixelResult == E_OUTOFMEMORY)
        {
            return error(GL_OUT_OF_MEMORY);
        }

        ASSERT(SUCCEEDED(vertexResult) && SUCCEEDED(pixelResult));

        vertexBinary->Release();
        pixelBinary->Release();
        vertexBinary = NULL;
        pixelBinary = NULL;

        if (mVertexExecutable && mPixelExecutable)
        {
            if (!linkAttributes())
            {
                return;
            }

            if (!linkUniforms(mConstantTablePS))
            {
                return;
            }

            if (!linkUniforms(mConstantTableVS))
            {
                return;
            }

            mLinked = true;   // Success
        }
    }
}

// Determines the mapping between GL attributes and Direct3D 9 vertex stream usage indices
bool Program::linkAttributes()
{
    unsigned int usedLocations = 0;

    // Link attributes that have a binding location
    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        const Attribute &attribute = mVertexShader->getAttribute(attributeIndex);
        int location = getAttributeBinding(attribute.name);

        if (location != -1)   // Set by glBindAttribLocation
        {
            if (!mLinkedAttribute[location].name.empty())
            {
                // Multiple active attributes bound to the same location; not an error
            }

            mLinkedAttribute[location] = attribute;

            int size = AttributeVectorCount(attribute.type);

            if (size + location > MAX_VERTEX_ATTRIBS)
            {
                appendToInfoLog("Active attribute (%s) at location %d is too big to fit", attribute.name.c_str(), location);

                return false;
            }

            for (int i = 0; i < size; i++)
            {
                usedLocations |= 1 << (location + i);
            }
        }
    }

    // Link attributes that don't have a binding location
    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS + 1; attributeIndex++)
    {
        const Attribute &attribute = mVertexShader->getAttribute(attributeIndex);
        int location = getAttributeBinding(attribute.name);

        if (!attribute.name.empty() && location == -1)   // Not set by glBindAttribLocation
        {
            int size = AttributeVectorCount(attribute.type);
            int availableIndex = AllocateFirstFreeBits(&usedLocations, size, MAX_VERTEX_ATTRIBS);

            if (availableIndex == -1 || availableIndex + size > MAX_VERTEX_ATTRIBS)
            {
                appendToInfoLog("Too many active attributes (%s)", attribute.name.c_str());

                return false;   // Fail to link
            }

            mLinkedAttribute[availableIndex] = attribute;
        }
    }

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; )
    {
        int index = mVertexShader->getSemanticIndex(mLinkedAttribute[attributeIndex].name);

        if (index == -1)
        {
            mSemanticIndex[attributeIndex++] = -1;
        }
        else
        {
            int size = AttributeVectorCount(mVertexShader->getAttribute(index).type);

            for (int i = 0; i < size; i++)
            {
                mSemanticIndex[attributeIndex++] = index++;
            }
        }
    }

    return true;
}

int Program::getAttributeBinding(const std::string &name)
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

bool Program::linkUniforms(ID3DXConstantTable *constantTable)
{
    D3DXCONSTANTTABLE_DESC constantTableDescription;
    D3DXCONSTANT_DESC constantDescription;
    UINT descriptionCount = 1;

    constantTable->GetDesc(&constantTableDescription);

    for (unsigned int constantIndex = 0; constantIndex < constantTableDescription.Constants; constantIndex++)
    {
        D3DXHANDLE constantHandle = constantTable->GetConstant(0, constantIndex);
        constantTable->GetConstantDesc(constantHandle, &constantDescription, &descriptionCount);

        if (!defineUniform(constantHandle, constantDescription))
        {
            return false;
        }
    }

    return true;
}

// Adds the description of a constant found in the binary shader to the list of uniforms
// Returns true if succesful (uniform not already defined)
bool Program::defineUniform(const D3DXHANDLE &constantHandle, const D3DXCONSTANT_DESC &constantDescription, std::string name)
{
    if (constantDescription.RegisterSet == D3DXRS_SAMPLER)
    {
        unsigned int samplerIndex = constantDescription.RegisterIndex;

        assert(samplerIndex < sizeof(mSamplers)/sizeof(mSamplers[0]));

        mSamplers[samplerIndex].active = true;
        mSamplers[samplerIndex].type = (constantDescription.Type == D3DXPT_SAMPLERCUBE) ? SAMPLER_CUBE : SAMPLER_2D;
        mSamplers[samplerIndex].logicalTextureUnit = 0;
    }

    switch(constantDescription.Class)
    {
      case D3DXPC_STRUCT:
        {
            for (unsigned int field = 0; field < constantDescription.StructMembers; field++)
            {
                D3DXHANDLE fieldHandle = mConstantTablePS->GetConstant(constantHandle, field);

                D3DXCONSTANT_DESC fieldDescription;
                UINT descriptionCount = 1;

                mConstantTablePS->GetConstantDesc(fieldHandle, &fieldDescription, &descriptionCount);

                if (!defineUniform(fieldHandle, fieldDescription, name + constantDescription.Name + "."))
                {
                    return false;
                }
            }

            return true;
        }
      case D3DXPC_SCALAR:
      case D3DXPC_VECTOR:
      case D3DXPC_MATRIX_COLUMNS:
      case D3DXPC_OBJECT:
        return defineUniform(constantDescription, name + constantDescription.Name);
      default:
        UNREACHABLE();
        return false;
    }
}

bool Program::defineUniform(const D3DXCONSTANT_DESC &constantDescription, std::string &name)
{
    Uniform *uniform = createUniform(constantDescription, name);

    if(!uniform)
    {
        return false;
    }

    // Check if already defined
    GLint location = getUniformLocation(name.c_str());
    GLenum type = uniform->type;

    if (location >= 0)
    {
        delete uniform;

        if (mUniforms[mUniformIndex[location].index]->type != type)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    mUniforms.push_back(uniform);
    unsigned int uniformIndex = mUniforms.size() - 1;

    for (unsigned int i = 0; i < uniform->arraySize; ++i)
    {
        mUniformIndex.push_back(UniformLocation(name, i, uniformIndex));
    }

    return true;
}

Uniform *Program::createUniform(const D3DXCONSTANT_DESC &constantDescription, std::string &name)
{
    if (constantDescription.Rows == 1)   // Vectors and scalars
    {
        switch (constantDescription.Type)
        {
          case D3DXPT_SAMPLER2D:
          case D3DXPT_SAMPLERCUBE:
            switch (constantDescription.Columns)
            {
              case 1: return new Uniform(GL_INT, name, constantDescription.Elements);
              default: UNREACHABLE();
            }
            break;
          case D3DXPT_BOOL:
            switch (constantDescription.Columns)
            {
              case 1: return new Uniform(GL_BOOL, name, constantDescription.Elements);
              case 2: return new Uniform(GL_BOOL_VEC2, name, constantDescription.Elements);
              case 3: return new Uniform(GL_BOOL_VEC3, name, constantDescription.Elements);
              case 4: return new Uniform(GL_BOOL_VEC4, name, constantDescription.Elements);
              default: UNREACHABLE();
            }
            break;
          case D3DXPT_INT:
            switch (constantDescription.Columns)
            {
              case 1: return new Uniform(GL_INT, name, constantDescription.Elements);
              case 2: return new Uniform(GL_INT_VEC2, name, constantDescription.Elements);
              case 3: return new Uniform(GL_INT_VEC3, name, constantDescription.Elements);
              case 4: return new Uniform(GL_INT_VEC4, name, constantDescription.Elements);
              default: UNREACHABLE();
            }
            break;
          case D3DXPT_FLOAT:
            switch (constantDescription.Columns)
            {
              case 1: return new Uniform(GL_FLOAT, name, constantDescription.Elements);
              case 2: return new Uniform(GL_FLOAT_VEC2, name, constantDescription.Elements);
              case 3: return new Uniform(GL_FLOAT_VEC3, name, constantDescription.Elements);
              case 4: return new Uniform(GL_FLOAT_VEC4, name, constantDescription.Elements);
              default: UNREACHABLE();
            }
            break;
          default:
            UNREACHABLE();
        }
    }
    else if (constantDescription.Rows == constantDescription.Columns)  // Square matrices
    {
        switch (constantDescription.Type)
        {
          case D3DXPT_FLOAT:
            switch (constantDescription.Rows)
            {
              case 2: return new Uniform(GL_FLOAT_MAT2, name, constantDescription.Elements);
              case 3: return new Uniform(GL_FLOAT_MAT3, name, constantDescription.Elements);
              case 4: return new Uniform(GL_FLOAT_MAT4, name, constantDescription.Elements);
              default: UNREACHABLE();
            }
            break;
          default: UNREACHABLE();
        }
    }
    else UNREACHABLE();

    return 0;
}

// This method needs to match OutputHLSL::decorate
std::string Program::decorate(const std::string &string)
{
    if (string.substr(0, 3) != "gl_" && string.substr(0, 3) != "dx_")
    {
        return "_" + string;
    }
    else
    {
        return string;
    }
}

std::string Program::undecorate(const std::string &string)
{
    if (string.substr(0, 1) == "_")
    {
        return string.substr(1);
    }
    else
    {
        return string;
    }
}

bool Program::applyUniform1bv(GLint location, GLsizei count, const GLboolean *v)
{
    BOOL *vector = new BOOL[count];
    for (int i = 0; i < count; i++)
    {
        if (v[i] == GL_FALSE)
            vector[i] = 0;
        else 
            vector[i] = 1;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetBoolArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetBoolArray(device, constantVS, vector, count);
    }

    delete [] vector;

    return true;
}

bool Program::applyUniform2bv(GLint location, GLsizei count, const GLboolean *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4((v[0] == GL_FALSE ? 0.0f : 1.0f),
                                (v[1] == GL_FALSE ? 0.0f : 1.0f), 0, 0);

        v += 2;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete[] vector;

    return true;
}

bool Program::applyUniform3bv(GLint location, GLsizei count, const GLboolean *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4((v[0] == GL_FALSE ? 0.0f : 1.0f),
                                (v[1] == GL_FALSE ? 0.0f : 1.0f), 
                                (v[2] == GL_FALSE ? 0.0f : 1.0f), 0);

        v += 3;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete[] vector;

    return true;
}

bool Program::applyUniform4bv(GLint location, GLsizei count, const GLboolean *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4((v[0] == GL_FALSE ? 0.0f : 1.0f),
                                (v[1] == GL_FALSE ? 0.0f : 1.0f), 
                                (v[2] == GL_FALSE ? 0.0f : 1.0f), 
                                (v[3] == GL_FALSE ? 0.0f : 1.0f));

        v += 3;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete [] vector;

    return true;
}

bool Program::applyUniform1fv(GLint location, GLsizei count, const GLfloat *v)
{
    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetFloatArray(device, constantPS, v, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetFloatArray(device, constantVS, v, count);
    }

    return true;
}

bool Program::applyUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4(v[0], v[1], 0, 0);

        v += 2;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete[] vector;

    return true;
}

bool Program::applyUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4(v[0], v[1], v[2], 0);

        v += 3;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete[] vector;

    return true;
}

bool Program::applyUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, (D3DXVECTOR4*)v, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, (D3DXVECTOR4*)v, count);
    }

    return true;
}

bool Program::applyUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value)
{
    D3DXMATRIX *matrix = new D3DXMATRIX[count];

    for (int i = 0; i < count; i++)
    {
        matrix[i] = D3DXMATRIX(value[0], value[2], 0, 0,
                               value[1], value[3], 0, 0,
                               0,        0,        1, 0,
                               0,        0,        0, 1);

        value += 4;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetMatrixTransposeArray(device, constantPS, matrix, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetMatrixTransposeArray(device, constantVS, matrix, count);
    }

    delete[] matrix;

    return true;
}

bool Program::applyUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value)
{
    D3DXMATRIX *matrix = new D3DXMATRIX[count];

    for (int i = 0; i < count; i++)
    {
        matrix[i] = D3DXMATRIX(value[0], value[3], value[6], 0,
                               value[1], value[4], value[7], 0,
                               value[2], value[5], value[8], 0,
                               0,        0,        0,        1);

        value += 9;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetMatrixTransposeArray(device, constantPS, matrix, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetMatrixTransposeArray(device, constantVS, matrix, count);
    }

    delete[] matrix;

    return true;
}

bool Program::applyUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value)
{
    D3DXMATRIX *matrix = new D3DXMATRIX[count];

    for (int i = 0; i < count; i++)
    {
        matrix[i] = D3DXMATRIX(value[0], value[4], value[8],  value[12],
                               value[1], value[5], value[9],  value[13],
                               value[2], value[6], value[10], value[14],
                               value[3], value[7], value[11], value[15]);

        value += 16;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetMatrixTransposeArray(device, constantPS, matrix, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetMatrixTransposeArray(device, constantVS, matrix, count);
    }

    delete[] matrix;

    return true;
}

bool Program::applyUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        D3DXCONSTANT_DESC constantDescription;
        UINT descriptionCount = 1;
        HRESULT result = mConstantTablePS->GetConstantDesc(constantPS, &constantDescription, &descriptionCount);

        if (FAILED(result))
        {
            return false;
        }

        if (constantDescription.RegisterSet == D3DXRS_SAMPLER)
        {
            unsigned int firstIndex = mConstantTablePS->GetSamplerIndex(constantPS);

            for (unsigned int samplerIndex = firstIndex; samplerIndex < firstIndex + count; samplerIndex++)
            {
                GLint mappedSampler = v[0];

                if (mappedSampler >= 0 && mappedSampler < MAX_TEXTURE_IMAGE_UNITS)
                {
                    if (samplerIndex >= 0 && samplerIndex < MAX_TEXTURE_IMAGE_UNITS)
                    {
                        ASSERT(mSamplers[samplerIndex].active);
                        mSamplers[samplerIndex].logicalTextureUnit = mappedSampler;
                    }
                }
            }

            return true;
        }
    }

    if (constantPS)
    {
        mConstantTablePS->SetIntArray(device, constantPS, v, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetIntArray(device, constantVS, v, count);
    }

    return true;
}

bool Program::applyUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4((float)v[0], (float)v[1], 0, 0);

        v += 2;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete[] vector;

    return true;
}

bool Program::applyUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4((float)v[0], (float)v[1], (float)v[2], 0);

        v += 3;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete[] vector;

    return true;
}

bool Program::applyUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    D3DXVECTOR4 *vector = new D3DXVECTOR4[count];

    for (int i = 0; i < count; i++)
    {
        vector[i] = D3DXVECTOR4((float)v[0], (float)v[1], (float)v[2], (float)v[3]);

        v += 4;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, targetUniform->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, targetUniform->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetVectorArray(device, constantPS, vector, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetVectorArray(device, constantVS, vector, count);
    }

    delete [] vector;

    return true;
}

void Program::appendToInfoLog(const char *format, ...)
{
    if (!format)
    {
        return;
    }

    char info[1024];

    va_list vararg;
    va_start(vararg, format);
    vsnprintf(info, sizeof(info), format, vararg);
    va_end(vararg);

    size_t infoLength = strlen(info);

    if (!mInfoLog)
    {
        mInfoLog = new char[infoLength + 1];
        strcpy(mInfoLog, info);
    }
    else
    {
        size_t logLength = strlen(mInfoLog);
        char *newLog = new char[logLength + infoLength + 1];
        strcpy(newLog, mInfoLog);
        strcpy(newLog + logLength, info);

        delete[] mInfoLog;
        mInfoLog = newLog;
    }
}

void Program::resetInfoLog()
{
    if (mInfoLog)
    {
        delete [] mInfoLog;
    }
}

// Returns the program object to an unlinked state, after detaching a shader, before re-linking, or at destruction
void Program::unlink(bool destroy)
{
    if (destroy)   // Object being destructed
    {
        if (mFragmentShader)
        {
            mFragmentShader->detach();
            mFragmentShader = NULL;
        }

        if (mVertexShader)
        {
            mVertexShader->detach();
            mVertexShader = NULL;
        }
    }

    if (mPixelExecutable)
    {
        mPixelExecutable->Release();
        mPixelExecutable = NULL;
    }

    if (mVertexExecutable)
    {
        mVertexExecutable->Release();
        mVertexExecutable = NULL;
    }

    if (mConstantTablePS)
    {
        mConstantTablePS->Release();
        mConstantTablePS = NULL;
    }

    if (mConstantTableVS)
    {
        mConstantTableVS->Release();
        mConstantTableVS = NULL;
    }

    for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
    {
        mLinkedAttribute[index].name.clear();
        mSemanticIndex[index] = -1;
    }

    for (int index = 0; index < MAX_TEXTURE_IMAGE_UNITS; index++)
    {
        mSamplers[index].active = false;
    }

    while (!mUniforms.empty())
    {
        delete mUniforms.back();
        mUniforms.pop_back();
    }

    mUniformIndex.clear();

    delete[] mPixelHLSL;
    mPixelHLSL = NULL;

    delete[] mVertexHLSL;
    mVertexHLSL = NULL;

    delete[] mInfoLog;
    mInfoLog = NULL;

    mLinked = false;
}

bool Program::isLinked()
{
    return mLinked;
}

bool Program::isValidated() const 
{
    return mValidated;
}

int Program::getInfoLogLength() const
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

void Program::getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog)
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
    unsigned int attribute = 0;
    for (unsigned int i = 0; i < index; i++)
    {
        do
        {
            attribute++;

            ASSERT(attribute < MAX_VERTEX_ATTRIBS);   // index must be smaller than getActiveAttributeCount()
        }
        while (mLinkedAttribute[attribute].name.empty());
    }

    if (bufsize > 0)
    {
        const char *string = mLinkedAttribute[attribute].name.c_str();

        strncpy(name, string, bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = 1;   // Always a single 'type' instance

    *type = mLinkedAttribute[attribute].type;
}

GLint Program::getActiveAttributeCount()
{
    int count = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            count++;
        }
    }

    return count;
}

GLint Program::getActiveAttributeMaxLength()
{
    int maxLength = 0;

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        if (!mLinkedAttribute[attributeIndex].name.empty())
        {
            maxLength = std::max((int)(mLinkedAttribute[attributeIndex].name.length() + 1), maxLength);
        }
    }

    return maxLength;
}

void Program::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name)
{
    unsigned int uniform = 0;
    for (unsigned int i = 0; i < index; i++)
    {
        do
        {
            uniform++;

            ASSERT(uniform < mUniforms.size());   // index must be smaller than getActiveUniformCount()
        }
        while (mUniforms[uniform]->name.substr(0, 3) == "dx_");
    }

    if (bufsize > 0)
    {
        std::string string = undecorate(mUniforms[uniform]->name);

        if (mUniforms[uniform]->arraySize != 1)
        {
            string += "[0]";
        }

        strncpy(name, string.c_str(), bufsize);
        name[bufsize - 1] = '\0';

        if (length)
        {
            *length = strlen(name);
        }
    }

    *size = mUniforms[uniform]->arraySize;

    *type = mUniforms[uniform]->type;
}

GLint Program::getActiveUniformCount()
{
    int count = 0;

    for (unsigned int uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        if (mUniforms[uniformIndex]->name.substr(0, 3) != "dx_")
        {
            count++;
        }
    }

    return count;
}

GLint Program::getActiveUniformMaxLength()
{
    int maxLength = 0;

    for (unsigned int uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        if (!mUniforms[uniformIndex]->name.empty() && mUniforms[uniformIndex]->name.substr(0, 3) != "dx_")
        {
            maxLength = std::max((int)(undecorate(mUniforms[uniformIndex]->name).length() + 1), maxLength);
        }
    }

    return maxLength;
}

void Program::flagForDeletion()
{
    mDeleteStatus = true;
}

bool Program::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}

void Program::validate()
{
    resetInfoLog();

    if (!isLinked()) 
    {
        appendToInfoLog("Program has not been successfully linked.");
        mValidated = false;
    }
    else
    {
        applyUniforms();
        if (!validateSamplers())
        {
            appendToInfoLog("Samplers of conflicting types refer to the same texture image unit.");
            mValidated = false;
        }
        else
        {
            mValidated = true;
        }
    }
}

bool Program::validateSamplers() const
{
    // if any two active samplers in a program are of different types, but refer to the same
    // texture image unit, and this is the current program, then ValidateProgram will fail, and
    // DrawArrays and DrawElements will issue the INVALID_OPERATION error.
    std::map<int, SamplerType> samplerMap; 
    for (unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; ++i)
    {
        if (mSamplers[i].active)
        {
            if (samplerMap.find(mSamplers[i].logicalTextureUnit) != samplerMap.end())
            {
                if (mSamplers[i].type != samplerMap[mSamplers[i].logicalTextureUnit])
                    return false;
            }
            else
            {
                samplerMap[mSamplers[i].logicalTextureUnit] = mSamplers[i].type;
            }
        }
    }

    return true;
}
}
