//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramD3D.cpp: Defines the rx::ProgramD3D class which implements rx::ProgramImpl.

#include "libANGLE/renderer/d3d/ProgramD3D.h"

#include "common/utilities.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/Program.h"
#include "libANGLE/VertexArray.h"
#include "libANGLE/features.h"
#include "libANGLE/renderer/d3d/DynamicHLSL.h"
#include "libANGLE/renderer/d3d/FramebufferD3D.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/ShaderD3D.h"
#include "libANGLE/renderer/d3d/ShaderExecutableD3D.h"
#include "libANGLE/renderer/d3d/VertexDataManager.h"

namespace rx
{

namespace
{

GLenum GetTextureType(GLenum samplerType)
{
    switch (samplerType)
    {
      case GL_SAMPLER_2D:
      case GL_INT_SAMPLER_2D:
      case GL_UNSIGNED_INT_SAMPLER_2D:
      case GL_SAMPLER_2D_SHADOW:
        return GL_TEXTURE_2D;
      case GL_SAMPLER_3D:
      case GL_INT_SAMPLER_3D:
      case GL_UNSIGNED_INT_SAMPLER_3D:
        return GL_TEXTURE_3D;
      case GL_SAMPLER_CUBE:
      case GL_SAMPLER_CUBE_SHADOW:
        return GL_TEXTURE_CUBE_MAP;
      case GL_INT_SAMPLER_CUBE:
      case GL_UNSIGNED_INT_SAMPLER_CUBE:
        return GL_TEXTURE_CUBE_MAP;
      case GL_SAMPLER_2D_ARRAY:
      case GL_INT_SAMPLER_2D_ARRAY:
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GL_SAMPLER_2D_ARRAY_SHADOW:
        return GL_TEXTURE_2D_ARRAY;
      default: UNREACHABLE();
    }

    return GL_TEXTURE_2D;
}

gl::InputLayout GetDefaultInputLayoutFromShader(const gl::Shader *vertexShader)
{
    gl::InputLayout defaultLayout;
    for (const sh::Attribute &shaderAttr : vertexShader->getActiveAttributes())
    {
        if (shaderAttr.type != GL_NONE)
        {
            GLenum transposedType = gl::TransposeMatrixType(shaderAttr.type);

            for (size_t rowIndex = 0;
                 static_cast<int>(rowIndex) < gl::VariableRowCount(transposedType);
                 ++rowIndex)
            {
                GLenum componentType = gl::VariableComponentType(transposedType);
                GLuint components = static_cast<GLuint>(gl::VariableColumnCount(transposedType));
                bool pureInt = (componentType != GL_FLOAT);
                gl::VertexFormatType defaultType = gl::GetVertexFormatType(
                    componentType, GL_FALSE, components, pureInt);

                defaultLayout.push_back(defaultType);
            }
        }
    }

    return defaultLayout;
}

std::vector<GLenum> GetDefaultOutputLayoutFromShader(const std::vector<PixelShaderOutputVariable> &shaderOutputVars)
{
    std::vector<GLenum> defaultPixelOutput;

    if (!shaderOutputVars.empty())
    {
        defaultPixelOutput.push_back(GL_COLOR_ATTACHMENT0 +
                                     static_cast<unsigned int>(shaderOutputVars[0].outputIndex));
    }

    return defaultPixelOutput;
}

bool IsRowMajorLayout(const sh::InterfaceBlockField &var)
{
    return var.isRowMajorLayout;
}

bool IsRowMajorLayout(const sh::ShaderVariable &var)
{
    return false;
}

struct AttributeSorter
{
    AttributeSorter(const ProgramImpl::SemanticIndexArray &semanticIndices)
        : originalIndices(&semanticIndices)
    {
    }

    bool operator()(int a, int b)
    {
        int indexA = (*originalIndices)[a];
        int indexB = (*originalIndices)[b];

        if (indexA == -1) return false;
        if (indexB == -1) return true;
        return (indexA < indexB);
    }

    const ProgramImpl::SemanticIndexArray *originalIndices;
};

bool LinkVaryingRegisters(gl::InfoLog &infoLog,
                          ShaderD3D *vertexShaderD3D,
                          ShaderD3D *fragmentShaderD3D)
{
    for (gl::PackedVarying &input : fragmentShaderD3D->getVaryings())
    {
        bool matched = false;

        // Built-in varyings obey special rules
        if (input.isBuiltIn())
        {
            continue;
        }

        for (gl::PackedVarying &output : vertexShaderD3D->getVaryings())
        {
            if (output.name == input.name)
            {
                output.registerIndex = input.registerIndex;
                output.columnIndex   = input.columnIndex;

                matched = true;
                break;
            }
        }

        // We permit unmatched, unreferenced varyings
        ASSERT(matched || !input.staticUse);
    }

    return true;
}

}  // anonymous namespace

ProgramD3D::VertexExecutable::VertexExecutable(const gl::InputLayout &inputLayout,
                                               const Signature &signature,
                                               ShaderExecutableD3D *shaderExecutable)
    : mInputs(inputLayout),
      mSignature(signature),
      mShaderExecutable(shaderExecutable)
{
}

ProgramD3D::VertexExecutable::~VertexExecutable()
{
    SafeDelete(mShaderExecutable);
}

// static
void ProgramD3D::VertexExecutable::getSignature(RendererD3D *renderer,
                                                const gl::InputLayout &inputLayout,
                                                Signature *signatureOut)
{
    signatureOut->resize(inputLayout.size());

    for (size_t index = 0; index < inputLayout.size(); ++index)
    {
        gl::VertexFormatType vertexFormatType = inputLayout[index];
        bool converted = false;
        if (vertexFormatType != gl::VERTEX_FORMAT_INVALID)
        {
            VertexConversionType conversionType =
                renderer->getVertexConversionType(vertexFormatType);
            converted = ((conversionType & VERTEX_CONVERT_GPU) != 0);
        }

        (*signatureOut)[index] = converted;
    }
}

bool ProgramD3D::VertexExecutable::matchesSignature(const Signature &signature) const
{
    size_t limit = std::max(mSignature.size(), signature.size());
    for (size_t index = 0; index < limit; ++index)
    {
        // treat undefined indexes as 'not converted'
        bool a = index < signature.size() ? signature[index] : false;
        bool b = index < mSignature.size() ? mSignature[index] : false;
        if (a != b)
            return false;
    }

    return true;
}

ProgramD3D::PixelExecutable::PixelExecutable(const std::vector<GLenum> &outputSignature,
                                             ShaderExecutableD3D *shaderExecutable)
    : mOutputSignature(outputSignature),
      mShaderExecutable(shaderExecutable)
{
}

ProgramD3D::PixelExecutable::~PixelExecutable()
{
    SafeDelete(mShaderExecutable);
}

ProgramD3D::Sampler::Sampler() : active(false), logicalTextureUnit(0), textureType(GL_TEXTURE_2D)
{
}

unsigned int ProgramD3D::mCurrentSerial = 1;

ProgramD3D::ProgramD3D(const gl::Program::Data &data, RendererD3D *renderer)
    : ProgramImpl(data),
      mRenderer(renderer),
      mDynamicHLSL(NULL),
      mGeometryExecutable(NULL),
      mUsesPointSize(false),
      mVertexUniformStorage(NULL),
      mFragmentUniformStorage(NULL),
      mUsedVertexSamplerRange(0),
      mUsedPixelSamplerRange(0),
      mDirtySamplerMapping(true),
      mTextureUnitTypesCache(renderer->getRendererCaps().maxCombinedTextureImageUnits),
      mShaderVersion(100),
      mSerial(issueSerial())
{
    mDynamicHLSL = new DynamicHLSL(renderer);
}

ProgramD3D::~ProgramD3D()
{
    reset();
    SafeDelete(mDynamicHLSL);
}

bool ProgramD3D::usesPointSpriteEmulation() const
{
    return mUsesPointSize && mRenderer->getMajorShaderModel() >= 4;
}

bool ProgramD3D::usesGeometryShader() const
{
    return usesPointSpriteEmulation() && !usesInstancedPointSpriteEmulation();
}

bool ProgramD3D::usesInstancedPointSpriteEmulation() const
{
    return mRenderer->getWorkarounds().useInstancedPointSpriteEmulation;
}

GLint ProgramD3D::getSamplerMapping(gl::SamplerType type, unsigned int samplerIndex, const gl::Caps &caps) const
{
    GLint logicalTextureUnit = -1;

    switch (type)
    {
      case gl::SAMPLER_PIXEL:
        ASSERT(samplerIndex < caps.maxTextureImageUnits);
        if (samplerIndex < mSamplersPS.size() && mSamplersPS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersPS[samplerIndex].logicalTextureUnit;
        }
        break;
      case gl::SAMPLER_VERTEX:
        ASSERT(samplerIndex < caps.maxVertexTextureImageUnits);
        if (samplerIndex < mSamplersVS.size() && mSamplersVS[samplerIndex].active)
        {
            logicalTextureUnit = mSamplersVS[samplerIndex].logicalTextureUnit;
        }
        break;
      default: UNREACHABLE();
    }

    if (logicalTextureUnit >= 0 && logicalTextureUnit < static_cast<GLint>(caps.maxCombinedTextureImageUnits))
    {
        return logicalTextureUnit;
    }

    return -1;
}

// Returns the texture type for a given Direct3D 9 sampler type and
// index (0-15 for the pixel shader and 0-3 for the vertex shader).
GLenum ProgramD3D::getSamplerTextureType(gl::SamplerType type, unsigned int samplerIndex) const
{
    switch (type)
    {
      case gl::SAMPLER_PIXEL:
        ASSERT(samplerIndex < mSamplersPS.size());
        ASSERT(mSamplersPS[samplerIndex].active);
        return mSamplersPS[samplerIndex].textureType;
      case gl::SAMPLER_VERTEX:
        ASSERT(samplerIndex < mSamplersVS.size());
        ASSERT(mSamplersVS[samplerIndex].active);
        return mSamplersVS[samplerIndex].textureType;
      default: UNREACHABLE();
    }

    return GL_TEXTURE_2D;
}

GLint ProgramD3D::getUsedSamplerRange(gl::SamplerType type) const
{
    switch (type)
    {
      case gl::SAMPLER_PIXEL:
        return mUsedPixelSamplerRange;
      case gl::SAMPLER_VERTEX:
        return mUsedVertexSamplerRange;
      default:
        UNREACHABLE();
        return 0;
    }
}

void ProgramD3D::updateSamplerMapping()
{
    if (!mDirtySamplerMapping)
    {
        return;
    }

    mDirtySamplerMapping = false;

    // Retrieve sampler uniform values
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        gl::LinkedUniform *targetUniform = mUniforms[uniformIndex];

        if (targetUniform->dirty)
        {
            if (gl::IsSamplerType(targetUniform->type))
            {
                int count = targetUniform->elementCount();
                GLint (*v)[4] = reinterpret_cast<GLint(*)[4]>(targetUniform->data);

                if (targetUniform->isReferencedByFragmentShader())
                {
                    unsigned int firstIndex = targetUniform->psRegisterIndex;

                    for (int i = 0; i < count; i++)
                    {
                        unsigned int samplerIndex = firstIndex + i;

                        if (samplerIndex < mSamplersPS.size())
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

                        if (samplerIndex < mSamplersVS.size())
                        {
                            ASSERT(mSamplersVS[samplerIndex].active);
                            mSamplersVS[samplerIndex].logicalTextureUnit = v[i][0];
                        }
                    }
                }
            }
        }
    }
}

