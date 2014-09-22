//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramD3D.cpp: Defines the rx::ProgramD3D class which implements rx::ProgramImpl.

#include "libGLESv2/renderer/d3d/ProgramD3D.h"

#include "common/utilities.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/ShaderExecutable.h"
#include "libGLESv2/renderer/d3d/DynamicHLSL.h"
#include "libGLESv2/renderer/d3d/ShaderD3D.h"
#include "libGLESv2/main.h"

namespace rx
{

ProgramD3D::ProgramD3D(rx::Renderer *renderer)
    : ProgramImpl(),
      mRenderer(renderer),
      mDynamicHLSL(NULL),
      mVertexWorkarounds(rx::ANGLE_D3D_WORKAROUND_NONE),
      mPixelWorkarounds(rx::ANGLE_D3D_WORKAROUND_NONE),
      mUsesPointSize(false),
      mVertexUniformStorage(NULL),
      mFragmentUniformStorage(NULL),
      mShaderVersion(100)
{
    mDynamicHLSL = new rx::DynamicHLSL(renderer);
}

ProgramD3D::~ProgramD3D()
{
    reset();
    SafeDelete(mDynamicHLSL);
}

ProgramD3D *ProgramD3D::makeProgramD3D(ProgramImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(ProgramD3D*, impl));
    return static_cast<ProgramD3D*>(impl);
}

const ProgramD3D *ProgramD3D::makeProgramD3D(const ProgramImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(const ProgramD3D*, impl));
    return static_cast<const ProgramD3D*>(impl);
}

bool ProgramD3D::usesPointSize() const
{
    return mUsesPointSize;
}

bool ProgramD3D::usesPointSpriteEmulation() const
{
    return mUsesPointSize && mRenderer->getMajorShaderModel() >= 4;
}

bool ProgramD3D::usesGeometryShader() const
{
    return usesPointSpriteEmulation();
}

bool ProgramD3D::load(gl::InfoLog &infoLog, gl::BinaryInputStream *stream)
{
    stream->readInt(&mShaderVersion);

    stream->readString(&mVertexHLSL);
    stream->readInt(&mVertexWorkarounds);
    stream->readString(&mPixelHLSL);
    stream->readInt(&mPixelWorkarounds);
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

    GUID binaryIdentifier = {0};
    stream->readBytes(reinterpret_cast<unsigned char*>(&binaryIdentifier), sizeof(GUID));

    GUID identifier = mRenderer->getAdapterIdentifier();
    if (memcmp(&identifier, &binaryIdentifier, sizeof(GUID)) != 0)
    {
        infoLog.append("Invalid program binary.");
        return false;
    }

    return true;
}

bool ProgramD3D::save(gl::BinaryOutputStream *stream)
{
    stream->writeInt(mShaderVersion);

    stream->writeString(mVertexHLSL);
    stream->writeInt(mVertexWorkarounds);
    stream->writeString(mPixelHLSL);
    stream->writeInt(mPixelWorkarounds);
    stream->writeInt(mUsesFragDepth);
    stream->writeInt(mUsesPointSize);

    const std::vector<rx::PixelShaderOutputVariable> &pixelShaderKey = mPixelShaderKey;
    stream->writeInt(pixelShaderKey.size());
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < pixelShaderKey.size(); pixelShaderKeyIndex++)
    {
        const rx::PixelShaderOutputVariable &variable = pixelShaderKey[pixelShaderKeyIndex];
        stream->writeInt(variable.type);
        stream->writeString(variable.name);
        stream->writeString(variable.source);
        stream->writeInt(variable.outputIndex);
    }

    GUID binaryIdentifier = mRenderer->getAdapterIdentifier();
    stream->writeBytes(reinterpret_cast<unsigned char*>(&binaryIdentifier),  sizeof(GUID));

    return true;
}

ShaderExecutable *ProgramD3D::getPixelExecutableForOutputLayout(gl::InfoLog &infoLog, const std::vector<GLenum> &outputSignature,
                                                                const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                                                bool separatedOutputBuffers)
{
    std::string finalPixelHLSL = mDynamicHLSL->generatePixelShaderForOutputSignature(mPixelHLSL, mPixelShaderKey, mUsesFragDepth,
                                                                                     outputSignature);

    // Generate new pixel executable
    ShaderExecutable *pixelExecutable = mRenderer->compileToExecutable(infoLog, finalPixelHLSL.c_str(), rx::SHADER_PIXEL,
                                                                        transformFeedbackLinkedVaryings, separatedOutputBuffers,
                                                                        mPixelWorkarounds);

    return pixelExecutable;
}

