//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "Program.h"

#include "main.h"
#include "Shader.h"
#include "debug.h"

namespace gl
{
Uniform::Uniform(UniformType type, const std::string &name, unsigned int bytes) : type(type), name(name), bytes(bytes)
{
    this->data = new unsigned char[bytes];
    memset(this->data, 0, bytes);
}

Uniform::~Uniform()
{
    delete[] data;
}

Program::Program()
{
    mFragmentShader = NULL;
    mVertexShader = NULL;

    mPixelExecutable = NULL;
    mVertexExecutable = NULL;
    mConstantTablePS = NULL;
    mConstantTableVS = NULL;

    for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
    {
        mAttributeName[index] = NULL;
    }

    unlink();

    mDeleteStatus = false;
}

Program::~Program()
{
    unlink(true);
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
        delete[] mAttributeName[index];
        mAttributeName[index] = new char[strlen(name) + 1];
        strcpy(mAttributeName[index], name);
    }
}

GLuint Program::getAttributeLocation(const char *name)
{
    for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
    {
        if (mAttributeName[index] && strcmp(mAttributeName[index], name) == 0)
        {
            return index;
        }
    }

    return -1;
}

bool Program::isActiveAttribute(int attributeIndex)
{
    if (attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS)
    {
        return mInputMapping[attributeIndex] != -1;
    }

    return false;
}

int Program::getInputMapping(int attributeIndex)
{
    if (attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS)
    {
        return mInputMapping[attributeIndex];
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
    for (unsigned int location = 0; location < mUniforms.size(); location++)
    {
        if (mUniforms[location]->name == name)
        {
            return location;
        }
    }

    return -1;
}

bool Program::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_1FV || mUniforms[location]->bytes < sizeof(GLfloat) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, v, sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_2FV || mUniforms[location]->bytes < 2 * sizeof(GLfloat) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, v, 2 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_3FV || mUniforms[location]->bytes < 3 * sizeof(GLfloat) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, v, 3 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_4FV || mUniforms[location]->bytes < 4 * sizeof(GLfloat) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, v, 4 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniformMatrix2fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_MATRIX_2FV || mUniforms[location]->bytes < 4 * sizeof(GLfloat) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, value, 4 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniformMatrix3fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_MATRIX_3FV || mUniforms[location]->bytes < 9 * sizeof(GLfloat) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, value, 9 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniformMatrix4fv(GLint location, GLsizei count, const GLfloat *value)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_MATRIX_4FV || mUniforms[location]->bytes <  16 * sizeof(GLfloat) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, value, 16 * sizeof(GLfloat) * count);

    return true;
}

bool Program::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniforms.size())
    {
        return false;
    }

    if (mUniforms[location]->type != UNIFORM_1IV || mUniforms[location]->bytes < sizeof(GLint) * count)
    {
        return false;
    }

    memcpy(mUniforms[location]->data, v, sizeof(GLint) * count);

    return true;
}