bool ProgramD3D::validateSamplers(gl::InfoLog *infoLog, const gl::Caps &caps)
{
    // Skip cache if we're using an infolog, so we get the full error.
    // Also skip the cache if the sample mapping has changed, or if we haven't ever validated.
    if (!mDirtySamplerMapping && infoLog == nullptr && mCachedValidateSamplersResult.valid())
    {
        return mCachedValidateSamplersResult.value();
    }

    // if any two active samplers in a program are of different types, but refer to the same
    // texture image unit, and this is the current program, then ValidateProgram will fail, and
    // DrawArrays and DrawElements will issue the INVALID_OPERATION error.
    updateSamplerMapping();

    std::fill(mTextureUnitTypesCache.begin(), mTextureUnitTypesCache.end(), GL_NONE);

    for (unsigned int i = 0; i < mUsedPixelSamplerRange; ++i)
    {
        if (mSamplersPS[i].active)
        {
            unsigned int unit = mSamplersPS[i].logicalTextureUnit;

            if (unit >= caps.maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    (*infoLog) << "Sampler uniform (" << unit
                               << ") exceeds GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS ("
                               << caps.maxCombinedTextureImageUnits << ")";
                }

                mCachedValidateSamplersResult = false;
                return false;
            }

            if (mTextureUnitTypesCache[unit] != GL_NONE)
            {
                if (mSamplersPS[i].textureType != mTextureUnitTypesCache[unit])
                {
                    if (infoLog)
                    {
                        (*infoLog) << "Samplers of conflicting types refer to the same texture image unit ("
                                   << unit << ").";
                    }

                    mCachedValidateSamplersResult = false;
                    return false;
                }
            }
            else
            {
                mTextureUnitTypesCache[unit] = mSamplersPS[i].textureType;
            }
        }
    }

    for (unsigned int i = 0; i < mUsedVertexSamplerRange; ++i)
    {
        if (mSamplersVS[i].active)
        {
            unsigned int unit = mSamplersVS[i].logicalTextureUnit;

            if (unit >= caps.maxCombinedTextureImageUnits)
            {
                if (infoLog)
                {
                    (*infoLog) << "Sampler uniform (" << unit
                               << ") exceeds GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS ("
                               << caps.maxCombinedTextureImageUnits << ")";
                }

                mCachedValidateSamplersResult = false;
                return false;
            }

            if (mTextureUnitTypesCache[unit] != GL_NONE)
            {
                if (mSamplersVS[i].textureType != mTextureUnitTypesCache[unit])
                {
                    if (infoLog)
                    {
                        (*infoLog) << "Samplers of conflicting types refer to the same texture image unit ("
                                   << unit << ").";
                    }

                    mCachedValidateSamplersResult = false;
                    return false;
                }
            }
            else
            {
                mTextureUnitTypesCache[unit] = mSamplersVS[i].textureType;
            }
        }
    }

    mCachedValidateSamplersResult = true;
    return true;
}