ShaderExecutable *ProgramD3D::getVertexExecutableForInputLayout(gl::InfoLog &infoLog,
                                                                const gl::VertexFormat inputLayout[gl::MAX_VERTEX_ATTRIBS],
                                                                const sh::Attribute shaderAttributes[],
                                                                const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                                                bool separatedOutputBuffers)
{
    // Generate new dynamic layout with attribute conversions
    std::string finalVertexHLSL = mDynamicHLSL->generateVertexShaderForInputLayout(mVertexHLSL, inputLayout, shaderAttributes);

    // Generate new vertex executable
    ShaderExecutable *vertexExecutable = mRenderer->compileToExecutable(infoLog, finalVertexHLSL.c_str(),
                                                                        rx::SHADER_VERTEX,
                                                                        transformFeedbackLinkedVaryings, separatedOutputBuffers,
                                                                        mVertexWorkarounds);

    return vertexExecutable;
}

ShaderExecutable *ProgramD3D::getGeometryExecutable(gl::InfoLog &infoLog, gl::Shader *fragmentShader, gl::Shader *vertexShader,
                                                    const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                                    bool separatedOutputBuffers, int registers)
{
    ShaderD3D *vertexShaderD3D = ShaderD3D::makeShaderD3D(vertexShader->getImplementation());
    ShaderD3D *fragmentShaderD3D = ShaderD3D::makeShaderD3D(fragmentShader->getImplementation());

    std::string geometryHLSL = mDynamicHLSL->generateGeometryShaderHLSL(registers, fragmentShaderD3D, vertexShaderD3D);

    ShaderExecutable *geometryExecutable = mRenderer->compileToExecutable(infoLog, geometryHLSL.c_str(),
                                                                          rx::SHADER_GEOMETRY, transformFeedbackLinkedVaryings,
                                                                          separatedOutputBuffers, rx::ANGLE_D3D_WORKAROUND_NONE);

    return geometryExecutable;
}

ShaderExecutable *ProgramD3D::loadExecutable(const void *function, size_t length, rx::ShaderType type,
                                             const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                             bool separatedOutputBuffers)
{
    return mRenderer->loadExecutable(function, length, type, transformFeedbackLinkedVaryings, separatedOutputBuffers);
}

bool ProgramD3D::link(gl::InfoLog &infoLog, gl::Shader *fragmentShader, gl::Shader *vertexShader,
                      const std::vector<std::string> &transformFeedbackVaryings, int *registers,
                      std::vector<gl::LinkedVarying> *linkedVaryings, std::map<int, gl::VariableLocation> *outputVariables)
{
    rx::ShaderD3D *vertexShaderD3D = rx::ShaderD3D::makeShaderD3D(vertexShader->getImplementation());
    rx::ShaderD3D *fragmentShaderD3D = rx::ShaderD3D::makeShaderD3D(fragmentShader->getImplementation());

    mPixelHLSL = fragmentShaderD3D->getTranslatedSource();
    mPixelWorkarounds = fragmentShaderD3D->getD3DWorkarounds();

    mVertexHLSL = vertexShaderD3D->getTranslatedSource();
    mVertexWorkarounds = vertexShaderD3D->getD3DWorkarounds();
    mShaderVersion = vertexShaderD3D->getShaderVersion();

    // Map the varyings to the register file
    rx::VaryingPacking packing = { NULL };
    *registers = mDynamicHLSL->packVaryings(infoLog, packing, fragmentShaderD3D, vertexShaderD3D, transformFeedbackVaryings);

    if (*registers < 0)
    {
        return false;
    }

    if (!gl::ProgramBinary::linkVaryings(infoLog, fragmentShader, vertexShader))
    {
        return false;
    }

    if (!mDynamicHLSL->generateShaderLinkHLSL(infoLog, *registers, packing, mPixelHLSL, mVertexHLSL,
                                              fragmentShaderD3D, vertexShaderD3D, transformFeedbackVaryings,
                                              linkedVaryings, outputVariables, &mPixelShaderKey, &mUsesFragDepth))
    {
        return false;
    }

    mUsesPointSize = vertexShaderD3D->usesPointSize();

    return true;
}

void ProgramD3D::getInputLayoutSignature(const gl::VertexFormat inputLayout[], GLenum signature[]) const
{
    mDynamicHLSL->getInputLayoutSignature(inputLayout, signature);
}

