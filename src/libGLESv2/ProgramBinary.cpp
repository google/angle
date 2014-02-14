#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.cpp: Implements the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#include "libGLESv2/BinaryStream.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/renderer/ShaderExecutable.h"

#include "common/debug.h"
#include "common/version.h"
#include "common/utilities.h"

#include "libGLESv2/main.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/VertexDataManager.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/DynamicHLSL.h"

#undef near
#undef far

namespace gl
{

namespace 
{

unsigned int ParseAndStripArrayIndex(std::string* name)
{
    unsigned int subscript = GL_INVALID_INDEX;

    // Strip any trailing array operator and retrieve the subscript
    size_t open = name->find_last_of('[');
    size_t close = name->find_last_of(']');
    if (open != std::string::npos && close == name->length() - 1)
    {
        subscript = atoi(name->substr(open + 1).c_str());
        name->erase(open);
    }

    return subscript;
}

}

VariableLocation::VariableLocation(const std::string &name, unsigned int element, unsigned int index) 
    : name(name), element(element), index(index)
{
}

unsigned int ProgramBinary::mCurrentSerial = 1;

ProgramBinary::ProgramBinary(rx::Renderer *renderer)
    : RefCountObject(0),
      mRenderer(renderer),
      mDynamicHLSL(NULL),
      mPixelExecutable(NULL),
      mVertexExecutable(NULL),
      mGeometryExecutable(NULL),
      mUsedVertexSamplerRange(0),
      mUsedPixelSamplerRange(0),
      mUsesPointSize(false),
      mShaderVersion(100),
      mVertexUniformStorage(NULL),
      mFragmentUniformStorage(NULL),
      mValidated(false),
      mSerial(issueSerial())
{
    for (int index = 0; index < MAX_VERTEX_ATTRIBS; index++)
    {
        mSemanticIndex[index] = -1;
    }

    for (int index = 0; index < MAX_TEXTURE_IMAGE_UNITS; index++)
    {
        mSamplersPS[index].active = false;
    }

    for (int index = 0; index < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; index++)
    {
        mSamplersVS[index].active = false;
    }

    mDynamicHLSL = new DynamicHLSL(renderer);
}

ProgramBinary::~ProgramBinary()
{
    SafeDelete(mPixelExecutable);
    SafeDelete(mVertexExecutable);
    SafeDelete(mGeometryExecutable);

    while (!mUniforms.empty())
    {
        delete mUniforms.back();
        mUniforms.pop_back();
    }

    while (!mUniformBlocks.empty())
    {
        delete mUniformBlocks.back();
        mUniformBlocks.pop_back();
    }

    SafeDelete(mVertexUniformStorage);
    SafeDelete(mFragmentUniformStorage);
    SafeDelete(mDynamicHLSL);
}

unsigned int ProgramBinary::getSerial() const
{
    return mSerial;
}

int ProgramBinary::getShaderVersion() const
{
    return mShaderVersion;
}

unsigned int ProgramBinary::issueSerial()
{
    return mCurrentSerial++;
}

rx::ShaderExecutable *ProgramBinary::getPixelExecutable() const
{
    return mPixelExecutable;
}

rx::ShaderExecutable *ProgramBinary::getVertexExecutable() const
{
    return mVertexExecutable;
}

rx::ShaderExecutable *ProgramBinary::getGeometryExecutable() const
{
    return mGeometryExecutable;
}

GLuint ProgramBinary::getAttributeLocation(const char *name)
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

int ProgramBinary::getSemanticIndex(int attributeIndex)
{
    ASSERT(attributeIndex >= 0 && attributeIndex < MAX_VERTEX_ATTRIBS);
    
    return mSemanticIndex[attributeIndex];
}

// Returns one more than the highest sampler index used.
GLint ProgramBinary::getUsedSamplerRange(SamplerType type)
{
    switch (type)
    {
      case SAMPLER_PIXEL:
        return mUsedPixelSamplerRange;
      case SAMPLER_VERTEX:
        return mUsedVertexSamplerRange;
      default:
        UNREACHABLE();
        return 0;
    }
}

bool ProgramBinary::usesPointSize() const
{
    return mUsesPointSize;
}

bool ProgramBinary::usesPointSpriteEmulation() const
{
    return mUsesPointSize && mRenderer->getMajorShaderModel() >= 4;
}

bool ProgramBinary::usesGeometryShader() const
{
    return usesPointSpriteEmulation();
}

// Returns the index of the texture image unit (0-19) corresponding to a Direct3D 9 sampler
// index (0-15 for the pixel shader and 0-3 for the vertex shader).
GLint ProgramBinary::getSamplerMapping(SamplerType type, unsigned int samplerIndex)
{
    GLint logicalTextureUnit = -1;

    switch (type)
    {
      case SAMPLER_PIXEL:
        ASSERT(samplerIndex < sizeof(mSamplersPS)/sizeof(mSamplersPS[0]));

        if (mSamplersPS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersPS[samplerIndex].logicalTextureUnit;
        }
        break;
      case SAMPLER_VERTEX:
        ASSERT(samplerIndex < sizeof(mSamplersVS)/sizeof(mSamplersVS[0]));

        if (mSamplersVS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersVS[samplerIndex].logicalTextureUnit;
        }
        break;
      default: UNREACHABLE();
    }

    if (logicalTextureUnit >= 0 && logicalTextureUnit < (GLint)mRenderer->getMaxCombinedTextureImageUnits())
    {
        return logicalTextureUnit;
    }

    return -1;
}

// Returns the texture type for a given Direct3D 9 sampler type and
// index (0-15 for the pixel shader and 0-3 for the vertex shader).
TextureType ProgramBinary::getSamplerTextureType(SamplerType type, unsigned int samplerIndex)
{
    switch (type)
    {
      case SAMPLER_PIXEL:
        ASSERT(samplerIndex < sizeof(mSamplersPS)/sizeof(mSamplersPS[0]));
        ASSERT(mSamplersPS[samplerIndex].active);
        return mSamplersPS[samplerIndex].textureType;
      case SAMPLER_VERTEX:
        ASSERT(samplerIndex < sizeof(mSamplersVS)/sizeof(mSamplersVS[0]));
        ASSERT(mSamplersVS[samplerIndex].active);
        return mSamplersVS[samplerIndex].textureType;
      default: UNREACHABLE();
    }

    return TEXTURE_2D;
}

GLint ProgramBinary::getUniformLocation(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    unsigned int numUniforms = mUniformIndex.size();
    for (unsigned int location = 0; location < numUniforms; location++)
    {
        if (mUniformIndex[location].name == name)
        {
            const int index = mUniformIndex[location].index;
            const bool isArray = mUniforms[index]->isArray();

            if ((isArray && mUniformIndex[location].element == subscript) || 
                (subscript == GL_INVALID_INDEX))
            {
                return location;
            }
        }
    }

    return -1;
}

GLuint ProgramBinary::getUniformIndex(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    // The app is not allowed to specify array indices other than 0 for arrays of basic types
    if (subscript != 0 && subscript != GL_INVALID_INDEX)
    {
        return GL_INVALID_INDEX;
    }

    unsigned int numUniforms = mUniforms.size();
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        if (mUniforms[index]->name == name)
        {
            if (mUniforms[index]->isArray() || subscript == GL_INVALID_INDEX)
            {
                return index;
            }
        }
    }

    return GL_INVALID_INDEX;
}

GLuint ProgramBinary::getUniformBlockIndex(std::string name)
{
    unsigned int subscript = ParseAndStripArrayIndex(&name);

    unsigned int numUniformBlocks = mUniformBlocks.size();
    for (unsigned int blockIndex = 0; blockIndex < numUniformBlocks; blockIndex++)
    {
        const UniformBlock &uniformBlock = *mUniformBlocks[blockIndex];
        if (uniformBlock.name == name)
        {
            const bool arrayElementZero = (subscript == GL_INVALID_INDEX && uniformBlock.elementIndex == 0);
            if (subscript == uniformBlock.elementIndex || arrayElementZero)
            {
                return blockIndex;
            }
        }
    }

    return GL_INVALID_INDEX;
}

UniformBlock *ProgramBinary::getUniformBlockByIndex(GLuint blockIndex)
{
    ASSERT(blockIndex < mUniformBlocks.size());
    return mUniformBlocks[blockIndex];
}

GLint ProgramBinary::getFragDataLocation(const char *name) const
{
    std::string baseName(name);
    unsigned int arrayIndex;
    arrayIndex = ParseAndStripArrayIndex(&baseName);

    for (auto locationIt = mOutputVariables.begin(); locationIt != mOutputVariables.end(); locationIt++)
    {
        const VariableLocation &outputVariable = locationIt->second;

        if (outputVariable.name == baseName && (arrayIndex == GL_INVALID_INDEX || arrayIndex == outputVariable.element))
        {
            return static_cast<GLint>(locationIt->first);
        }
    }

    return -1;
}