LinkResult ProgramD3D::load(gl::InfoLog &infoLog, gl::BinaryInputStream *stream)
{
    DeviceIdentifier binaryDeviceIdentifier = { 0 };
    stream->readBytes(reinterpret_cast<unsigned char*>(&binaryDeviceIdentifier), sizeof(DeviceIdentifier));

    DeviceIdentifier identifier = mRenderer->getAdapterIdentifier();
    if (memcmp(&identifier, &binaryDeviceIdentifier, sizeof(DeviceIdentifier)) != 0)
    {
        infoLog << "Invalid program binary, device configuration has changed.";
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    int compileFlags = stream->readInt<int>();
    if (compileFlags != ANGLE_COMPILE_OPTIMIZATION_LEVEL)
    {
        infoLog << "Mismatched compilation flags.";
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    stream->readInt(&mShaderVersion);

    const unsigned int psSamplerCount = stream->readInt<unsigned int>();
    for (unsigned int i = 0; i < psSamplerCount; ++i)
    {
        Sampler sampler;
        stream->readBool(&sampler.active);
        stream->readInt(&sampler.logicalTextureUnit);
        stream->readInt(&sampler.textureType);
        mSamplersPS.push_back(sampler);
    }
    const unsigned int vsSamplerCount = stream->readInt<unsigned int>();
    for (unsigned int i = 0; i < vsSamplerCount; ++i)
    {
        Sampler sampler;
        stream->readBool(&sampler.active);
        stream->readInt(&sampler.logicalTextureUnit);
        stream->readInt(&sampler.textureType);
        mSamplersVS.push_back(sampler);
    }

    stream->readInt(&mUsedVertexSamplerRange);
    stream->readInt(&mUsedPixelSamplerRange);

    const unsigned int uniformCount = stream->readInt<unsigned int>();
    if (stream->error())
    {
        infoLog << "Invalid program binary.";
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    mUniforms.resize(uniformCount);
    for (unsigned int uniformIndex = 0; uniformIndex < uniformCount; uniformIndex++)
    {
        GLenum type = stream->readInt<GLenum>();
        GLenum precision = stream->readInt<GLenum>();
        std::string name = stream->readString();
        unsigned int arraySize = stream->readInt<unsigned int>();
        int blockIndex = stream->readInt<int>();

        int offset = stream->readInt<int>();
        int arrayStride = stream->readInt<int>();
        int matrixStride = stream->readInt<int>();
        bool isRowMajorMatrix = stream->readBool();

        const sh::BlockMemberInfo blockInfo(offset, arrayStride, matrixStride, isRowMajorMatrix);

        gl::LinkedUniform *uniform = new gl::LinkedUniform(type, precision, name, arraySize, blockIndex, blockInfo);

        stream->readInt(&uniform->psRegisterIndex);
        stream->readInt(&uniform->vsRegisterIndex);
        stream->readInt(&uniform->registerCount);
        stream->readInt(&uniform->registerElement);

        mUniforms[uniformIndex] = uniform;
    }

    const unsigned int uniformIndexCount = stream->readInt<unsigned int>();
    if (stream->error())
    {
        infoLog << "Invalid program binary.";
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    for (unsigned int uniformIndexIndex = 0; uniformIndexIndex < uniformIndexCount; uniformIndexIndex++)
    {
        GLuint location;
        stream->readInt(&location);

        gl::VariableLocation variable;
        stream->readString(&variable.name);
        stream->readInt(&variable.element);
        stream->readInt(&variable.index);

        mUniformIndex[location] = variable;
    }

    unsigned int uniformBlockCount = stream->readInt<unsigned int>();
    if (stream->error())
    {
        infoLog << "Invalid program binary.";
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    mUniformBlocks.resize(uniformBlockCount);
    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < uniformBlockCount; ++uniformBlockIndex)
    {
        std::string name = stream->readString();
        unsigned int elementIndex = stream->readInt<unsigned int>();
        unsigned int dataSize = stream->readInt<unsigned int>();

        gl::UniformBlock *uniformBlock = new gl::UniformBlock(name, elementIndex, dataSize);

        stream->readInt(&uniformBlock->psRegisterIndex);
        stream->readInt(&uniformBlock->vsRegisterIndex);

        unsigned int numMembers = stream->readInt<unsigned int>();
        uniformBlock->memberUniformIndexes.resize(numMembers);
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < numMembers; blockMemberIndex++)
        {
            stream->readInt(&uniformBlock->memberUniformIndexes[blockMemberIndex]);
        }

        mUniformBlocks[uniformBlockIndex] = uniformBlock;
    }

    const unsigned int transformFeedbackVaryingCount = stream->readInt<unsigned int>();
    mTransformFeedbackLinkedVaryings.resize(transformFeedbackVaryingCount);
    for (unsigned int varyingIndex = 0; varyingIndex < transformFeedbackVaryingCount; varyingIndex++)
    {
        gl::LinkedVarying &varying = mTransformFeedbackLinkedVaryings[varyingIndex];

        stream->readString(&varying.name);
        stream->readInt(&varying.type);
        stream->readInt(&varying.size);
        stream->readString(&varying.semanticName);
        stream->readInt(&varying.semanticIndex);
        stream->readInt(&varying.semanticIndexCount);
    }

    stream->readString(&mVertexHLSL);
    stream->readBytes(reinterpret_cast<unsigned char*>(&mVertexWorkarounds), sizeof(D3DCompilerWorkarounds));
    stream->readString(&mPixelHLSL);
    stream->readBytes(reinterpret_cast<unsigned char*>(&mPixelWorkarounds), sizeof(D3DCompilerWorkarounds));
    stream->readBool(&mUsesFragDepth);
    stream->readBool(&mUsesPointSize);

    const size_t pixelShaderKeySize = stream->readInt<unsigned int>();
    mPixelShaderKey.resize(pixelShaderKeySize);
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < pixelShaderKeySize; pixelShaderKeyIndex++)
    {
        stream->readInt(&mPixelShaderKey[pixelShaderKeyIndex].type);
        stream->readString(&mPixelShaderKey[pixelShaderKeyIndex].name);
        stream->readString(&mPixelShaderKey[pixelShaderKeyIndex].source);
        stream->readInt(&mPixelShaderKey[pixelShaderKeyIndex].outputIndex);
    }

    const unsigned char* binary = reinterpret_cast<const unsigned char*>(stream->data());

    const unsigned int vertexShaderCount = stream->readInt<unsigned int>();
    for (unsigned int vertexShaderIndex = 0; vertexShaderIndex < vertexShaderCount; vertexShaderIndex++)
    {
        size_t inputLayoutSize = stream->readInt<size_t>();
        gl::InputLayout inputLayout(inputLayoutSize, gl::VERTEX_FORMAT_INVALID);

        for (size_t inputIndex = 0; inputIndex < inputLayoutSize; inputIndex++)
        {
            inputLayout[inputIndex] = stream->readInt<gl::VertexFormatType>();
        }

        unsigned int vertexShaderSize = stream->readInt<unsigned int>();
        const unsigned char *vertexShaderFunction = binary + stream->offset();

        ShaderExecutableD3D *shaderExecutable = nullptr;

        gl::Error error = mRenderer->loadExecutable(
            vertexShaderFunction, vertexShaderSize, SHADER_VERTEX, mTransformFeedbackLinkedVaryings,
            (mData.getTransformFeedbackBufferMode() == GL_SEPARATE_ATTRIBS), &shaderExecutable);
        if (error.isError())
        {
            return LinkResult(false, error);
        }

        if (!shaderExecutable)
        {
            infoLog << "Could not create vertex shader.";
            return LinkResult(false, gl::Error(GL_NO_ERROR));
        }

        // generated converted input layout
        VertexExecutable::Signature signature;
        VertexExecutable::getSignature(mRenderer, inputLayout, &signature);

        // add new binary
        mVertexExecutables.push_back(new VertexExecutable(inputLayout, signature, shaderExecutable));

        stream->skip(vertexShaderSize);
    }

    const size_t pixelShaderCount = stream->readInt<unsigned int>();
    for (size_t pixelShaderIndex = 0; pixelShaderIndex < pixelShaderCount; pixelShaderIndex++)
    {
        const size_t outputCount = stream->readInt<unsigned int>();
        std::vector<GLenum> outputs(outputCount);
        for (size_t outputIndex = 0; outputIndex < outputCount; outputIndex++)
        {
            stream->readInt(&outputs[outputIndex]);
        }

        const size_t pixelShaderSize = stream->readInt<unsigned int>();
        const unsigned char *pixelShaderFunction = binary + stream->offset();
        ShaderExecutableD3D *shaderExecutable    = nullptr;

        gl::Error error = mRenderer->loadExecutable(
            pixelShaderFunction, pixelShaderSize, SHADER_PIXEL, mTransformFeedbackLinkedVaryings,
            (mData.getTransformFeedbackBufferMode() == GL_SEPARATE_ATTRIBS), &shaderExecutable);
        if (error.isError())
        {
            return LinkResult(false, error);
        }

        if (!shaderExecutable)
        {
            infoLog << "Could not create pixel shader.";
            return LinkResult(false, gl::Error(GL_NO_ERROR));
        }

        // add new binary
        mPixelExecutables.push_back(new PixelExecutable(outputs, shaderExecutable));

        stream->skip(pixelShaderSize);
    }

    unsigned int geometryShaderSize = stream->readInt<unsigned int>();

    if (geometryShaderSize > 0)
    {
        const unsigned char *geometryShaderFunction = binary + stream->offset();
        gl::Error error                             = mRenderer->loadExecutable(
            geometryShaderFunction, geometryShaderSize, SHADER_GEOMETRY,
            mTransformFeedbackLinkedVaryings,
            (mData.getTransformFeedbackBufferMode() == GL_SEPARATE_ATTRIBS), &mGeometryExecutable);
        if (error.isError())
        {
            return LinkResult(false, error);
        }

        if (!mGeometryExecutable)
        {
            infoLog << "Could not create geometry shader.";
            return LinkResult(false, gl::Error(GL_NO_ERROR));
        }
        stream->skip(geometryShaderSize);
    }

    initializeUniformStorage();
    initAttributesByLayout();

    return LinkResult(true, gl::Error(GL_NO_ERROR));
}

gl::Error ProgramD3D::save(gl::BinaryOutputStream *stream)
{
    // Output the DeviceIdentifier before we output any shader code
    // When we load the binary again later, we can validate the device identifier before trying to compile any HLSL
    DeviceIdentifier binaryIdentifier = mRenderer->getAdapterIdentifier();
    stream->writeBytes(reinterpret_cast<unsigned char*>(&binaryIdentifier), sizeof(DeviceIdentifier));

    stream->writeInt(ANGLE_COMPILE_OPTIMIZATION_LEVEL);

    stream->writeInt(mShaderVersion);

    stream->writeInt(mSamplersPS.size());
    for (unsigned int i = 0; i < mSamplersPS.size(); ++i)
    {
        stream->writeInt(mSamplersPS[i].active);
        stream->writeInt(mSamplersPS[i].logicalTextureUnit);
        stream->writeInt(mSamplersPS[i].textureType);
    }

    stream->writeInt(mSamplersVS.size());
    for (unsigned int i = 0; i < mSamplersVS.size(); ++i)
    {
        stream->writeInt(mSamplersVS[i].active);
        stream->writeInt(mSamplersVS[i].logicalTextureUnit);
        stream->writeInt(mSamplersVS[i].textureType);
    }

    stream->writeInt(mUsedVertexSamplerRange);
    stream->writeInt(mUsedPixelSamplerRange);

    stream->writeInt(mUniforms.size());
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); ++uniformIndex)
    {
        const gl::LinkedUniform &uniform = *mUniforms[uniformIndex];

        stream->writeInt(uniform.type);
        stream->writeInt(uniform.precision);
        stream->writeString(uniform.name);
        stream->writeInt(uniform.arraySize);
        stream->writeInt(uniform.blockIndex);

        stream->writeInt(uniform.blockInfo.offset);
        stream->writeInt(uniform.blockInfo.arrayStride);
        stream->writeInt(uniform.blockInfo.matrixStride);
        stream->writeInt(uniform.blockInfo.isRowMajorMatrix);

        stream->writeInt(uniform.psRegisterIndex);
        stream->writeInt(uniform.vsRegisterIndex);
        stream->writeInt(uniform.registerCount);
        stream->writeInt(uniform.registerElement);
    }

    stream->writeInt(mUniformIndex.size());
    for (const auto &uniform : mUniformIndex)
    {
        GLuint location = uniform.first;
        stream->writeInt(location);

        const gl::VariableLocation &variable = uniform.second;
        stream->writeString(variable.name);
        stream->writeInt(variable.element);
        stream->writeInt(variable.index);
    }

    stream->writeInt(mUniformBlocks.size());
    for (size_t uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); ++uniformBlockIndex)
    {
        const gl::UniformBlock& uniformBlock = *mUniformBlocks[uniformBlockIndex];

        stream->writeString(uniformBlock.name);
        stream->writeInt(uniformBlock.elementIndex);
        stream->writeInt(uniformBlock.dataSize);

        stream->writeInt(uniformBlock.memberUniformIndexes.size());
        for (unsigned int blockMemberIndex = 0; blockMemberIndex < uniformBlock.memberUniformIndexes.size(); blockMemberIndex++)
        {
            stream->writeInt(uniformBlock.memberUniformIndexes[blockMemberIndex]);
        }

        stream->writeInt(uniformBlock.psRegisterIndex);
        stream->writeInt(uniformBlock.vsRegisterIndex);
    }

    stream->writeInt(mTransformFeedbackLinkedVaryings.size());
    for (size_t i = 0; i < mTransformFeedbackLinkedVaryings.size(); i++)
    {
        const gl::LinkedVarying &varying = mTransformFeedbackLinkedVaryings[i];

        stream->writeString(varying.name);
        stream->writeInt(varying.type);
        stream->writeInt(varying.size);
        stream->writeString(varying.semanticName);
        stream->writeInt(varying.semanticIndex);
        stream->writeInt(varying.semanticIndexCount);
    }

    stream->writeString(mVertexHLSL);
    stream->writeBytes(reinterpret_cast<unsigned char*>(&mVertexWorkarounds), sizeof(D3DCompilerWorkarounds));
    stream->writeString(mPixelHLSL);
    stream->writeBytes(reinterpret_cast<unsigned char*>(&mPixelWorkarounds), sizeof(D3DCompilerWorkarounds));
    stream->writeInt(mUsesFragDepth);
    stream->writeInt(mUsesPointSize);

    const std::vector<PixelShaderOutputVariable> &pixelShaderKey = mPixelShaderKey;
    stream->writeInt(pixelShaderKey.size());
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < pixelShaderKey.size(); pixelShaderKeyIndex++)
    {
        const PixelShaderOutputVariable &variable = pixelShaderKey[pixelShaderKeyIndex];
        stream->writeInt(variable.type);
        stream->writeString(variable.name);
        stream->writeString(variable.source);
        stream->writeInt(variable.outputIndex);
    }

    stream->writeInt(mVertexExecutables.size());
    for (size_t vertexExecutableIndex = 0; vertexExecutableIndex < mVertexExecutables.size(); vertexExecutableIndex++)
    {
        VertexExecutable *vertexExecutable = mVertexExecutables[vertexExecutableIndex];

        const auto &inputLayout = vertexExecutable->inputs();
        stream->writeInt(inputLayout.size());

        for (size_t inputIndex = 0; inputIndex < inputLayout.size(); inputIndex++)
        {
            stream->writeInt(inputLayout[inputIndex]);
        }

        size_t vertexShaderSize = vertexExecutable->shaderExecutable()->getLength();
        stream->writeInt(vertexShaderSize);

        const uint8_t *vertexBlob = vertexExecutable->shaderExecutable()->getFunction();
        stream->writeBytes(vertexBlob, vertexShaderSize);
    }

    stream->writeInt(mPixelExecutables.size());
    for (size_t pixelExecutableIndex = 0; pixelExecutableIndex < mPixelExecutables.size(); pixelExecutableIndex++)
    {
        PixelExecutable *pixelExecutable = mPixelExecutables[pixelExecutableIndex];

        const std::vector<GLenum> outputs = pixelExecutable->outputSignature();
        stream->writeInt(outputs.size());
        for (size_t outputIndex = 0; outputIndex < outputs.size(); outputIndex++)
        {
            stream->writeInt(outputs[outputIndex]);
        }

        size_t pixelShaderSize = pixelExecutable->shaderExecutable()->getLength();
        stream->writeInt(pixelShaderSize);

        const uint8_t *pixelBlob = pixelExecutable->shaderExecutable()->getFunction();
        stream->writeBytes(pixelBlob, pixelShaderSize);
    }

    size_t geometryShaderSize = (mGeometryExecutable != NULL) ? mGeometryExecutable->getLength() : 0;
    stream->writeInt(geometryShaderSize);

    if (mGeometryExecutable != NULL && geometryShaderSize > 0)
    {
        const uint8_t *geometryBlob = mGeometryExecutable->getFunction();
        stream->writeBytes(geometryBlob, geometryShaderSize);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error ProgramD3D::getPixelExecutableForFramebuffer(const gl::Framebuffer *fbo, ShaderExecutableD3D **outExecutable)
{
    mPixelShaderOutputFormatCache.clear();

    const FramebufferD3D *fboD3D = GetImplAs<FramebufferD3D>(fbo);
    const gl::AttachmentList &colorbuffers = fboD3D->getColorAttachmentsForRender(mRenderer->getWorkarounds());

    for (size_t colorAttachment = 0; colorAttachment < colorbuffers.size(); ++colorAttachment)
    {
        const gl::FramebufferAttachment *colorbuffer = colorbuffers[colorAttachment];

        if (colorbuffer)
        {
            mPixelShaderOutputFormatCache.push_back(colorbuffer->getBinding() == GL_BACK ? GL_COLOR_ATTACHMENT0 : colorbuffer->getBinding());
        }
        else
        {
            mPixelShaderOutputFormatCache.push_back(GL_NONE);
        }
    }

    return getPixelExecutableForOutputLayout(mPixelShaderOutputFormatCache, outExecutable, nullptr);
}

gl::Error ProgramD3D::getPixelExecutableForOutputLayout(const std::vector<GLenum> &outputSignature,
                                                        ShaderExecutableD3D **outExectuable,
                                                        gl::InfoLog *infoLog)
{
    for (size_t executableIndex = 0; executableIndex < mPixelExecutables.size(); executableIndex++)
    {
        if (mPixelExecutables[executableIndex]->matchesSignature(outputSignature))
        {
            *outExectuable = mPixelExecutables[executableIndex]->shaderExecutable();
            return gl::Error(GL_NO_ERROR);
        }
    }

    std::string finalPixelHLSL = mDynamicHLSL->generatePixelShaderForOutputSignature(mPixelHLSL, mPixelShaderKey, mUsesFragDepth,
                                                                                     outputSignature);

    // Generate new pixel executable
    ShaderExecutableD3D *pixelExecutable = NULL;

    gl::InfoLog tempInfoLog;
    gl::InfoLog *currentInfoLog = infoLog ? infoLog : &tempInfoLog;

    gl::Error error = mRenderer->compileToExecutable(
        *currentInfoLog, finalPixelHLSL, SHADER_PIXEL, mTransformFeedbackLinkedVaryings,
        (mData.getTransformFeedbackBufferMode() == GL_SEPARATE_ATTRIBS), mPixelWorkarounds,
        &pixelExecutable);
    if (error.isError())
    {
        return error;
    }

    if (pixelExecutable)
    {
        mPixelExecutables.push_back(new PixelExecutable(outputSignature, pixelExecutable));
    }
    else if (!infoLog)
    {
        std::vector<char> tempCharBuffer(tempInfoLog.getLength() + 3);
        tempInfoLog.getLog(static_cast<GLsizei>(tempInfoLog.getLength()), NULL, &tempCharBuffer[0]);
        ERR("Error compiling dynamic pixel executable:\n%s\n", &tempCharBuffer[0]);
    }

    *outExectuable = pixelExecutable;
    return gl::Error(GL_NO_ERROR);
}

gl::Error ProgramD3D::getVertexExecutableForInputLayout(const gl::InputLayout &inputLayout,
                                                        ShaderExecutableD3D **outExectuable,
                                                        gl::InfoLog *infoLog)
{
    VertexExecutable::getSignature(mRenderer, inputLayout, &mCachedVertexSignature);

    for (size_t executableIndex = 0; executableIndex < mVertexExecutables.size(); executableIndex++)
    {
        if (mVertexExecutables[executableIndex]->matchesSignature(mCachedVertexSignature))
        {
            *outExectuable = mVertexExecutables[executableIndex]->shaderExecutable();
            return gl::Error(GL_NO_ERROR);
        }
    }

    // Generate new dynamic layout with attribute conversions
    std::string finalVertexHLSL = mDynamicHLSL->generateVertexShaderForInputLayout(
        mVertexHLSL, inputLayout, mData.getAttributes());

    // Generate new vertex executable
    ShaderExecutableD3D *vertexExecutable = NULL;

    gl::InfoLog tempInfoLog;
    gl::InfoLog *currentInfoLog = infoLog ? infoLog : &tempInfoLog;

    gl::Error error = mRenderer->compileToExecutable(
        *currentInfoLog, finalVertexHLSL, SHADER_VERTEX, mTransformFeedbackLinkedVaryings,
        (mData.getTransformFeedbackBufferMode() == GL_SEPARATE_ATTRIBS), mVertexWorkarounds,
        &vertexExecutable);
    if (error.isError())
    {
        return error;
    }

    if (vertexExecutable)
    {
        mVertexExecutables.push_back(new VertexExecutable(inputLayout, mCachedVertexSignature, vertexExecutable));
    }
    else if (!infoLog)
    {
        std::vector<char> tempCharBuffer(tempInfoLog.getLength() + 3);
        tempInfoLog.getLog(static_cast<GLsizei>(tempInfoLog.getLength()), NULL, &tempCharBuffer[0]);
        ERR("Error compiling dynamic vertex executable:\n%s\n", &tempCharBuffer[0]);
    }

    *outExectuable = vertexExecutable;
    return gl::Error(GL_NO_ERROR);
}

LinkResult ProgramD3D::compileProgramExecutables(gl::InfoLog &infoLog, int registers)
{
    const ShaderD3D *vertexShaderD3D   = GetImplAs<ShaderD3D>(mData.getAttachedVertexShader());
    const ShaderD3D *fragmentShaderD3D = GetImplAs<ShaderD3D>(mData.getAttachedFragmentShader());

    const gl::InputLayout &defaultInputLayout =
        GetDefaultInputLayoutFromShader(mData.getAttachedVertexShader());
    ShaderExecutableD3D *defaultVertexExecutable = NULL;
    gl::Error error = getVertexExecutableForInputLayout(defaultInputLayout, &defaultVertexExecutable, &infoLog);
    if (error.isError())
    {
        return LinkResult(false, error);
    }

    std::vector<GLenum> defaultPixelOutput = GetDefaultOutputLayoutFromShader(getPixelShaderKey());
    ShaderExecutableD3D *defaultPixelExecutable = NULL;
    error = getPixelExecutableForOutputLayout(defaultPixelOutput, &defaultPixelExecutable, &infoLog);
    if (error.isError())
    {
        return LinkResult(false, error);
    }

    if (usesGeometryShader())
    {
        std::string geometryHLSL = mDynamicHLSL->generateGeometryShaderHLSL(registers, fragmentShaderD3D, vertexShaderD3D);

        error = mRenderer->compileToExecutable(
            infoLog, geometryHLSL, SHADER_GEOMETRY, mTransformFeedbackLinkedVaryings,
            (mData.getTransformFeedbackBufferMode() == GL_SEPARATE_ATTRIBS),
            D3DCompilerWorkarounds(), &mGeometryExecutable);
        if (error.isError())
        {
            return LinkResult(false, error);
        }
    }

#if ANGLE_SHADER_DEBUG_INFO == ANGLE_ENABLED
    if (usesGeometryShader() && mGeometryExecutable)
    {
        // Geometry shaders are currently only used internally, so there is no corresponding shader object at the interface level
        // For now the geometry shader debug info is pre-pended to the vertex shader, this is a bit of a clutch
        vertexShaderD3D->appendDebugInfo("// GEOMETRY SHADER BEGIN\n\n");
        vertexShaderD3D->appendDebugInfo(mGeometryExecutable->getDebugInfo());
        vertexShaderD3D->appendDebugInfo("\nGEOMETRY SHADER END\n\n\n");
    }

    if (defaultVertexExecutable)
    {
        vertexShaderD3D->appendDebugInfo(defaultVertexExecutable->getDebugInfo());
    }

    if (defaultPixelExecutable)
    {
        fragmentShaderD3D->appendDebugInfo(defaultPixelExecutable->getDebugInfo());
    }
#endif

    bool linkSuccess = (defaultVertexExecutable && defaultPixelExecutable && (!usesGeometryShader() || mGeometryExecutable));
    return LinkResult(linkSuccess, gl::Error(GL_NO_ERROR));
}

LinkResult ProgramD3D::link(const gl::Data &data,
                            gl::InfoLog &infoLog,
                            gl::Shader *fragmentShader,
                            gl::Shader *vertexShader,
                            std::map<int, gl::VariableLocation> *outputVariables)
{
    ShaderD3D *vertexShaderD3D = GetImplAs<ShaderD3D>(vertexShader);
    ShaderD3D *fragmentShaderD3D = GetImplAs<ShaderD3D>(fragmentShader);

    mSamplersPS.resize(data.caps->maxTextureImageUnits);
    mSamplersVS.resize(data.caps->maxVertexTextureImageUnits);

    mPixelHLSL = fragmentShaderD3D->getTranslatedSource();
    fragmentShaderD3D->generateWorkarounds(&mPixelWorkarounds);

    mVertexHLSL = vertexShaderD3D->getTranslatedSource();
    vertexShaderD3D->generateWorkarounds(&mVertexWorkarounds);
    mShaderVersion = vertexShaderD3D->getShaderVersion();

    if (mRenderer->getRendererLimitations().noFrontFacingSupport)
    {
        if (fragmentShaderD3D->usesFrontFacing())
        {
            infoLog << "The current renderer doesn't support gl_FrontFacing";
            return LinkResult(false, gl::Error(GL_NO_ERROR));
        }
    }

    // Map the varyings to the register file
    VaryingPacking packing = {};
    int registers = mDynamicHLSL->packVaryings(infoLog, packing, fragmentShaderD3D, vertexShaderD3D,
                                               mData.getTransformFeedbackVaryingNames());

    if (registers < 0)
    {
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    LinkVaryingRegisters(infoLog, vertexShaderD3D, fragmentShaderD3D);

    std::vector<gl::LinkedVarying> linkedVaryings;
    if (!mDynamicHLSL->generateShaderLinkHLSL(
            data, infoLog, registers, packing, mPixelHLSL, mVertexHLSL, fragmentShaderD3D,
            vertexShaderD3D, mData.getTransformFeedbackVaryingNames(), &linkedVaryings,
            outputVariables, &mPixelShaderKey, &mUsesFragDepth))
    {
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    mUsesPointSize = vertexShaderD3D->usesPointSize();

    initAttributesByLayout();

    if (!defineUniforms(infoLog, *data.caps))
    {
        return LinkResult(false, gl::Error(GL_NO_ERROR));
    }

    defineUniformBlocks(*data.caps);

    gatherTransformFeedbackVaryings(linkedVaryings);

    LinkResult result = compileProgramExecutables(infoLog, registers);
    if (result.error.isError() || !result.linkSuccess)
    {
        infoLog << "Failed to create D3D shaders.";
        return result;
    }

    return LinkResult(true, gl::Error(GL_NO_ERROR));
}

GLboolean ProgramD3D::validate(const gl::Caps &caps, gl::InfoLog *infoLog)
{
    applyUniforms();
    return validateSamplers(infoLog, caps);
}

void ProgramD3D::bindAttributeLocation(GLuint index, const std::string &name)
{
}

void ProgramD3D::initializeUniformStorage()
{
    // Compute total default block size
    unsigned int vertexRegisters = 0;
    unsigned int fragmentRegisters = 0;
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const gl::LinkedUniform &uniform = *mUniforms[uniformIndex];

        if (!gl::IsSamplerType(uniform.type))
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

gl::Error ProgramD3D::applyUniforms()
{
    updateSamplerMapping();

    gl::Error error = mRenderer->applyUniforms(*this, mUniforms);
    if (error.isError())
    {
        return error;
    }

    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        mUniforms[uniformIndex]->dirty = false;
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error ProgramD3D::applyUniformBuffers(const gl::Data &data)
{
    mVertexUBOCache.clear();
    mFragmentUBOCache.clear();

    const unsigned int reservedBuffersInVS = mRenderer->getReservedVertexUniformBuffers();
    const unsigned int reservedBuffersInFS = mRenderer->getReservedFragmentUniformBuffers();

    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < mUniformBlocks.size(); uniformBlockIndex++)
    {
        gl::UniformBlock *uniformBlock = mUniformBlocks[uniformBlockIndex];
        GLuint blockBinding            = mData.getUniformBlockBinding(uniformBlockIndex);

        ASSERT(uniformBlock);

        // Unnecessary to apply an unreferenced standard or shared UBO
        if (!uniformBlock->isReferencedByVertexShader() && !uniformBlock->isReferencedByFragmentShader())
        {
            continue;
        }

        if (uniformBlock->isReferencedByVertexShader())
        {
            unsigned int registerIndex = uniformBlock->vsRegisterIndex - reservedBuffersInVS;
            ASSERT(registerIndex < data.caps->maxVertexUniformBlocks);

            if (mVertexUBOCache.size() <= registerIndex)
            {
                mVertexUBOCache.resize(registerIndex + 1, -1);
            }

            ASSERT(mVertexUBOCache[registerIndex] == -1);
            mVertexUBOCache[registerIndex] = blockBinding;
        }

        if (uniformBlock->isReferencedByFragmentShader())
        {
            unsigned int registerIndex = uniformBlock->psRegisterIndex - reservedBuffersInFS;
            ASSERT(registerIndex < data.caps->maxFragmentUniformBlocks);

            if (mFragmentUBOCache.size() <= registerIndex)
            {
                mFragmentUBOCache.resize(registerIndex + 1, -1);
            }

            ASSERT(mFragmentUBOCache[registerIndex] == -1);
            mFragmentUBOCache[registerIndex] = blockBinding;
        }
    }

    return mRenderer->setUniformBuffers(data, mVertexUBOCache, mFragmentUBOCache);
}

void ProgramD3D::assignUniformBlockRegister(gl::UniformBlock *uniformBlock,
                                            GLenum shader,
                                            unsigned int registerIndex,
                                            const gl::Caps &caps)
{
    // Validation done in the GL-level Program.
    if (shader == GL_VERTEX_SHADER)
    {
        uniformBlock->vsRegisterIndex = registerIndex;
        ASSERT(registerIndex < caps.maxVertexUniformBlocks);
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        uniformBlock->psRegisterIndex = registerIndex;
        ASSERT(registerIndex < caps.maxFragmentUniformBlocks);
    }
    else UNREACHABLE();
}

void ProgramD3D::dirtyAllUniforms()
{
    unsigned int numUniforms = static_cast<unsigned int>(mUniforms.size());
    for (unsigned int index = 0; index < numUniforms; index++)
    {
        mUniforms[index]->dirty = true;
    }
}

void ProgramD3D::setUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    setUniform(location, count, v, GL_FLOAT);
}

void ProgramD3D::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC2);
}

void ProgramD3D::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC3);
}