void ProgramD3D::initializeUniformStorage(const std::vector<gl::LinkedUniform*> &uniforms)
{
    // Compute total default block size
    unsigned int vertexRegisters = 0;
    unsigned int fragmentRegisters = 0;
    for (size_t uniformIndex = 0; uniformIndex < uniforms.size(); uniformIndex++)
    {
        const gl::LinkedUniform &uniform = *uniforms[uniformIndex];

        if (!gl::IsSampler(uniform.type))
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

gl::Error ProgramD3D::applyUniforms(const std::vector<gl::LinkedUniform*> &uniforms)
{
    return mRenderer->applyUniforms(*this, uniforms);
}

gl::Error ProgramD3D::applyUniformBuffers(const std::vector<gl::UniformBlock*> uniformBlocks, const std::vector<gl::Buffer*> boundBuffers,
                                     const gl::Caps &caps)
{
    const gl::Buffer *vertexUniformBuffers[gl::IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS] = {NULL};
    const gl::Buffer *fragmentUniformBuffers[gl::IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS] = {NULL};

    const unsigned int reservedBuffersInVS = mRenderer->getReservedVertexUniformBuffers();
    const unsigned int reservedBuffersInFS = mRenderer->getReservedFragmentUniformBuffers();

    for (unsigned int uniformBlockIndex = 0; uniformBlockIndex < uniformBlocks.size(); uniformBlockIndex++)
    {
        gl::UniformBlock *uniformBlock = uniformBlocks[uniformBlockIndex];
        gl::Buffer *uniformBuffer = boundBuffers[uniformBlockIndex];

        ASSERT(uniformBlock && uniformBuffer);

        if (uniformBuffer->getSize() < uniformBlock->dataSize)
        {
            // undefined behaviour
            return gl::Error(GL_INVALID_OPERATION, "It is undefined behaviour to use a uniform buffer that is too small.");
        }

        // Unnecessary to apply an unreferenced standard or shared UBO
        if (!uniformBlock->isReferencedByVertexShader() && !uniformBlock->isReferencedByFragmentShader())
        {
            continue;
        }

        if (uniformBlock->isReferencedByVertexShader())
        {
            unsigned int registerIndex = uniformBlock->vsRegisterIndex - reservedBuffersInVS;
            ASSERT(vertexUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < caps.maxVertexUniformBlocks);
            vertexUniformBuffers[registerIndex] = uniformBuffer;
        }

        if (uniformBlock->isReferencedByFragmentShader())
        {
            unsigned int registerIndex = uniformBlock->psRegisterIndex - reservedBuffersInFS;
            ASSERT(fragmentUniformBuffers[registerIndex] == NULL);
            ASSERT(registerIndex < caps.maxFragmentUniformBlocks);
            fragmentUniformBuffers[registerIndex] = uniformBuffer;
        }
    }

    return mRenderer->setUniformBuffers(vertexUniformBuffers, fragmentUniformBuffers);
}

bool ProgramD3D::assignUniformBlockRegister(gl::InfoLog &infoLog, gl::UniformBlock *uniformBlock, GLenum shader,
                                            unsigned int registerIndex, const gl::Caps &caps)
{
    if (shader == GL_VERTEX_SHADER)
    {
        uniformBlock->vsRegisterIndex = registerIndex;
        if (registerIndex - mRenderer->getReservedVertexUniformBuffers() >= caps.maxVertexUniformBlocks)
        {
            infoLog.append("Vertex shader uniform block count exceed GL_MAX_VERTEX_UNIFORM_BLOCKS (%u)", caps.maxVertexUniformBlocks);
            return false;
        }
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        uniformBlock->psRegisterIndex = registerIndex;
        if (registerIndex - mRenderer->getReservedFragmentUniformBuffers() >= caps.maxFragmentUniformBlocks)
        {
            infoLog.append("Fragment shader uniform block count exceed GL_MAX_FRAGMENT_UNIFORM_BLOCKS (%u)", caps.maxFragmentUniformBlocks);
            return false;
        }
    }
    else UNREACHABLE();

    return true;
}

unsigned int ProgramD3D::getReservedUniformVectors(GLenum shader)
{
    if (shader == GL_VERTEX_SHADER)
    {
        return mRenderer->getReservedVertexUniformVectors();
    }
    else if (shader == GL_FRAGMENT_SHADER)
    {
        return mRenderer->getReservedFragmentUniformVectors();
    }
    else UNREACHABLE();

    return 0;
}

void ProgramD3D::reset()
{
    mVertexHLSL.clear();
    mVertexWorkarounds = rx::ANGLE_D3D_WORKAROUND_NONE;
    mShaderVersion = 100;

    mPixelHLSL.clear();
    mPixelWorkarounds = rx::ANGLE_D3D_WORKAROUND_NONE;
    mUsesFragDepth = false;
    mPixelShaderKey.clear();
    mUsesPointSize = false;

    SafeDelete(mVertexUniformStorage);
    SafeDelete(mFragmentUniformStorage);
}

}