template <typename T>
bool ProgramBinary::setUniform(GLint location, GLsizei count, const T* v, GLenum targetUniformType)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    const int components = UniformComponentCount(targetUniformType);
    const GLenum targetBoolType = UniformBoolVectorType(targetUniformType);

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == targetUniformType)
    {
        T *target = (T*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            for (int c = 0; c < components; c++)
            {
                target[c] = v[c];
            }
            for (int c = components; c < 4; c++)
            {
                target[c] = 0;
            }
            target += 4;
            v += components;
        }
    }
    else if (targetUniform->type == targetBoolType)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            for (int c = 0; c < components; c++)
            {
                boolParams[c] = (v[c] == static_cast<T>(0)) ? GL_FALSE : GL_TRUE;
            }
            for (int c = components; c < 4; c++)
            {
                boolParams[c] = GL_FALSE;
            }
            boolParams += 4;
            v += components;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    return setUniform(location, count, v, GL_FLOAT);
}

bool ProgramBinary::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    return setUniform(location, count, v, GL_FLOAT_VEC2);
}

bool ProgramBinary::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    return setUniform(location, count, v, GL_FLOAT_VEC3);
}

bool ProgramBinary::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    return setUniform(location, count, v, GL_FLOAT_VEC4);
}

template<typename T>
void transposeMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    int copyWidth = std::min(targetHeight, srcWidth);
    int copyHeight = std::min(targetWidth, srcHeight);

    for (int x = 0; x < copyWidth; x++)
    {
        for (int y = 0; y < copyHeight; y++)
        {
            target[x * targetWidth + y] = static_cast<T>(value[y * srcWidth + x]);
        }
    }
    // clear unfilled right side
    for (int y = 0; y < copyWidth; y++)
    {
        for (int x = copyHeight; x < targetWidth; x++)
        {
            target[y * targetWidth + x] = static_cast<T>(0);
        }
    }
    // clear unfilled bottom.
    for (int y = copyWidth; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            target[y * targetWidth + x] = static_cast<T>(0);
        }
    }
}

template<typename T>
void expandMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    int copyWidth = std::min(targetWidth, srcWidth);
    int copyHeight = std::min(targetHeight, srcHeight);

    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = 0; x < copyWidth; x++)
        {
            target[y * targetWidth + x] = static_cast<T>(value[y * srcWidth + x]);
        }
    }
    // clear unfilled right side
    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = copyWidth; x < targetWidth; x++)
        {
            target[y * targetWidth + x] = static_cast<T>(0);
        }
    }
    // clear unfilled bottom.
    for (int y = copyHeight; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            target[y * targetWidth + x] = static_cast<T>(0);
        }
    }
}

template <int cols, int rows>
bool ProgramBinary::setUniformMatrixfv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value, GLenum targetUniformType) 
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    if (targetUniform->type != targetUniformType)
    {
        return false;
    }

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);
    const unsigned int targetMatrixStride = (4 * rows);
    GLfloat *target = (GLfloat*)(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * targetMatrixStride);

    for (int i = 0; i < count; i++)
    {
        // Internally store matrices as transposed versions to accomodate HLSL matrix indexing
        if (transpose == GL_FALSE)
        {
            transposeMatrix<GLfloat>(target, value, 4, rows, rows, cols);
        }
        else
        {
            expandMatrix<GLfloat>(target, value, 4, rows, cols, rows);
        }
        target += targetMatrixStride;
        value += cols * rows;
    }

    return true;
}

bool ProgramBinary::setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<2, 2>(location, count, transpose, value, GL_FLOAT_MAT2);
}

bool ProgramBinary::setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<3, 3>(location, count, transpose, value, GL_FLOAT_MAT3);
}

bool ProgramBinary::setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<4, 4>(location, count, transpose, value, GL_FLOAT_MAT4);
}

bool ProgramBinary::setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<2, 3>(location, count, transpose, value, GL_FLOAT_MAT2x3);
}

bool ProgramBinary::setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<3, 2>(location, count, transpose, value, GL_FLOAT_MAT3x2);
}

bool ProgramBinary::setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<2, 4>(location, count, transpose, value, GL_FLOAT_MAT2x4);
}

bool ProgramBinary::setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<4, 2>(location, count, transpose, value, GL_FLOAT_MAT4x2);
}

bool ProgramBinary::setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<3, 4>(location, count, transpose, value, GL_FLOAT_MAT3x4);
}

bool ProgramBinary::setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    return setUniformMatrixfv<4, 3>(location, count, transpose, value, GL_FLOAT_MAT4x3);
}

bool ProgramBinary::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];
    targetUniform->dirty = true;

    int elementCount = targetUniform->elementCount();

    if (elementCount == 1 && count > 1)
        return false; // attempting to write an array to a non-array uniform is an INVALID_OPERATION

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == GL_INT || IsSampler(targetUniform->type))
    {
        GLint *target = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            target[0] = v[0];
            target[1] = 0;
            target[2] = 0;
            target[3] = 0;
            target += 4;
            v += 1;
        }
    }
    else if (targetUniform->type == GL_BOOL)
    {
        GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            boolParams[0] = (v[0] == 0) ? GL_FALSE : GL_TRUE;
            boolParams[1] = GL_FALSE;
            boolParams[2] = GL_FALSE;
            boolParams[3] = GL_FALSE;
            boolParams += 4;
            v += 1;
        }
    }
    else
    {
        return false;
    }

    return true;
}

bool ProgramBinary::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    return setUniform(location, count, v, GL_INT_VEC2);
}

bool ProgramBinary::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    return setUniform(location, count, v, GL_INT_VEC3);
}

bool ProgramBinary::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    return setUniform(location, count, v, GL_INT_VEC4);
}

bool ProgramBinary::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
    return setUniform(location, count, v, GL_UNSIGNED_INT);
}

bool ProgramBinary::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
    return setUniform(location, count, v, GL_UNSIGNED_INT_VEC2);
}

bool ProgramBinary::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
    return setUniform(location, count, v, GL_UNSIGNED_INT_VEC3);
}

bool ProgramBinary::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
    return setUniform(location, count, v, GL_UNSIGNED_INT_VEC4);
}

template <typename T>
bool ProgramBinary::getUniformv(GLint location, GLsizei *bufSize, T *params, GLenum uniformType)
{
    if (location < 0 || location >= (int)mUniformIndex.size())
    {
        return false;
    }

    Uniform *targetUniform = mUniforms[mUniformIndex[location].index];

    // sized queries -- ensure the provided buffer is large enough
    if (bufSize)
    {
        int requiredBytes = UniformExternalSize(targetUniform->type);
        if (*bufSize < requiredBytes)
        {
            return false;
        }
    }

    if (IsMatrixType(targetUniform->type))
    {
        const int rows = VariableRowCount(targetUniform->type);
        const int cols = VariableColumnCount(targetUniform->type);
        transposeMatrix(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4 * rows, rows, cols, 4, rows);
    }
    else if (uniformType == UniformComponentType(targetUniform->type))
    {
        unsigned int size = UniformComponentCount(targetUniform->type);
        memcpy(params, targetUniform->data + mUniformIndex[location].element * 4 * sizeof(T),
                size * sizeof(T));
    }
    else
    {
        unsigned int size = UniformComponentCount(targetUniform->type);
        switch (UniformComponentType(targetUniform->type))
        {
          case GL_BOOL:
            {
                GLint *boolParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = (boolParams[i] == GL_FALSE) ? static_cast<T>(0) : static_cast<T>(1);
                }
            }
            break;

          case GL_FLOAT:
            {
                GLfloat *floatParams = (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(floatParams[i]);
                }
            }
            break;

          case GL_INT:
            {
                GLint *intParams = (GLint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(intParams[i]);
                }
            }
            break;
       
          case GL_UNSIGNED_INT:
            {
                GLuint *uintParams = (GLuint*)targetUniform->data + mUniformIndex[location].element * 4;

                for (unsigned int i = 0; i < size; i++)
                {
                    params[i] = static_cast<T>(uintParams[i]);
                }
            }
            break;
          
          default: UNREACHABLE();
        }
    }

    return true;
}

bool ProgramBinary::getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params)
{
    return getUniformv(location, bufSize, params, GL_FLOAT);
}

bool ProgramBinary::getUniformiv(GLint location, GLsizei *bufSize, GLint *params)
{
    return getUniformv(location, bufSize, params, GL_INT);
}

bool ProgramBinary::getUniformuiv(GLint location, GLsizei *bufSize, GLuint *params)
{
    return getUniformv(location, bufSize, params, GL_UNSIGNED_INT);
}

void ProgramBinary::dirtyAllUniforms()
{
    unsigned int numUniforms = mUniforms.size();
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        mUniforms[index]->dirty = true;
    }
}