void ProgramD3D::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniform(location, count, v, GL_FLOAT_VEC4);
}

void ProgramD3D::setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 2>(location, count, transpose, value, GL_FLOAT_MAT2);
}

void ProgramD3D::setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 3>(location, count, transpose, value, GL_FLOAT_MAT3);
}

void ProgramD3D::setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 4>(location, count, transpose, value, GL_FLOAT_MAT4);
}

void ProgramD3D::setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 3>(location, count, transpose, value, GL_FLOAT_MAT2x3);
}

void ProgramD3D::setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 2>(location, count, transpose, value, GL_FLOAT_MAT3x2);
}

void ProgramD3D::setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<2, 4>(location, count, transpose, value, GL_FLOAT_MAT2x4);
}

void ProgramD3D::setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 2>(location, count, transpose, value, GL_FLOAT_MAT4x2);
}

void ProgramD3D::setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<3, 4>(location, count, transpose, value, GL_FLOAT_MAT3x4);
}

void ProgramD3D::setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    setUniformMatrixfv<4, 3>(location, count, transpose, value, GL_FLOAT_MAT4x3);
}

void ProgramD3D::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT);
}

void ProgramD3D::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC2);
}

void ProgramD3D::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC3);
}

void ProgramD3D::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    setUniform(location, count, v, GL_INT_VEC4);
}