// Applies all the uniforms set for this program object to the Direct3D 9 device
void Program::applyUniforms()
{
    for (unsigned int location = 0; location < mUniforms.size(); location++)
    {
        int bytes = mUniforms[location]->bytes;
        GLfloat *f = (GLfloat*)mUniforms[location]->data;
        GLint *i = (GLint*)mUniforms[location]->data;

        switch (mUniforms[location]->type)
        {
          case UNIFORM_1FV:        applyUniform1fv(location, bytes / sizeof(GLfloat), f);            break;
          case UNIFORM_2FV:        applyUniform2fv(location, bytes / 2 / sizeof(GLfloat), f);        break;
          case UNIFORM_3FV:        applyUniform3fv(location, bytes / 3 / sizeof(GLfloat), f);        break;
          case UNIFORM_4FV:        applyUniform4fv(location, bytes / 4 / sizeof(GLfloat), f);        break;
          case UNIFORM_MATRIX_2FV: applyUniformMatrix2fv(location, bytes / 4 / sizeof(GLfloat), f);  break;
          case UNIFORM_MATRIX_3FV: applyUniformMatrix3fv(location, bytes / 9 / sizeof(GLfloat), f);  break;
          case UNIFORM_MATRIX_4FV: applyUniformMatrix4fv(location, bytes / 16 / sizeof(GLfloat), f); break;
          case UNIFORM_1IV:        applyUniform1iv(location, bytes / sizeof(GLint), i);              break;
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
    DWORD flags = D3DXSHADER_USE_LEGACY_D3DX9_31_DLL |
                  D3DXSHADER_PREFER_FLOW_CONTROL;

    HRESULT result = D3DXCompileShader(hlsl, (UINT)strlen(hlsl), NULL, 0, "main", profile, flags, &binary, &errorMessage, constantTable);

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
        trace(hlsl);
        trace(message);
    }

    return NULL;
}

// Links the HLSL code of the vertex and pixel shader by matching up their varyings,
// compiling them into binaries, determining the attribute mappings, and collecting
// a list of uniforms
void Program::link()
{
    if (mLinked)
    {
        return;
    }

    unlink();

    if (!mFragmentShader || !mFragmentShader->isCompiled())
    {
        return;
    }

    if (!mVertexShader || !mVertexShader->isCompiled())
    {
        return;
    }

    const char *pixelHLSL = mFragmentShader->linkHLSL();
    const char *vertexHLSL = mVertexShader->linkHLSL(pixelHLSL);
    ID3DXBuffer *vertexBinary = compileToBinary(vertexHLSL, "vs_3_0", &mConstantTableVS);
    ID3DXBuffer *pixelBinary = compileToBinary(pixelHLSL, "ps_3_0", &mConstantTablePS);

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

            for (int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++)
            {
                mSamplers[i].active = false;
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
    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        const char *name = mVertexShader->getAttributeName(attributeIndex);

        if (name)
        {
            GLuint location = getAttributeLocation(name);

            if (location == -1)   // Not set by glBindAttribLocation
            {
                int availableIndex = 0;

                while (availableIndex < MAX_VERTEX_ATTRIBS && mAttributeName[availableIndex] && mVertexShader->isActiveAttribute(mAttributeName[availableIndex]))
                {
                    availableIndex++;
                }

                if (availableIndex == MAX_VERTEX_ATTRIBS)
                {
                    return false;   // Fail to link
                }

                delete[] mAttributeName[availableIndex];
                mAttributeName[availableIndex] = new char[strlen(name) + 1];   // FIXME: Check allocation
                strcpy(mAttributeName[availableIndex], name);
            }
        }
    }

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        mInputMapping[attributeIndex] = mVertexShader->getInputMapping(mAttributeName[attributeIndex]);
    }

    return true;
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
    UniformType type = uniform->type;

    if (location >= 0)
    {
        delete uniform;

        if (mUniforms[location]->type != type)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    mUniforms.push_back(uniform);

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
          case D3DXPT_BOOL:
            switch (constantDescription.Columns)
            {
              case 1: return new Uniform(UNIFORM_1IV, name, 1 * sizeof(GLint) * constantDescription.Elements);
              default:
                UNIMPLEMENTED();   // FIXME
                UNREACHABLE();
            }
            break;
          case D3DXPT_FLOAT:
            switch (constantDescription.Columns)
            {
              case 1: return new Uniform(UNIFORM_1FV, name, 1 * sizeof(GLfloat) * constantDescription.Elements);
              case 2: return new Uniform(UNIFORM_2FV, name, 2 * sizeof(GLfloat) * constantDescription.Elements);
              case 3: return new Uniform(UNIFORM_3FV, name, 3 * sizeof(GLfloat) * constantDescription.Elements);
              case 4: return new Uniform(UNIFORM_4FV, name, 4 * sizeof(GLfloat) * constantDescription.Elements);
              default: UNREACHABLE();
            }
            break;
          default:
            UNIMPLEMENTED();   // FIXME
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
              case 2: return new Uniform(UNIFORM_MATRIX_2FV, name, 2 * 2 * sizeof(GLfloat) * constantDescription.Elements);
              case 3: return new Uniform(UNIFORM_MATRIX_3FV, name, 3 * 3 * sizeof(GLfloat) * constantDescription.Elements);
              case 4: return new Uniform(UNIFORM_MATRIX_4FV, name, 4 * 4 * sizeof(GLfloat) * constantDescription.Elements);
              default: UNREACHABLE();
            }
            break;
          default: UNREACHABLE();
        }
    }
    else UNREACHABLE();

    return 0;
}

bool Program::applyUniform1fv(GLint location, GLsizei count, const GLfloat *v)
{
    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
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

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
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

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
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
    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
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

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetMatrixArray(device, constantPS, matrix, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetMatrixArray(device, constantVS, matrix, count);
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

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetMatrixArray(device, constantPS, matrix, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetMatrixArray(device, constantVS, matrix, count);
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

    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
    IDirect3DDevice9 *device = getDevice();

    if (constantPS)
    {
        mConstantTablePS->SetMatrixArray(device, constantPS, matrix, count);
    }

    if (constantVS)
    {
        mConstantTableVS->SetMatrixArray(device, constantVS, matrix, count);
    }

    delete[] matrix;

    return true;
}

bool Program::applyUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    D3DXHANDLE constantPS = mConstantTablePS->GetConstantByName(0, mUniforms[location]->name.c_str());
    D3DXHANDLE constantVS = mConstantTableVS->GetConstantByName(0, mUniforms[location]->name.c_str());
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

        for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
        {
            delete[] mAttributeName[index];
            mAttributeName[index] = NULL;
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
        mInputMapping[index] = 0;
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

    mLinked = false;
}

bool Program::isLinked()
{
    return mLinked;
}

void Program::flagForDeletion()
{
    mDeleteStatus = true;
}

bool Program::isFlaggedForDeletion() const
{
    return mDeleteStatus;
}
}