// Applies all the uniforms set for this program object to the renderer
void ProgramBinary::applyUniforms()
{
    // Retrieve sampler uniform values
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        Uniform *targetUniform = mUniforms[uniformIndex];

        if (targetUniform->dirty)
        {
            if (IsSampler(targetUniform->type))
            {
                int count = targetUniform->elementCount();
                GLint (*v)[4] = (GLint(*)[4])targetUniform->data;

                if (targetUniform->isReferencedByFragmentShader())
                {
                    unsigned int firstIndex = targetUniform->psRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < MAX_TEXTURE_IMAGE_UNITS)
                        {
                            ASSERT(mSamplersPS[samplerIndex].active);
                            mSamplersPS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }

                if (targetUniform->isReferencedByVertexShader())
                {
                    unsigned int firstIndex = targetUniform->vsRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS)
                        {
                            ASSERT(mSamplersVS[samplerIndex].active);
                            mSamplersVS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }
            }
        }
    }

    mRenderer->applyUniforms(*this);

    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        mUniforms[uniformIndex]->dirty = false;
    }
}

bool ProgramBinary::applyUniformBuffers(const std::vector<gl::Buffer*> boundBuffers)
{
    const gl::Buffer *vertexUniformBuffers[gl::IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS] = {NULL};
    const gl::Buffer *fragmentUniformBuffers[gl::IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS] = {NULL};

    const unsigned int reservedBuffersInVS = mRenderer->getReservedVertexUniformBuffers();
    const unsigned int reservedBuffersInFS = mRenderer->getReservedFragmentUniformBuffers();

    ASSERT(boundBuffers.size() == mUniformBlocks.size());

    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); uniformBlockIndex++)
    {
        gl::UniformBlock *uniformBlock = getUniformBlockByIndex(uniformBlockIndex);
        gl::Buffer *uniformBuffer = boundBuffers[uniformBlockIndex];

        ASSERT(uniformBlock && uniformBuffer);

        if (uniformBuffer->size() < uniformBlock->dataSize)
        {
            // undefined behaviour
            return false;
        }

        ASSERT(uniformBlock->isReferencedByVertexShader() || uniformBlock->isReferencedByFragmentShader());

        if (uniformBlock->isReferencedByVertexShader())
        {
            unsigned int registerIndex = uniformBlock->vsRegisterIndex - reservedBuffersInVS;
            ASSERT(vertexUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < mRenderer->getMaxVertexShaderUniformBuffers());
            vertexUniformBuffers[registerIndex] = uniformBuffer;
        }

        if (uniformBlock->isReferencedByFragmentShader())
        {
            unsigned int registerIndex = uniformBlock->psRegisterIndex - reservedBuffersInFS;
            ASSERT(fragmentUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < mRenderer->getMaxFragmentShaderUniformBuffers());
            fragmentUniformBuffers[registerIndex] = uniformBuffer;
        }
    }

    return mRenderer->setUniformBuffers(vertexUniformBuffers, fragmentUniformBuffers);
}

bool ProgramBinary::linkVaryings(InfoLog &infoLog, FragmentShader *fragmentShader, VertexShader *vertexShader)
{
    vertexShader->resetVaryingsRegisterAssignment();

    std::vector<sh::Varying> &fragmentVaryings = fragmentShader->getVaryings();
    std::vector<sh::Varying> &vertexVaryings = vertexShader->getVaryings();

    for (size_t fragVaryingIndex = 0; fragVaryingIndex < fragmentVaryings.size(); fragVaryingIndex++)
    {
        sh::Varying *input = &fragmentVaryings[fragVaryingIndex];
        bool matched = false;

        for (size_t vertVaryingIndex = 0; vertVaryingIndex < vertexVaryings.size(); vertVaryingIndex++)
        {
            sh::Varying *output = &vertexVaryings[vertVaryingIndex];
            if (output->name == input->name)
            {
                if (!linkValidateVariables(infoLog, output->name, *input, *output))
                {
                    return false;
                }

                output->registerIndex = input->registerIndex;
                output->elementIndex = input->elementIndex;

                matched = true;
                break;
            }
        }

        if (!matched)
        {
            infoLog.append("Fragment varying %s does not match any vertex varying", input->name.c_str());
            return false;
        }
    }

    return true;
}

bool ProgramBinary::load(InfoLog &infoLog, const void *binary, GLsizei length)
{
    BinaryInputStream stream(binary, length);

    int format = 0;
    stream.read(&format);
    if (format != GL_PROGRAM_BINARY_ANGLE)
    {
        infoLog.append("Invalid program binary format.");
        return false;
    }

    int majorVersion = 0;
    int minorVersion = 0;
    stream.read(&majorVersion);
    stream.read(&minorVersion);
    if (majorVersion != ANGLE_MAJOR_VERSION || minorVersion != ANGLE_MINOR_VERSION)
    {
        infoLog.append("Invalid program binary version.");
        return false;
    }

    unsigned char commitString[ANGLE_COMMIT_HASH_SIZE];
    stream.read(commitString, ANGLE_COMMIT_HASH_SIZE);
    if (memcmp(commitString, ANGLE_COMMIT_HASH, sizeof(unsigned char) * ANGLE_COMMIT_HASH_SIZE) != 0)
    {
        infoLog.append("Invalid program binary version.");
        return false;
    }

    int compileFlags = 0;
    stream.read(&compileFlags);
    if (compileFlags != ANGLE_COMPILE_OPTIMIZATION_LEVEL)
    {
        infoLog.append("Mismatched compilation flags.");
        return false;
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.read(&mLinkedAttribute[i].type);
        std::string name;
        stream.read(&name);
        mLinkedAttribute[i].name = name;
        stream.read(&mSemanticIndex[i]);
    }

    initAttributesByLayout();

    for (unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.read(&mSamplersPS[i].active);
        stream.read(&mSamplersPS[i].logicalTextureUnit);
        
        int textureType;
        stream.read(&textureType);
        mSamplersPS[i].textureType = (TextureType) textureType;
    }

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.read(&mSamplersVS[i].active);
        stream.read(&mSamplersVS[i].logicalTextureUnit);
        
        int textureType;
        stream.read(&textureType);
        mSamplersVS[i].textureType = (TextureType) textureType;
    }

    stream.read(&mUsedVertexSamplerRange);
    stream.read(&mUsedPixelSamplerRange);
    stream.read(&mUsesPointSize);
    stream.read(&mShaderVersion);

    size_t size;
    stream.read(&size);
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniforms.resize(size);
    for (unsigned int i = 0; i < size; ++i)
    {
        GLenum type;
        GLenum precision;
        std::string name;
        unsigned int arraySize;
        int blockIndex;

        stream.read(&type);
        stream.read(&precision);
        stream.read(&name);
        stream.read(&arraySize);
        stream.read(&blockIndex);

        int offset;
        int arrayStride;
        int matrixStride;
        bool isRowMajorMatrix;

        stream.read(&offset);
        stream.read(&arrayStride);
        stream.read(&matrixStride);
        stream.read(&isRowMajorMatrix);

        const sh::BlockMemberInfo blockInfo(offset, arrayStride, matrixStride, isRowMajorMatrix);

        mUniforms[i] = new Uniform(type, precision, name, arraySize, blockIndex, blockInfo);
        
        stream.read(&mUniforms[i]->psRegisterIndex);
        stream.read(&mUniforms[i]->vsRegisterIndex);
        stream.read(&mUniforms[i]->registerCount);
        stream.read(&mUniforms[i]->registerElement);
    }

    stream.read(&size);
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniformBlocks.resize(size);
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < size; ++uniformBlockIndex)
    {
        std::string name;
        unsigned int elementIndex;
        unsigned int dataSize;

        stream.read(&name);
        stream.read(&elementIndex);
        stream.read(&dataSize);

        mUniformBlocks[uniformBlockIndex] = new UniformBlock(name, elementIndex, dataSize);

        UniformBlock& uniformBlock = *mUniformBlocks[uniformBlockIndex];
        stream.read(&uniformBlock.psRegisterIndex);
        stream.read(&uniformBlock.vsRegisterIndex);

        size_t numMembers;
        stream.read(&numMembers);
        uniformBlock.memberUniformIndexes.resize(numMembers);
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < numMembers; blockMemberIndex++)
        {
            stream.read(&uniformBlock.memberUniformIndexes[blockMemberIndex]);
        }
    }

    stream.read(&size);
    if (stream.error())
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    mUniformIndex.resize(size);
    for (unsigned int i = 0; i < size; ++i)
    {
        stream.read(&mUniformIndex[i].name);
        stream.read(&mUniformIndex[i].element);
        stream.read(&mUniformIndex[i].index);
    }

    unsigned int pixelShaderSize;
    stream.read(&pixelShaderSize);

    unsigned int vertexShaderSize;
    stream.read(&vertexShaderSize);

    unsigned int geometryShaderSize;
    stream.read(&geometryShaderSize);

    const char *ptr = (const char*) binary + stream.offset();

    const GUID *binaryIdentifier = (const GUID *) ptr;
    ptr += sizeof(GUID);

    GUID identifier = mRenderer->getAdapterIdentifier();
    if (memcmp(&identifier, binaryIdentifier, sizeof(GUID)) != 0)
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    const char *pixelShaderFunction = ptr;
    ptr += pixelShaderSize;

    const char *vertexShaderFunction = ptr;
    ptr += vertexShaderSize;

    const char *geometryShaderFunction = geometryShaderSize > 0 ? ptr : NULL;
    ptr += geometryShaderSize;

    mPixelExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(pixelShaderFunction),
                                                 pixelShaderSize, rx::SHADER_PIXEL);
    if (!mPixelExecutable)
    {
        infoLog.append("Could not create pixel shader.");
        return false;
    }

    mVertexExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(vertexShaderFunction),
                                                  vertexShaderSize, rx::SHADER_VERTEX);
    if (!mVertexExecutable)
    {
        infoLog.append("Could not create vertex shader.");
        delete mPixelExecutable;
        mPixelExecutable = NULL;
        return false;
    }

    if (geometryShaderFunction != NULL && geometryShaderSize > 0)
    {
        mGeometryExecutable = mRenderer->loadExecutable(reinterpret_cast<const DWORD*>(geometryShaderFunction),
                                                        geometryShaderSize, rx::SHADER_GEOMETRY);
        if (!mGeometryExecutable)
        {
            infoLog.append("Could not create geometry shader.");
            delete mPixelExecutable;
            mPixelExecutable = NULL;
            delete mVertexExecutable;
            mVertexExecutable = NULL;
            return false;
        }
    }
    else
    {
        mGeometryExecutable = NULL;
    }

    initializeUniformStorage();

    return true;
}