void ProgramD3D::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT);
}

void ProgramD3D::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC2);
}

void ProgramD3D::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC3);
}

void ProgramD3D::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
    setUniform(location, count, v, GL_UNSIGNED_INT_VEC4);
}

void ProgramD3D::getUniformfv(GLint location, GLfloat *params)
{
    getUniformv(location, params, GL_FLOAT);
}

void ProgramD3D::getUniformiv(GLint location, GLint *params)
{
    getUniformv(location, params, GL_INT);
}

void ProgramD3D::getUniformuiv(GLint location, GLuint *params)
{
    getUniformv(location, params, GL_UNSIGNED_INT);
}

bool ProgramD3D::defineUniforms(gl::InfoLog &infoLog, const gl::Caps &caps)
{
    const gl::Shader *vertexShader                 = mData.getAttachedVertexShader();
    const std::vector<sh::Uniform> &vertexUniforms = vertexShader->getUniforms();
    const ShaderD3D *vertexShaderD3D               = GetImplAs<ShaderD3D>(vertexShader);

    for (const sh::Uniform &uniform : vertexUniforms)
    {
        if (uniform.staticUse)
        {
            unsigned int registerBase = uniform.isBuiltIn() ? GL_INVALID_INDEX :
                vertexShaderD3D->getUniformRegister(uniform.name);
            defineUniformBase(vertexShaderD3D, uniform, registerBase);
        }
    }

    const gl::Shader *fragmentShader                 = mData.getAttachedFragmentShader();
    const std::vector<sh::Uniform> &fragmentUniforms = fragmentShader->getUniforms();
    const ShaderD3D *fragmentShaderD3D               = GetImplAs<ShaderD3D>(fragmentShader);

    for (const sh::Uniform &uniform : fragmentUniforms)
    {
        if (uniform.staticUse)
        {
            unsigned int registerBase = uniform.isBuiltIn() ? GL_INVALID_INDEX :
                fragmentShaderD3D->getUniformRegister(uniform.name);
            defineUniformBase(fragmentShaderD3D, uniform, registerBase);
        }
    }

    // TODO(jmadill): move the validation part to gl::Program
    if (!indexUniforms(infoLog, caps))
    {
        return false;
    }

    initializeUniformStorage();

    return true;
}

void ProgramD3D::defineUniformBase(const ShaderD3D *shader, const sh::Uniform &uniform, unsigned int uniformRegister)
{
    if (uniformRegister == GL_INVALID_INDEX)
    {
        defineUniform(shader, uniform, uniform.name, nullptr);
        return;
    }

    ShShaderOutput outputType = shader->getCompilerOutputType();
    sh::HLSLBlockEncoder encoder(sh::HLSLBlockEncoder::GetStrategyFor(outputType));
    encoder.skipRegisters(uniformRegister);

    defineUniform(shader, uniform, uniform.name, &encoder);
}

void ProgramD3D::defineUniform(const ShaderD3D *shader, const sh::ShaderVariable &uniform,
                               const std::string &fullName, sh::HLSLBlockEncoder *encoder)
{
    if (uniform.isStruct())
    {
        for (unsigned int elementIndex = 0; elementIndex < uniform.elementCount(); elementIndex++)
        {
            const std::string &elementString = (uniform.isArray() ? ArrayString(elementIndex) : "");

            if (encoder)
                encoder->enterAggregateType();

            for (size_t fieldIndex = 0; fieldIndex < uniform.fields.size(); fieldIndex++)
            {
                const sh::ShaderVariable &field = uniform.fields[fieldIndex];
                const std::string &fieldFullName = (fullName + elementString + "." + field.name);

                defineUniform(shader, field, fieldFullName, encoder);
            }

            if (encoder)
                encoder->exitAggregateType();
        }
    }
    else // Not a struct
    {
        // Arrays are treated as aggregate types
        if (uniform.isArray() && encoder)
        {
            encoder->enterAggregateType();
        }

        gl::LinkedUniform *linkedUniform = getUniformByName(fullName);

        // Advance the uniform offset, to track registers allocation for structs
        sh::BlockMemberInfo blockInfo = encoder ?
            encoder->encodeType(uniform.type, uniform.arraySize, false) :
            sh::BlockMemberInfo::getDefaultBlockInfo();

        if (!linkedUniform)
        {
            linkedUniform = new gl::LinkedUniform(uniform.type, uniform.precision, fullName, uniform.arraySize,
                                                  -1, sh::BlockMemberInfo::getDefaultBlockInfo());
            ASSERT(linkedUniform);

            if (encoder)
                linkedUniform->registerElement = static_cast<unsigned int>(
                    sh::HLSLBlockEncoder::getBlockRegisterElement(blockInfo));
            mUniforms.push_back(linkedUniform);
        }

        if (encoder)
        {
            if (shader->getShaderType() == GL_FRAGMENT_SHADER)
            {
                linkedUniform->psRegisterIndex =
                    static_cast<unsigned int>(sh::HLSLBlockEncoder::getBlockRegister(blockInfo));
            }
            else if (shader->getShaderType() == GL_VERTEX_SHADER)
            {
                linkedUniform->vsRegisterIndex =
                    static_cast<unsigned int>(sh::HLSLBlockEncoder::getBlockRegister(blockInfo));
            }
            else UNREACHABLE();
        }

        // Arrays are treated as aggregate types
        if (uniform.isArray() && encoder)
        {
            encoder->exitAggregateType();
        }
    }
}

void ProgramD3D::defineUniformBlocks(const gl::Caps &caps)
{
    const gl::Shader *vertexShader = mData.getAttachedVertexShader();

    for (const sh::InterfaceBlock &vertexBlock : vertexShader->getInterfaceBlocks())
    {
        if (vertexBlock.staticUse || vertexBlock.layout != sh::BLOCKLAYOUT_PACKED)
        {
            defineUniformBlock(*vertexShader, vertexBlock, caps);
        }
    }

    const gl::Shader *fragmentShader = mData.getAttachedFragmentShader();

    for (const sh::InterfaceBlock &fragmentBlock : fragmentShader->getInterfaceBlocks())
    {
        if (fragmentBlock.staticUse || fragmentBlock.layout != sh::BLOCKLAYOUT_PACKED)
        {
            defineUniformBlock(*fragmentShader, fragmentBlock, caps);
        }
    }
}

template <typename T>
static inline void SetIfDirty(T *dest, const T& source, bool *dirtyFlag)
{
    ASSERT(dest != NULL);
    ASSERT(dirtyFlag != NULL);

    *dirtyFlag = *dirtyFlag || (memcmp(dest, &source, sizeof(T)) != 0);
    *dest = source;
}