bool ProgramBinary::save(void* binary, GLsizei bufSize, GLsizei *length)
{
    BinaryOutputStream stream;

    stream.write(GL_PROGRAM_BINARY_ANGLE);
    stream.write(ANGLE_MAJOR_VERSION);
    stream.write(ANGLE_MINOR_VERSION);
    stream.write(ANGLE_COMMIT_HASH, ANGLE_COMMIT_HASH_SIZE);
    stream.write(ANGLE_COMPILE_OPTIMIZATION_LEVEL);

    for (unsigned int i = 0; i < MAX_VERTEX_ATTRIBS; ++i)
    {
        stream.write(mLinkedAttribute[i].type);
        stream.write(mLinkedAttribute[i].name);
        stream.write(mSemanticIndex[i]);
    }

    for (unsigned int i = 0; i < MAX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.write(mSamplersPS[i].active);
        stream.write(mSamplersPS[i].logicalTextureUnit);
        stream.write((int) mSamplersPS[i].textureType);
    }

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; ++i)
    {
        stream.write(mSamplersVS[i].active);
        stream.write(mSamplersVS[i].logicalTextureUnit);
        stream.write((int) mSamplersVS[i].textureType);
    }

    stream.write(mUsedVertexSamplerRange);
    stream.write(mUsedPixelSamplerRange);
    stream.write(mUsesPointSize);
    stream.write(mShaderVersion);

    stream.write(mUniforms.size());
    for (unsigned int uniformIndex = 0; uniformIndex < mUniforms.size(); ++uniformIndex)
    {
        const Uniform &uniform = *mUniforms[uniformIndex];

        stream.write(uniform.type);
        stream.write(uniform.precision);
        stream.write(uniform.name);
        stream.write(uniform.arraySize);
        stream.write(uniform.blockIndex);

        stream.write(uniform.blockInfo.offset);
        stream.write(uniform.blockInfo.arrayStride);
        stream.write(uniform.blockInfo.matrixStride);
        stream.write(uniform.blockInfo.isRowMajorMatrix);

        stream.write(uniform.psRegisterIndex);
        stream.write(uniform.vsRegisterIndex);
        stream.write(uniform.registerCount);
        stream.write(uniform.registerElement);
    }

    stream.write(mUniformBlocks.size());
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); ++uniformBlockIndex)
    {
        const UniformBlock& uniformBlock = *mUniformBlocks[uniformBlockIndex];

        stream.write(uniformBlock.name);
        stream.write(uniformBlock.elementIndex);
        stream.write(uniformBlock.dataSize);

        stream.write(uniformBlock.memberUniformIndexes.size());
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
        {
            stream.write(uniformBlock.memberUniformIndexes[blockMemberIndex]);
        }

        stream.write(uniformBlock.psRegisterIndex);
        stream.write(uniformBlock.vsRegisterIndex);
    }

    stream.write(mUniformIndex.size());
    for (unsigned int i = 0; i < mUniformIndex.size(); ++i)
    {
        stream.write(mUniformIndex[i].name);
        stream.write(mUniformIndex[i].element);
        stream.write(mUniformIndex[i].index);
    }

    UINT pixelShaderSize = mPixelExecutable->getLength();
    stream.write(pixelShaderSize);

    UINT vertexShaderSize = mVertexExecutable->getLength();
    stream.write(vertexShaderSize);

    UINT geometryShaderSize = (mGeometryExecutable != NULL) ? mGeometryExecutable->getLength() : 0;
    stream.write(geometryShaderSize);

    GUID identifier = mRenderer->getAdapterIdentifier();

    GLsizei streamLength = stream.length();
    const void *streamData = stream.data();

    GLsizei totalLength = streamLength + sizeof(GUID) + pixelShaderSize + vertexShaderSize + geometryShaderSize;
    if (totalLength > bufSize)
    {
        if (length)
        {
            *length = 0;
        }

        return false;
    }

    if (binary)
    {
        char *ptr = (char*) binary;

        memcpy(ptr, streamData, streamLength);
        ptr += streamLength;

        memcpy(ptr, &identifier, sizeof(GUID));
        ptr += sizeof(GUID);

        memcpy(ptr, mPixelExecutable->getFunction(), pixelShaderSize);
        ptr += pixelShaderSize;

        memcpy(ptr, mVertexExecutable->getFunction(), vertexShaderSize);
        ptr += vertexShaderSize;

        if (mGeometryExecutable != NULL && geometryShaderSize > 0)
        {
            memcpy(ptr, mGeometryExecutable->getFunction(), geometryShaderSize);
            ptr += geometryShaderSize;
        }

        ASSERT(ptr - totalLength == binary);
    }

    if (length)
    {
        *length = totalLength;
    }

    return true;
}

GLint ProgramBinary::getLength()
{
    GLint length;
    if (save(NULL, INT_MAX, &length))
    {
        return length;
    }
    else
    {
        return 0;
    }
}