template <typename T>
void ProgramD3D::setUniform(GLint location, GLsizei count, const T* v, GLenum targetUniformType)
{
    const int components = gl::VariableComponentCount(targetUniformType);
    const GLenum targetBoolType = gl::VariableBoolVectorType(targetUniformType);

    gl::LinkedUniform *targetUniform = getUniformByLocation(location);

    int elementCount = targetUniform->elementCount();

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);

    if (targetUniform->type == targetUniformType)
    {
        T *target = reinterpret_cast<T*>(targetUniform->data) + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            T *dest = target + (i * 4);
            const T *source = v + (i * components);

            for (int c = 0; c < components; c++)
            {
                SetIfDirty(dest + c, source[c], &targetUniform->dirty);
            }
            for (int c = components; c < 4; c++)
            {
                SetIfDirty(dest + c, T(0), &targetUniform->dirty);
            }
        }
    }
    else if (targetUniform->type == targetBoolType)
    {
        GLint *boolParams = reinterpret_cast<GLint*>(targetUniform->data) + mUniformIndex[location].element * 4;

        for (int i = 0; i < count; i++)
        {
            GLint *dest = boolParams + (i * 4);
            const T *source = v + (i * components);

            for (int c = 0; c < components; c++)
            {
                SetIfDirty(dest + c, (source[c] == static_cast<T>(0)) ? GL_FALSE : GL_TRUE, &targetUniform->dirty);
            }
            for (int c = components; c < 4; c++)
            {
                SetIfDirty(dest + c, GL_FALSE, &targetUniform->dirty);
            }
        }
    }
    else if (gl::IsSamplerType(targetUniform->type))
    {
        ASSERT(targetUniformType == GL_INT);

        GLint *target = reinterpret_cast<GLint*>(targetUniform->data) + mUniformIndex[location].element * 4;

        bool wasDirty = targetUniform->dirty;

        for (int i = 0; i < count; i++)
        {
            GLint *dest = target + (i * 4);
            const GLint *source = reinterpret_cast<const GLint*>(v) + (i * components);

            SetIfDirty(dest + 0, source[0], &targetUniform->dirty);
            SetIfDirty(dest + 1, 0, &targetUniform->dirty);
            SetIfDirty(dest + 2, 0, &targetUniform->dirty);
            SetIfDirty(dest + 3, 0, &targetUniform->dirty);
        }

        if (!wasDirty && targetUniform->dirty)
        {
            mDirtySamplerMapping = true;
        }
    }
    else UNREACHABLE();
}

template<typename T>
bool transposeMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    bool dirty = false;
    int copyWidth = std::min(targetHeight, srcWidth);
    int copyHeight = std::min(targetWidth, srcHeight);

    for (int x = 0; x < copyWidth; x++)
    {
        for (int y = 0; y < copyHeight; y++)
        {
            SetIfDirty(target + (x * targetWidth + y), static_cast<T>(value[y * srcWidth + x]), &dirty);
        }
    }
    // clear unfilled right side
    for (int y = 0; y < copyWidth; y++)
    {
        for (int x = copyHeight; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }
    // clear unfilled bottom.
    for (int y = copyWidth; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }

    return dirty;
}

template<typename T>
bool expandMatrix(T *target, const GLfloat *value, int targetWidth, int targetHeight, int srcWidth, int srcHeight)
{
    bool dirty = false;
    int copyWidth = std::min(targetWidth, srcWidth);
    int copyHeight = std::min(targetHeight, srcHeight);

    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = 0; x < copyWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(value[y * srcWidth + x]), &dirty);
        }
    }
    // clear unfilled right side
    for (int y = 0; y < copyHeight; y++)
    {
        for (int x = copyWidth; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }
    // clear unfilled bottom.
    for (int y = copyHeight; y < targetHeight; y++)
    {
        for (int x = 0; x < targetWidth; x++)
        {
            SetIfDirty(target + (y * targetWidth + x), static_cast<T>(0), &dirty);
        }
    }

    return dirty;
}

template <int cols, int rows>
void ProgramD3D::setUniformMatrixfv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value, GLenum targetUniformType)
{
    gl::LinkedUniform *targetUniform = getUniformByLocation(location);

    int elementCount = targetUniform->elementCount();

    count = std::min(elementCount - (int)mUniformIndex[location].element, count);
    const unsigned int targetMatrixStride = (4 * rows);
    GLfloat *target = (GLfloat*)(targetUniform->data + mUniformIndex[location].element * sizeof(GLfloat) * targetMatrixStride);

    for (int i = 0; i < count; i++)
    {
        // Internally store matrices as transposed versions to accomodate HLSL matrix indexing
        if (transpose == GL_FALSE)
        {
            targetUniform->dirty = transposeMatrix<GLfloat>(target, value, 4, rows, rows, cols) || targetUniform->dirty;
        }
        else
        {
            targetUniform->dirty = expandMatrix<GLfloat>(target, value, 4, rows, cols, rows) || targetUniform->dirty;
        }
        target += targetMatrixStride;
        value += cols * rows;
    }
}

template <typename T>
void ProgramD3D::getUniformv(GLint location, T *params, GLenum uniformType)
{
    gl::LinkedUniform *targetUniform = mUniforms[mUniformIndex[location].index];

    if (gl::IsMatrixType(targetUniform->type))
    {
        const int rows = gl::VariableRowCount(targetUniform->type);
        const int cols = gl::VariableColumnCount(targetUniform->type);
        transposeMatrix(params, (GLfloat*)targetUniform->data + mUniformIndex[location].element * 4 * rows, rows, cols, 4, rows);
    }
    else if (uniformType == gl::VariableComponentType(targetUniform->type))
    {
        unsigned int size = gl::VariableComponentCount(targetUniform->type);
        memcpy(params, targetUniform->data + mUniformIndex[location].element * 4 * sizeof(T),
                size * sizeof(T));
    }
    else
    {
        unsigned int size = gl::VariableComponentCount(targetUniform->type);
        switch (gl::VariableComponentType(targetUniform->type))
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
}

template <typename VarT>
void ProgramD3D::defineUniformBlockMembers(const std::vector<VarT> &fields, const std::string &prefix, int blockIndex,
                                           sh::BlockLayoutEncoder *encoder, std::vector<unsigned int> *blockUniformIndexes,
                                           bool inRowMajorLayout)
{
    for (unsigned int uniformIndex = 0; uniformIndex < fields.size(); uniformIndex++)
    {
        const VarT &field = fields[uniformIndex];
        const std::string &fieldName = (prefix.empty() ? field.name : prefix + "." + field.name);

        if (field.isStruct())
        {
            bool rowMajorLayout = (inRowMajorLayout || IsRowMajorLayout(field));

            for (unsigned int arrayElement = 0; arrayElement < field.elementCount(); arrayElement++)
            {
                encoder->enterAggregateType();

                const std::string uniformElementName = fieldName + (field.isArray() ? ArrayString(arrayElement) : "");
                defineUniformBlockMembers(field.fields, uniformElementName, blockIndex, encoder, blockUniformIndexes, rowMajorLayout);

                encoder->exitAggregateType();
            }
        }
        else
        {
            bool isRowMajorMatrix = (gl::IsMatrixType(field.type) && inRowMajorLayout);

            sh::BlockMemberInfo memberInfo = encoder->encodeType(field.type, field.arraySize, isRowMajorMatrix);

            gl::LinkedUniform *newUniform = new gl::LinkedUniform(field.type, field.precision, fieldName, field.arraySize,
                                                          blockIndex, memberInfo);

            // add to uniform list, but not index, since uniform block uniforms have no location
            blockUniformIndexes->push_back(static_cast<GLenum>(mUniforms.size()));
            mUniforms.push_back(newUniform);
        }
    }
}

void ProgramD3D::defineUniformBlock(const gl::Shader &shader,
                                    const sh::InterfaceBlock &interfaceBlock,
                                    const gl::Caps &caps)
{
    const ShaderD3D* shaderD3D = GetImplAs<ShaderD3D>(&shader);

    // create uniform block entries if they do not exist
    if (getUniformBlockIndex(interfaceBlock.name) == GL_INVALID_INDEX)
    {
        std::vector<unsigned int> blockUniformIndexes;
        const unsigned int blockIndex = static_cast<unsigned int>(mUniformBlocks.size());

        // define member uniforms
        sh::BlockLayoutEncoder *encoder = NULL;

        if (interfaceBlock.layout == sh::BLOCKLAYOUT_STANDARD)
        {
            encoder = new sh::Std140BlockEncoder;
        }
        else
        {
            encoder = new sh::HLSLBlockEncoder(sh::HLSLBlockEncoder::ENCODE_PACKED);
        }
        ASSERT(encoder);

        defineUniformBlockMembers(interfaceBlock.fields, "", blockIndex, encoder, &blockUniformIndexes, interfaceBlock.isRowMajorLayout);

        unsigned int dataSize = static_cast<unsigned int>(encoder->getBlockSize());

        // create all the uniform blocks
        if (interfaceBlock.arraySize > 0)
        {
            for (unsigned int uniformBlockElement = 0; uniformBlockElement < interfaceBlock.arraySize; uniformBlockElement++)
            {
                gl::UniformBlock *newUniformBlock = new gl::UniformBlock(interfaceBlock.name, uniformBlockElement, dataSize);
                newUniformBlock->memberUniformIndexes = blockUniformIndexes;
                mUniformBlocks.push_back(newUniformBlock);
            }
        }
        else
        {
            gl::UniformBlock *newUniformBlock = new gl::UniformBlock(interfaceBlock.name, GL_INVALID_INDEX, dataSize);
            newUniformBlock->memberUniformIndexes = blockUniformIndexes;
            mUniformBlocks.push_back(newUniformBlock);
        }
    }

    if (interfaceBlock.staticUse)
    {
        // Assign registers to the uniform blocks
        const GLuint blockIndex = getUniformBlockIndex(interfaceBlock.name);
        const unsigned int elementCount = std::max(1u, interfaceBlock.arraySize);
        ASSERT(blockIndex != GL_INVALID_INDEX);
        ASSERT(blockIndex + elementCount <= mUniformBlocks.size());

        unsigned int interfaceBlockRegister = shaderD3D->getInterfaceBlockRegister(interfaceBlock.name);

        for (unsigned int uniformBlockElement = 0; uniformBlockElement < elementCount; uniformBlockElement++)
        {
            gl::UniformBlock *uniformBlock = mUniformBlocks[blockIndex + uniformBlockElement];
            ASSERT(uniformBlock->name == interfaceBlock.name);

            assignUniformBlockRegister(uniformBlock, shader.getType(),
                                       interfaceBlockRegister + uniformBlockElement, caps);
        }
    }
}

bool ProgramD3D::assignSamplers(unsigned int startSamplerIndex,
                                GLenum samplerType,
                                unsigned int samplerCount,
                                std::vector<Sampler> &outSamplers,
                                GLuint *outUsedRange)
{
    unsigned int samplerIndex = startSamplerIndex;

    do
    {
        if (samplerIndex < outSamplers.size())
        {
            Sampler& sampler = outSamplers[samplerIndex];
            sampler.active = true;
            sampler.textureType = GetTextureType(samplerType);
            sampler.logicalTextureUnit = 0;
            *outUsedRange = std::max(samplerIndex + 1, *outUsedRange);
        }
        else
        {
            return false;
        }

        samplerIndex++;
    } while (samplerIndex < startSamplerIndex + samplerCount);

    return true;
}

bool ProgramD3D::indexSamplerUniform(const gl::LinkedUniform &uniform, gl::InfoLog &infoLog, const gl::Caps &caps)
{
    ASSERT(gl::IsSamplerType(uniform.type));
    ASSERT(uniform.vsRegisterIndex != GL_INVALID_INDEX || uniform.psRegisterIndex != GL_INVALID_INDEX);

    if (uniform.vsRegisterIndex != GL_INVALID_INDEX)
    {
        if (!assignSamplers(uniform.vsRegisterIndex, uniform.type, uniform.arraySize, mSamplersVS,
                            &mUsedVertexSamplerRange))
        {
            infoLog << "Vertex shader sampler count exceeds the maximum vertex texture units ("
                    << mSamplersVS.size() << ").";
            return false;
        }

        unsigned int maxVertexVectors = mRenderer->getReservedVertexUniformVectors() + caps.maxVertexUniformVectors;
        if (uniform.vsRegisterIndex + uniform.registerCount > maxVertexVectors)
        {
            infoLog << "Vertex shader active uniforms exceed GL_MAX_VERTEX_UNIFORM_VECTORS ("
                    << caps.maxVertexUniformVectors << ").";
            return false;
        }
    }

    if (uniform.psRegisterIndex != GL_INVALID_INDEX)
    {
        if (!assignSamplers(uniform.psRegisterIndex, uniform.type, uniform.arraySize, mSamplersPS,
                            &mUsedPixelSamplerRange))
        {
            infoLog << "Pixel shader sampler count exceeds MAX_TEXTURE_IMAGE_UNITS ("
                    << mSamplersPS.size() << ").";
            return false;
        }

        unsigned int maxFragmentVectors = mRenderer->getReservedFragmentUniformVectors() + caps.maxFragmentUniformVectors;
        if (uniform.psRegisterIndex + uniform.registerCount > maxFragmentVectors)
        {
            infoLog << "Fragment shader active uniforms exceed GL_MAX_FRAGMENT_UNIFORM_VECTORS ("
                    << caps.maxFragmentUniformVectors << ").";
            return false;
        }
    }

    return true;
}

bool ProgramD3D::indexUniforms(gl::InfoLog &infoLog, const gl::Caps &caps)
{
    for (size_t uniformIndex = 0; uniformIndex < mUniforms.size(); uniformIndex++)
    {
        const gl::LinkedUniform &uniform = *mUniforms[uniformIndex];

        if (gl::IsSamplerType(uniform.type))
        {
            if (!indexSamplerUniform(uniform, infoLog, caps))
            {
                return false;
            }
        }

        for (unsigned int arrayIndex = 0; arrayIndex < uniform.elementCount(); arrayIndex++)
        {
            if (!uniform.isBuiltIn())
            {
                // Assign in-order uniform locations
                mUniformIndex[static_cast<GLuint>(mUniformIndex.size())] = gl::VariableLocation(
                    uniform.name, arrayIndex, static_cast<unsigned int>(uniformIndex));
            }
        }
    }

    return true;
}

void ProgramD3D::reset()
{
    ProgramImpl::reset();

    SafeDeleteContainer(mVertexExecutables);
    SafeDeleteContainer(mPixelExecutables);
    SafeDelete(mGeometryExecutable);

    mVertexHLSL.clear();
    mVertexWorkarounds = D3DCompilerWorkarounds();
    mShaderVersion = 100;

    mPixelHLSL.clear();
    mPixelWorkarounds = D3DCompilerWorkarounds();
    mUsesFragDepth = false;
    mPixelShaderKey.clear();
    mUsesPointSize = false;

    SafeDelete(mVertexUniformStorage);
    SafeDelete(mFragmentUniformStorage);

    mSamplersPS.clear();
    mSamplersVS.clear();

    mUsedVertexSamplerRange = 0;
    mUsedPixelSamplerRange = 0;
    mDirtySamplerMapping = true;

    std::fill(mAttributesByLayout, mAttributesByLayout + ArraySize(mAttributesByLayout), -1);

    mTransformFeedbackLinkedVaryings.clear();
}

unsigned int ProgramD3D::getSerial() const
{
    return mSerial;
}

unsigned int ProgramD3D::issueSerial()
{
    return mCurrentSerial++;
}

void ProgramD3D::initAttributesByLayout()
{
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mAttributesByLayout[i] = i;
    }

    std::sort(&mAttributesByLayout[0], &mAttributesByLayout[gl::MAX_VERTEX_ATTRIBS], AttributeSorter(mSemanticIndex));
}

void ProgramD3D::sortAttributesByLayout(const std::vector<TranslatedAttribute> &unsortedAttributes,
                                        int sortedSemanticIndicesOut[gl::MAX_VERTEX_ATTRIBS],
                                        const rx::TranslatedAttribute *sortedAttributesOut[gl::MAX_VERTEX_ATTRIBS]) const
{
    for (size_t attribIndex = 0; attribIndex < unsortedAttributes.size(); ++attribIndex)
    {
        int oldIndex = mAttributesByLayout[attribIndex];
        sortedSemanticIndicesOut[attribIndex] = mSemanticIndex[oldIndex];
        sortedAttributesOut[attribIndex] = &unsortedAttributes[oldIndex];
    }
}

void ProgramD3D::updateCachedInputLayout(const gl::Program *program, const gl::State &state)
{
    mCachedInputLayout.clear();
    const int *semanticIndexes = program->getSemanticIndexes();

    const auto &vertexAttributes = state.getVertexArray()->getVertexAttributes();

    for (unsigned int attributeIndex = 0; attributeIndex < vertexAttributes.size(); attributeIndex++)
    {
        int semanticIndex = semanticIndexes[attributeIndex];

        if (semanticIndex != -1)
        {
            if (mCachedInputLayout.size() < static_cast<size_t>(semanticIndex + 1))
            {
                mCachedInputLayout.resize(semanticIndex + 1, gl::VERTEX_FORMAT_INVALID);
            }
            mCachedInputLayout[semanticIndex] =
                GetVertexFormatType(vertexAttributes[attributeIndex],
                                    state.getVertexAttribCurrentValue(attributeIndex).Type);
        }
    }
}

void ProgramD3D::gatherTransformFeedbackVaryings(
    const std::vector<gl::LinkedVarying> &linkedVaryings)
{
    // Gather the linked varyings that are used for transform feedback, they should all exist.
    mTransformFeedbackLinkedVaryings.clear();
    for (const std::string &tfVaryingName : mData.getTransformFeedbackVaryingNames())
    {
        for (const gl::LinkedVarying &linkedVarying : linkedVaryings)
        {
            if (tfVaryingName == linkedVarying.name)
            {
                mTransformFeedbackLinkedVaryings.push_back(linkedVarying);
                break;
            }
        }
    }
}
}