bool ProgramBinary::link(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader)
{
    if (!fragmentShader || !fragmentShader->isCompiled())
    {
        return false;
    }

    if (!vertexShader || !vertexShader->isCompiled())
    {
        return false;
    }

    mShaderVersion = vertexShader->getShaderVersion();

    std::string pixelHLSL = fragmentShader->getHLSL();
    std::string vertexHLSL = vertexShader->getHLSL();

    // Map the varyings to the register file
    const sh::ShaderVariable *packing[IMPLEMENTATION_MAX_VARYING_VECTORS][4] = {NULL};
    int registers = mDynamicHLSL->packVaryings(infoLog, packing, fragmentShader);

    if (registers < 0)
    {
        return false;
    }

    if (!linkVaryings(infoLog, fragmentShader, vertexShader))
    {
        return false;
    }

    mUsesPointSize = vertexShader->usesPointSize();
    if (!mDynamicHLSL->generateShaderLinkHLSL(infoLog, registers, packing, pixelHLSL, vertexHLSL, fragmentShader, vertexShader, &mOutputVariables))
    {
        return false;
    }

    bool success = true;

    if (!linkAttributes(infoLog, attributeBindings, fragmentShader, vertexShader))
    {
        success = false;
    }

    if (!linkUniforms(infoLog, vertexShader->getUniforms(), fragmentShader->getUniforms()))
    {
        success = false;
    }

    // special case for gl_DepthRange, the only built-in uniform (also a struct)
    if (vertexShader->usesDepthRange() || fragmentShader->usesDepthRange())
    {
        mUniforms.push_back(new Uniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.near", 0, -1, sh::BlockMemberInfo::defaultBlockInfo));
        mUniforms.push_back(new Uniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.far", 0, -1, sh::BlockMemberInfo::defaultBlockInfo));
        mUniforms.push_back(new Uniform(GL_FLOAT, GL_HIGH_FLOAT, "gl_DepthRange.diff", 0, -1, sh::BlockMemberInfo::defaultBlockInfo));
    }

    if (!linkUniformBlocks(infoLog, vertexShader->getInterfaceBlocks(), fragmentShader->getInterfaceBlocks()))
    {
        success = false;
    }

    if (success)
    {
        mVertexExecutable = mRenderer->compileToExecutable(infoLog, vertexHLSL.c_str(), rx::SHADER_VERTEX, vertexShader->getD3DWorkarounds());
        mPixelExecutable = mRenderer->compileToExecutable(infoLog, pixelHLSL.c_str(), rx::SHADER_PIXEL, vertexShader->getD3DWorkarounds());

        if (usesGeometryShader())
        {
            std::string geometryHLSL = mDynamicHLSL->generateGeometryShaderHLSL(registers, packing, fragmentShader, vertexShader);
            mGeometryExecutable = mRenderer->compileToExecutable(infoLog, geometryHLSL.c_str(), rx::SHADER_GEOMETRY, rx::ANGLE_D3D_WORKAROUND_NONE);
        }

        if (!mVertexExecutable || !mPixelExecutable || (usesGeometryShader() && !mGeometryExecutable))
        {
            infoLog.append("Failed to create D3D shaders.");
            success = false;

            delete mVertexExecutable;
            mVertexExecutable = NULL;
            delete mPixelExecutable;
            mPixelExecutable = NULL;
            delete mGeometryExecutable;
            mGeometryExecutable = NULL;
        }
    }

    return success;
}

// Determines the mapping between GL attributes and Direct3D 9 vertex stream usage indices
bool ProgramBinary::linkAttributes(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader)
{
    unsigned int usedLocations = 0;
    const std::vector<sh::Attribute> &activeAttributes = vertexShader->activeAttributes();

    // Link attributes that have a binding location
    for (unsigned int attributeIndex = 0; attributeIndex < activeAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = activeAttributes[attributeIndex];
        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        if (location != -1)   // Set by glBindAttribLocation or by location layout qualifier
        {
            const int rows = AttributeRegisterCount(attribute.type);

            if (rows + location > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Active attribute (%s) at location %d is too big to fit", attribute.name.c_str(), location);

                return false;
            }

            for (int row = 0; row < rows; row++)
            {
                const int rowLocation = location + row;
                sh::ShaderVariable &linkedAttribute = mLinkedAttribute[rowLocation];

                // In GLSL 3.00, attribute aliasing produces a link error
                // In GLSL 1.00, attribute aliasing is allowed
                if (mShaderVersion >= 300)
                {
                    if (!linkedAttribute.name.empty())
                    {
                        infoLog.append("Attribute '%s' aliases attribute '%s' at location %d", attribute.name.c_str(), linkedAttribute.name.c_str(), rowLocation);
                        return false;
                    }
                }

                linkedAttribute = attribute;
                usedLocations |= 1 << rowLocation;
            }
        }
    }

    // Link attributes that don't have a binding location
    for (unsigned int attributeIndex = 0; attributeIndex < activeAttributes.size(); attributeIndex++)
    {
        const sh::Attribute &attribute = activeAttributes[attributeIndex];
        const int location = attribute.location == -1 ? attributeBindings.getAttributeBinding(attribute.name) : attribute.location;

        if (location == -1)   // Not set by glBindAttribLocation or by location layout qualifier
        {
            int rows = AttributeRegisterCount(attribute.type);
            int availableIndex = AllocateFirstFreeBits(&usedLocations, rows, MAX_VERTEX_ATTRIBS);

            if (availableIndex == -1 || availableIndex + rows > MAX_VERTEX_ATTRIBS)
            {
                infoLog.append("Too many active attributes (%s)", attribute.name.c_str());

                return false;   // Fail to link
            }

            mLinkedAttribute[availableIndex] = attribute;
        }
    }

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; )
    {
        int index = vertexShader->getSemanticIndex(mLinkedAttribute[attributeIndex].name);
        int rows = AttributeRegisterCount(mLinkedAttribute[attributeIndex].type);

        for (int r = 0; r < rows; r++)
        {
            mSemanticIndex[attributeIndex++] = index++;
        }
    }

    initAttributesByLayout();

    return true;
}

bool ProgramBinary::linkValidateVariablesBase(InfoLog &infoLog, const std::string &variableName, const sh::ShaderVariable &vertexVariable, const sh::ShaderVariable &fragmentVariable, bool validatePrecision)
{
    if (vertexVariable.type != fragmentVariable.type)
    {
        infoLog.append("Types for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    if (vertexVariable.arraySize != fragmentVariable.arraySize)
    {
        infoLog.append("Array sizes for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }
    if (validatePrecision && vertexVariable.precision != fragmentVariable.precision)
    {
        infoLog.append("Precisions for %s differ between vertex and fragment shaders", variableName.c_str());
        return false;
    }

    return true;
}

template <class ShaderVarType>
bool ProgramBinary::linkValidateFields(InfoLog &infoLog, const std::string &varName, const ShaderVarType &vertexVar, const ShaderVarType &fragmentVar)
{
    if (vertexVar.fields.size() != fragmentVar.fields.size())
    {
        infoLog.append("Structure lengths for %s differ between vertex and fragment shaders", varName.c_str());
        return false;
    }
    const unsigned int numMembers = vertexVar.fields.size();
    for (unsigned int memberIndex = 0; memberIndex < numMembers; memberIndex++)
    {
        const ShaderVarType &vertexMember = vertexVar.fields[memberIndex];
        const ShaderVarType &fragmentMember = fragmentVar.fields[memberIndex];

        if (vertexMember.name != fragmentMember.name)
        {
            infoLog.append("Name mismatch for field '%d' of %s: (in vertex: '%s', in fragment: '%s')",
                           memberIndex, varName.c_str(), vertexMember.name.c_str(), fragmentMember.name.c_str());
            return false;
        }

        const std::string memberName = varName.substr(0, varName.length()-1) + "." + vertexVar.name + "'";
        if (!linkValidateVariables(infoLog, memberName, vertexMember, fragmentMember))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::linkValidateVariables(InfoLog &infoLog, const std::string &uniformName, const sh::Uniform &vertexUniform, const sh::Uniform &fragmentUniform)
{
    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, true))
    {
        return false;
    }

    if (!linkValidateFields<sh::Uniform>(infoLog, uniformName, vertexUniform, fragmentUniform))
    {
        return false;
    }

    return true;
}

bool ProgramBinary::linkValidateVariables(InfoLog &infoLog, const std::string &varyingName, const sh::Varying &vertexVarying, const sh::Varying &fragmentVarying)
{
    if (!linkValidateVariablesBase(infoLog, varyingName, vertexVarying, fragmentVarying, false))
    {
        return false;
    }

    if (vertexVarying.interpolation != fragmentVarying.interpolation)
    {
        infoLog.append("Interpolation types for %s differ between vertex and fragment shaders", varyingName.c_str());
        return false;
    }

    if (!linkValidateFields<sh::Varying>(infoLog, varyingName, vertexVarying, fragmentVarying))
    {
        return false;
    }

    return true;
}

bool ProgramBinary::linkValidateVariables(InfoLog &infoLog, const std::string &uniformName, const sh::InterfaceBlockField &vertexUniform, const sh::InterfaceBlockField &fragmentUniform)
{
    if (!linkValidateVariablesBase(infoLog, uniformName, vertexUniform, fragmentUniform, true))
    {
        return false;
    }

    if (vertexUniform.isRowMajorMatrix != fragmentUniform.isRowMajorMatrix)
    {
        infoLog.append("Matrix packings for %s differ between vertex and fragment shaders", uniformName.c_str());
        return false;
    }

    if (!linkValidateFields<sh::InterfaceBlockField>(infoLog, uniformName, vertexUniform, fragmentUniform))
    {
        return false;
    }

    return true;
}

bool ProgramBinary::linkUniforms(InfoLog &infoLog, const std::vector<sh::Uniform> &vertexUniforms, const std::vector<sh::Uniform> &fragmentUniforms)
{
    // Check that uniforms defined in the vertex and fragment shaders are identical
    typedef std::map<std::string, const sh::Uniform*> UniformMap;
    UniformMap linkedUniforms;

    for (unsigned int vertexUniformIndex = 0; vertexUniformIndex < vertexUniforms.size(); vertexUniformIndex++)
    {
        const sh::Uniform &vertexUniform = vertexUniforms[vertexUniformIndex];
        linkedUniforms[vertexUniform.name] = &vertexUniform;
    }

    for (unsigned int fragmentUniformIndex = 0; fragmentUniformIndex < fragmentUniforms.size(); fragmentUniformIndex++)
    {
        const sh::Uniform &fragmentUniform = fragmentUniforms[fragmentUniformIndex];
        UniformMap::const_iterator entry = linkedUniforms.find(fragmentUniform.name);
        if (entry != linkedUniforms.end())
        {
            const sh::Uniform &vertexUniform = *entry->second;
            const std::string &uniformName = "uniform '" + vertexUniform.name + "'";
            if (!linkValidateVariables(infoLog, uniformName, vertexUniform, fragmentUniform))
            {
                return false;
            }
        }
    }

    for (unsigned int uniformIndex = 0; uniformIndex < vertexUniforms.size(); uniformIndex++)
    {
        if (!defineUniform(GL_VERTEX_SHADER, vertexUniforms[uniformIndex], infoLog))
        {
            return false;
        }
    }

    for (unsigned int uniformIndex = 0; uniformIndex < fragmentUniforms.size(); uniformIndex++)
    {
        if (!defineUniform(GL_FRAGMENT_SHADER, fragmentUniforms[uniformIndex], infoLog))
        {
            return false;
        }
    }

    initializeUniformStorage();

    return true;
}

int totalRegisterCount(const sh::Uniform &uniform)
{
    int registerCount = 0;

    if (!uniform.fields.empty())
    {
        for (unsigned int fieldIndex = 0; fieldIndex < uniform.fields.size(); fieldIndex++)
        {
            registerCount += totalRegisterCount(uniform.fields[fieldIndex]);
        }
    }
    else
    {
        registerCount = 1;
    }

    return (uniform.arraySize > 0) ? uniform.arraySize * registerCount : registerCount;
}

TextureType ProgramBinary::getTextureType(GLenum samplerType, InfoLog &infoLog)
{
    switch(samplerType)
    {
      case GL_SAMPLER_2D:
      case GL_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_SAMPLER_2D_SHADOW:
        return TEXTURE_2D;
      case GL_SAMPLER_3D:
      case GL_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
        return TEXTURE_3D;
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_CUBE_SHADOW:
        return TEXTURE_CUBE;
      case GL_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
        //UNIMPLEMENTED();
        infoLog.append("Integer cube texture sampling is currently not supported by ANGLE and returns a black color.");
        return TEXTURE_CUBE;
      case GL_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
        return TEXTURE_2D_ARRAY;
      default: UNREACHABLE();
    }

    return TEXTURE_2D;
}

bool ProgramBinary::defineUniform(GLenum shader, const sh::Uniform &constant, InfoLog &infoLog)
{
    if (constant.isStruct())
    {
        if (constant.arraySize > 0)
        {
            unsigned int elementRegisterIndex = constant.registerIndex;

            for (unsigned int elementIndex = 0; elementIndex < constant.arraySize; elementIndex++)
            {
                for (size_t fieldIndex = 0; fieldIndex < constant.fields.size(); fieldIndex++)
                {
                    const sh::Uniform &field = constant.fields[fieldIndex];
                    const std::string &uniformName = constant.name + ArrayString(elementIndex) + "." + field.name;
                    sh::Uniform fieldUniform(field.type, field.precision, uniformName.c_str(), field.arraySize,
                                             elementRegisterIndex, field.elementIndex);

                    fieldUniform.fields = field.fields;
                    if (!defineUniform(shader, fieldUniform, infoLog))
                    {
                        return false;
                    }
                    elementRegisterIndex += totalRegisterCount(field);
                }
            }
        }
        else
        {
            for (size_t fieldIndex = 0; fieldIndex < constant.fields.size(); fieldIndex++)
            {
                const sh::Uniform &field = constant.fields[fieldIndex];
                const std::string &uniformName = constant.name + "." + field.name;

                sh::Uniform fieldUniform(field.type, field.precision, uniformName.c_str(), field.arraySize,
                                         field.registerIndex, field.elementIndex);

                fieldUniform.fields = field.fields;

                if (!defineUniform(shader, fieldUniform, infoLog))
                {
                    return false;
                }
            }
        }

        return true;
    }

    if (IsSampler(constant.type))
    {
        unsigned int samplerIndex = constant.registerIndex;
            
        do
        {
            if (shader == GL_VERTEX_SHADER)
            {
                if (samplerIndex < mRenderer->getMaxVertexTextureImageUnits())
                {
                    mSamplersVS[samplerIndex].active = true;
                    mSamplersVS[samplerIndex].textureType = getTextureType(constant.type, infoLog);
                    mSamplersVS[samplerIndex].logicalTextureUnit = 0;
                    mUsedVertexSamplerRange = std::max(samplerIndex + 1, mUsedVertexSamplerRange);
                }
                else
                {
                    infoLog.append("Vertex shader sampler count exceeds the maximum vertex texture units (%d).", mRenderer->getMaxVertexTextureImageUnits());
                    return false;
                }
            }
            else if (shader == GL_FRAGMENT_SHADER)
            {
                if (samplerIndex < MAX_TEXTURE_IMAGE_UNITS)
                {
                    mSamplersPS[samplerIndex].active = true;
                    mSamplersPS[samplerIndex].textureType = getTextureType(constant.type, infoLog);
                    mSamplersPS[samplerIndex].logicalTextureUnit = 0;
                    mUsedPixelSamplerRange = std::max(samplerIndex + 1, mUsedPixelSamplerRange);
                }
                else
                {
                    infoLog.append("Pixel shader sampler count exceeds MAX_TEXTURE_IMAGE_UNITS (%d).", MAX_TEXTURE_IMAGE_UNITS);
                    return false;
                }
            }
            else UNREACHABLE();

            samplerIndex++;
        }
        while (samplerIndex < constant.registerIndex + constant.arraySize);
    }

    Uniform *uniform = NULL;
    GLint location = getUniformLocation(constant.name);

    if (location >= 0)   // Previously defined, type and precision must match
    {
        uniform = mUniforms[mUniformIndex[location].index];
    }
    else
    {
        uniform = new Uniform(constant.type, constant.precision, constant.name, constant.arraySize, -1, sh::BlockMemberInfo::defaultBlockInfo);
        uniform->registerElement = constant.elementIndex;
    }

    if (!uniform)
    {
        return false;
    }

    if (shader == GL_FRAGMENT_SHADER)
    {
        uniform->psRegisterIndex = constant.registerIndex;
    }
    else if (shader == GL_VERTEX_SHADER)
    {
        uniform->vsRegisterIndex = constant.registerIndex;
    }
    else UNREACHABLE();

    if (location >= 0)
    {
        return uniform->type == constant.type;
    }

    mUniforms.push_back(uniform);
    unsigned int uniformIndex = mUniforms.size() - 1;

    for (unsigned int arrayElementIndex = 0; arrayElementIndex < uniform->elementCount(); arrayElementIndex++)
    {
        mUniformIndex.push_back(VariableLocation(uniform->name, arrayElementIndex, uniformIndex));
    }

    if (shader == GL_VERTEX_SHADER)
    {
        if (constant.registerIndex + uniform->registerCount > mRenderer->getReservedVertexUniformVectors() + mRenderer->getMaxVertexUniformVectors())
        {
            infoLog.append("Vertex shader active uniforms exceed GL_MAX_VERTEX_UNIFORM_VECTORS (%u)", mRenderer->getMaxVertexUniformVectors());
            return false;
        }
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        if (constant.registerIndex + uniform->registerCount > mRenderer->getReservedFragmentUniformVectors() + mRenderer->getMaxFragmentUniformVectors())
        {
            infoLog.append("Fragment shader active uniforms exceed GL_MAX_FRAGMENT_UNIFORM_VECTORS (%u)", mRenderer->getMaxFragmentUniformVectors());
            return false;
        }
    }
    else UNREACHABLE();

    return true;
}

bool ProgramBinary::areMatchingInterfaceBlocks(InfoLog &infoLog, const sh::InterfaceBlock &vertexInterfaceBlock, const sh::InterfaceBlock &fragmentInterfaceBlock)
{
    const char* blockName = vertexInterfaceBlock.name.c_str();

    // validate blocks for the same member types
    if (vertexInterfaceBlock.fields.size() != fragmentInterfaceBlock.fields.size())
    {
        infoLog.append("Types for interface block '%s' differ between vertex and fragment shaders", blockName);
        return false;
    }

    if (vertexInterfaceBlock.arraySize != fragmentInterfaceBlock.arraySize)
    {
        infoLog.append("Array sizes differ for interface block '%s' between vertex and fragment shaders", blockName);
        return false;
    }

    if (vertexInterfaceBlock.layout != fragmentInterfaceBlock.layout || vertexInterfaceBlock.isRowMajorLayout != fragmentInterfaceBlock.isRowMajorLayout)
    {
        infoLog.append("Layout qualifiers differ for interface block '%s' between vertex and fragment shaders", blockName);
        return false;
    }

    const unsigned int numBlockMembers = vertexInterfaceBlock.fields.size();
    for (unsigned int blockMemberIndex = 0; blockMemberIndex < numBlockMembers; blockMemberIndex++)
    {
        const sh::InterfaceBlockField &vertexMember = vertexInterfaceBlock.fields[blockMemberIndex];
        const sh::InterfaceBlockField &fragmentMember = fragmentInterfaceBlock.fields[blockMemberIndex];

        if (vertexMember.name != fragmentMember.name)
        {
            infoLog.append("Name mismatch for field %d of interface block '%s': (in vertex: '%s', in fragment: '%s')",
                           blockMemberIndex, blockName, vertexMember.name.c_str(), fragmentMember.name.c_str());
            return false;
        }

        std::string uniformName = "interface block '" + vertexInterfaceBlock.name + "' member '" + vertexMember.name + "'";
        if (!linkValidateVariables(infoLog, uniformName, vertexMember, fragmentMember))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::linkUniformBlocks(InfoLog &infoLog, const sh::ActiveInterfaceBlocks &vertexInterfaceBlocks, const sh::ActiveInterfaceBlocks &fragmentInterfaceBlocks)
{
    // Check that interface blocks defined in the vertex and fragment shaders are identical
    typedef std::map<std::string, const sh::InterfaceBlock*> UniformBlockMap;
    UniformBlockMap linkedUniformBlocks;

    for (unsigned int blockIndex = 0; blockIndex < vertexInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &vertexInterfaceBlock = vertexInterfaceBlocks[blockIndex];
        linkedUniformBlocks[vertexInterfaceBlock.name] = &vertexInterfaceBlock;
    }

    for (unsigned int blockIndex = 0; blockIndex < fragmentInterfaceBlocks.size(); blockIndex++)
    {
        const sh::InterfaceBlock &fragmentInterfaceBlock = fragmentInterfaceBlocks[blockIndex];
        UniformBlockMap::const_iterator entry = linkedUniformBlocks.find(fragmentInterfaceBlock.name);
        if (entry != linkedUniformBlocks.end())
        {
            const sh::InterfaceBlock &vertexInterfaceBlock = *entry->second;
            if (!areMatchingInterfaceBlocks(infoLog, vertexInterfaceBlock, fragmentInterfaceBlock))
            {
                return false;
            }
        }
    }

    for (unsigned int blockIndex = 0; blockIndex < vertexInterfaceBlocks.size(); blockIndex++)
    {
        if (!defineUniformBlock(infoLog, GL_VERTEX_SHADER, vertexInterfaceBlocks[blockIndex]))
        {
            return false;
        }
    }

    for (unsigned int blockIndex = 0; blockIndex < fragmentInterfaceBlocks.size(); blockIndex++)
    {
        if (!defineUniformBlock(infoLog, GL_FRAGMENT_SHADER, fragmentInterfaceBlocks[blockIndex]))
        {
            return false;
        }
    }

    return true;
}

void ProgramBinary::defineUniformBlockMembers(const std::vector<sh::InterfaceBlockField> &fields, const std::string &prefix, int blockIndex, BlockInfoItr *blockInfoItr, std::vector<unsigned int> *blockUniformIndexes)
{
    for (unsigned int uniformIndex = 0; uniformIndex < fields.size(); uniformIndex++)
    {
        const sh::InterfaceBlockField &field = fields[uniformIndex];
        const std::string &fieldName = (prefix.empty() ? field.name : prefix + "." + field.name);

        if (!field.fields.empty())
        {
            if (field.arraySize > 0)
            {
                for (unsigned int arrayElement = 0; arrayElement < field.arraySize; arrayElement++)
                {
                    const std::string uniformElementName = fieldName + ArrayString(arrayElement);
                    defineUniformBlockMembers(field.fields, uniformElementName, blockIndex, blockInfoItr, blockUniformIndexes);
                }
            }
            else
            {
                defineUniformBlockMembers(field.fields, fieldName, blockIndex, blockInfoItr, blockUniformIndexes);
            }
        }
        else
        {
            Uniform *newUniform = new Uniform(field.type, field.precision, fieldName, field.arraySize,
                                              blockIndex, **blockInfoItr);

            // add to uniform list, but not index, since uniform block uniforms have no location
            blockUniformIndexes->push_back(mUniforms.size());
            mUniforms.push_back(newUniform);
            (*blockInfoItr)++;
        }
    }
}

bool ProgramBinary::defineUniformBlock(InfoLog &infoLog, GLenum shader, const sh::InterfaceBlock &interfaceBlock)
{
    // create uniform block entries if they do not exist
    if (getUniformBlockIndex(interfaceBlock.name) == GL_INVALID_INDEX)
    {
        std::vector<unsigned int> blockUniformIndexes;
        const unsigned int blockIndex = mUniformBlocks.size();

        // define member uniforms
        BlockInfoItr blockInfoItr = interfaceBlock.blockInfo.cbegin();
        defineUniformBlockMembers(interfaceBlock.fields, "", blockIndex, &blockInfoItr, &blockUniformIndexes);

        // create all the uniform blocks
        if (interfaceBlock.arraySize > 0)
        {
            for (unsigned int uniformBlockElement = 0; uniformBlockElement < interfaceBlock.arraySize; uniformBlockElement++)
            {
                gl::UniformBlock *newUniformBlock = new UniformBlock(interfaceBlock.name, uniformBlockElement, interfaceBlock.dataSize);
                newUniformBlock->memberUniformIndexes = blockUniformIndexes;
                mUniformBlocks.push_back(newUniformBlock);
            }
        }
        else
        {
            gl::UniformBlock *newUniformBlock = new UniformBlock(interfaceBlock.name, GL_INVALID_INDEX, interfaceBlock.dataSize);
            newUniformBlock->memberUniformIndexes = blockUniformIndexes;
            mUniformBlocks.push_back(newUniformBlock);
        }
    }

    // Assign registers to the uniform blocks
    const GLuint blockIndex = getUniformBlockIndex(interfaceBlock.name);
    const unsigned int elementCount = std::max(1u, interfaceBlock.arraySize);
    ASSERT(blockIndex != GL_INVALID_INDEX);
    ASSERT(blockIndex + elementCount <= mUniformBlocks.size());

    for (unsigned int uniformBlockElement = 0; uniformBlockElement < elementCount; uniformBlockElement++)
    {
        gl::UniformBlock *uniformBlock = mUniformBlocks[blockIndex + uniformBlockElement];
        ASSERT(uniformBlock->name == interfaceBlock.name);

        if (!assignUniformBlockRegister(infoLog, uniformBlock, shader, interfaceBlock.registerIndex + uniformBlockElement))
        {
            return false;
        }
    }

    return true;
}

bool ProgramBinary::assignUniformBlockRegister(InfoLog &infoLog, UniformBlock *uniformBlock, GLenum shader, unsigned int registerIndex)
{
    if (shader == GL_VERTEX_SHADER)
    {
        uniformBlock->vsRegisterIndex = registerIndex;
        unsigned int maximumBlocks = mRenderer->getMaxVertexShaderUniformBuffers();

        if (registerIndex - mRenderer->getReservedVertexUniformBuffers() >= maximumBlocks)
        {
            infoLog.append("Vertex shader uniform block count exceed GL_MAX_VERTEX_UNIFORM_BLOCKS (%u)", maximumBlocks);
            return false;
        }
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        uniformBlock->psRegisterIndex = registerIndex;
        unsigned int maximumBlocks = mRenderer->getMaxFragmentShaderUniformBuffers();

        if (registerIndex - mRenderer->getReservedFragmentUniformBuffers() >= maximumBlocks)
        {
            infoLog.append("Fragment shader uniform block count exceed GL_MAX_FRAGMENT_UNIFORM_BLOCKS (%u)", maximumBlocks);
            return false;
        }
    }
    else UNREACHABLE();

    return true;
}

bool ProgramBinary::isValidated() const 
{
    return mValidated;
}

void ProgramBinary::getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    // Skip over inactive attributes
    unsigned int activeAttribute = 0;
    unsigned int attribute;
    for (attribute = 0; attribute < MAX_VERTEX_ATTRIBS; attribute++)
    {
        if (mLinkedAttribute[attribute].name.empty())
        {
            continue;
        }

        if (activeAttribute == index)
        {
            break;
        }

        activeAttribute++;
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

GLint ProgramBinary::getActiveAttributeCount() const
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

GLint ProgramBinary::getActiveAttributeMaxLength() const
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

void ProgramBinary::getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const
{
    ASSERT(index < mUniforms.size());   // index must be smaller than getActiveUniformCount()

    if (bufsize > 0)
    {
        std::string string = mUniforms[index]->name;

        if (mUniforms[index]->isArray())
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

    *size = mUniforms[index]->elementCount();

    *type = mUniforms[index]->type;
}

GLint ProgramBinary::getActiveUniformCount() const
{
    return mUniforms.size();
}

GLint ProgramBinary::getActiveUniformMaxLength() const
{
    int maxLength = 0;

    unsigned int numUniforms = mUniforms.size();
    for (unsigned int uniformIndex = 0; uniformIndex < numUniforms; uniformIndex++)
    {
        if (!mUniforms[uniformIndex]->name.empty())
        {
            int length = (int)(mUniforms[uniformIndex]->name.length() + 1);
            if (mUniforms[uniformIndex]->isArray())
            {
                length += 3;  // Counting in "[0]".
            }
            maxLength = std::max(length, maxLength);
        }
    }

    return maxLength;
}

GLint ProgramBinary::getActiveUniformi(GLuint index, GLenum pname) const
{
    const gl::Uniform& uniform = *mUniforms[index];

    switch (pname)
    {
      case GL_UNIFORM_TYPE:         return static_cast<GLint>(uniform.type);
      case GL_UNIFORM_SIZE:         return static_cast<GLint>(uniform.elementCount());
      case GL_UNIFORM_NAME_LENGTH:  return static_cast<GLint>(uniform.name.size() + 1 + (uniform.isArray() ? 3 : 0));
      case GL_UNIFORM_BLOCK_INDEX:  return uniform.blockIndex;

      case GL_UNIFORM_OFFSET:       return uniform.blockInfo.offset;
      case GL_UNIFORM_ARRAY_STRIDE: return uniform.blockInfo.arrayStride;
      case GL_UNIFORM_MATRIX_STRIDE: return uniform.blockInfo.matrixStride;
      case GL_UNIFORM_IS_ROW_MAJOR: return static_cast<GLint>(uniform.blockInfo.isRowMajorMatrix);

      default:
        UNREACHABLE();
        break;
    }
    return 0;
}

void ProgramBinary::getActiveUniformBlockName(GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) const
{
    ASSERT(uniformBlockIndex < mUniformBlocks.size());   // index must be smaller than getActiveUniformBlockCount()

    const UniformBlock &uniformBlock = *mUniformBlocks[uniformBlockIndex];

    if (bufSize > 0)
    {
        std::string string = uniformBlock.name;

        if (uniformBlock.isArrayElement())
        {
            string += ArrayString(uniformBlock.elementIndex);
        }

        strncpy(uniformBlockName, string.c_str(), bufSize);
        uniformBlockName[bufSize - 1] = '\0';

        if (length)
        {
            *length = strlen(uniformBlockName);
        }
    }
}

void ProgramBinary::getActiveUniformBlockiv(GLuint uniformBlockIndex, GLenum pname, GLint *params) const
{
    ASSERT(uniformBlockIndex < mUniformBlocks.size());   // index must be smaller than getActiveUniformBlockCount()

    const UniformBlock &uniformBlock = *mUniformBlocks[uniformBlockIndex];

    switch (pname)
    {
      case GL_UNIFORM_BLOCK_DATA_SIZE:
        *params = static_cast<GLint>(uniformBlock.dataSize);
        break;
      case GL_UNIFORM_BLOCK_NAME_LENGTH:
        *params = static_cast<GLint>(uniformBlock.name.size() + 1 + (uniformBlock.isArrayElement() ? 3 : 0));
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS:
        *params = static_cast<GLint>(uniformBlock.memberUniformIndexes.size());
        break;
      case GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES:
        {
            for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
            {
                params[blockMemberIndex] = static_cast<GLint>(uniformBlock.memberUniformIndexes[blockMemberIndex]);
            }
        }
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_VERTEX_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByVertexShader());
        break;
      case GL_UNIFORM_BLOCK_REFERENCED_BY_FRAGMENT_SHADER:
        *params = static_cast<GLint>(uniformBlock.isReferencedByFragmentShader());
        break;
      default: UNREACHABLE();
    }
}

GLuint ProgramBinary::getActiveUniformBlockCount() const
{
    return mUniformBlocks.size();
}

GLuint ProgramBinary::getActiveUniformBlockMaxLength() const
{
    unsigned int maxLength = 0;

    unsigned int numUniformBlocks = mUniformBlocks.size();
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < numUniformBlocks; uniformBlockIndex++)
    {
        const UniformBlock &uniformBlock = *mUniformBlocks[uniformBlockIndex];
        if (!uniformBlock.name.empty())
        {
            const unsigned int length = uniformBlock.name.length() + 1;

            // Counting in "[0]".
            const unsigned int arrayLength = (uniformBlock.isArrayElement() ? 3 : 0);

            maxLength = std::max(length + arrayLength, maxLength);
        }
    }

    return maxLength;
}

void ProgramBinary::validate(InfoLog &infoLog)
{
    applyUniforms();
    if (!validateSamplers(&infoLog))
    {
        mValidated = false;
    }
    else
    {
        mValidated = true;
    }
}

bool ProgramBinary::validateSamplers(InfoLog *infoLog)
{
    // if any two active samplers in a program are of different types, but refer to the same
    // texture image unit, and this is the current program, then ValidateProgram will fail, and
    // DrawArrays and DrawElements will issue the INVALID_OPERATION error.

    const unsigned int maxCombinedTextureImageUnits = mRenderer->getMaxCombinedTextureImageUnits();
    TextureType textureUnitType[IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS];

    for (unsigned int i = 0; i < IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS; ++i)
    {
        textureUnitType[i] = TEXTURE_UNKNOWN;
    }

    for (unsigned int i = 0; i < mUsedPixelSamplerRange; ++i)
    {
        if (mSamplersPS[i].active)
        {
            unsigned int unit = mSamplersPS[i].logicalTextureUnit;
            
            if (unit >= maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, maxCombinedTextureImageUnits);
                }

                return false;
            }

            if (textureUnitType[unit] != TEXTURE_UNKNOWN)
            {
                if (mSamplersPS[i].textureType != textureUnitType[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitType[unit] = mSamplersPS[i].textureType;
            }
        }
    }

    for (unsigned int i = 0; i < mUsedVertexSamplerRange; ++i)
    {
        if (mSamplersVS[i].active)
        {
            unsigned int unit = mSamplersVS[i].logicalTextureUnit;
            
            if (unit >= maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    infoLog->append("Sampler uniform (%d) exceeds IMPLEMENTATION_MAX_COMBINED_TEXTURE_IMAGE_UNITS (%d)", unit, maxCombinedTextureImageUnits);
                }

                return false;
            }

            if (textureUnitType[unit] != TEXTURE_UNKNOWN)
            {
                if (mSamplersVS[i].textureType != textureUnitType[unit])
                {
                    if (infoLog)
                    {
                        infoLog->append("Samplers of conflicting types refer to the same texture image unit (%d).", unit);
                    }

                    return false;
                }
            }
            else
            {
                textureUnitType[unit] = mSamplersVS[i].textureType;
            }
        }
    }

    return true;
}

ProgramBinary::Sampler::Sampler() : active(false), logicalTextureUnit(0), textureType(TEXTURE_2D)
{
}

struct AttributeSorter
{
    AttributeSorter(const int (&semanticIndices)[MAX_VERTEX_ATTRIBS])
        : originalIndices(semanticIndices)
    {
    }

    bool operator()(int a, int b)
    {
        if (originalIndices[a] == -1) return false;
        if (originalIndices[b] == -1) return true;
        return (originalIndices[a] < originalIndices[b]);
    }

    const int (&originalIndices)[MAX_VERTEX_ATTRIBS];
};

void ProgramBinary::initAttributesByLayout()
{
    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        mAttributesByLayout[i] = i;
    }

    std::sort(&mAttributesByLayout[0], &mAttributesByLayout[MAX_VERTEX_ATTRIBS], AttributeSorter(mSemanticIndex));
}

void ProgramBinary::sortAttributesByLayout(rx::TranslatedAttribute attributes[MAX_VERTEX_ATTRIBS], int sortedSemanticIndices[MAX_VERTEX_ATTRIBS]) const
{
    rx::TranslatedAttribute oldTranslatedAttributes[MAX_VERTEX_ATTRIBS];

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        oldTranslatedAttributes[i] = attributes[i];
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        int oldIndex = mAttributesByLayout[i];
        sortedSemanticIndices[i] = mSemanticIndex[oldIndex];
        attributes[i] = oldTranslatedAttributes[oldIndex];
    }
}

void ProgramBinary::initializeUniformStorage()
{
    // Compute total default block size
    unsigned int vertexRegisters = 0;
    unsigned int fragmentRegisters = 0;
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const Uniform &uniform = *mUniforms[uniformIndex];

        if (!IsSampler(uniform.type))
        {
            if (uniform.isReferencedByVertexShader())
            {
                vertexRegisters = std::max(vertexRegisters, uniform.vsRegisterIndex + uniform.registerCount);
            }
            if (uniform.isReferencedByFragmentShader())
            {
                fragmentRegisters = std::max(fragmentRegisters, uniform.psRegisterIndex + uniform.registerCount);
            }
        }
    }

    mVertexUniformStorage = mRenderer->createUniformStorage(vertexRegisters * 16u);
    mFragmentUniformStorage = mRenderer->createUniformStorage(fragmentRegisters * 16u);
}

}
