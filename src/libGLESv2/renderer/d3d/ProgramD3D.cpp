//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramD3D.cpp: Defines the rx::ProgramD3D class which implements rx::ProgramImpl.

#include "libGLESv2/renderer/d3d/ProgramD3D.h"

#include "common/utilities.h"
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
      mVertexUniformStorage(NULL),
      mFragmentUniformStorage(NULL)
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

bool ProgramD3D::load(gl::InfoLog &infoLog, gl::BinaryInputStream *stream)
{
    stream->readString(&mVertexHLSL);
    stream->readInt(&mVertexWorkarounds);
    stream->readString(&mPixelHLSL);
    stream->readInt(&mPixelWorkarounds);
    stream->readBool(&mUsesFragDepth);

    const size_t pixelShaderKeySize = stream->readInt<unsigned int>();
    mPixelShaderKey.resize(pixelShaderKeySize);
    for (size_t pixelShaderKeyIndex = 0; pixelShaderKeyIndex < pixelShaderKeySize; pixelShaderKeyIndex++)
    {
        stream->readInt(&mPixelShaderKey[pixelShaderKeyIndex].type);
        stream->readString(&mPixelShaderKey[pixelShaderKeyIndex].name);
        stream->readString(&mPixelShaderKey[pixelShaderKeyIndex].source);
        stream->readInt(&mPixelShaderKey[pixelShaderKeyIndex].outputIndex);
    }

    return true;
}

bool ProgramD3D::save(gl::BinaryOutputStream *stream)
{
    stream->writeString(mVertexHLSL);
    stream->writeInt(mVertexWorkarounds);
    stream->writeString(mPixelHLSL);
    stream->writeInt(mPixelWorkarounds);
    stream->writeInt(mUsesFragDepth);

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

    return true;
}

rx::ShaderExecutable *ProgramD3D::getPixelExecutableForOutputLayout(gl::InfoLog &infoLog, const std::vector<GLenum> &outputSignature,
                                                                    const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                                                    bool separatedOutputBuffers)
{
    std::string finalPixelHLSL = mDynamicHLSL->generatePixelShaderForOutputSignature(mPixelHLSL, mPixelShaderKey, mUsesFragDepth,
                                                                                     outputSignature);

    // Generate new pixel executable
    rx::ShaderExecutable *pixelExecutable = mRenderer->compileToExecutable(infoLog, finalPixelHLSL.c_str(), rx::SHADER_PIXEL,
                                                                           transformFeedbackLinkedVaryings, separatedOutputBuffers,
                                                                           mPixelWorkarounds);

    return pixelExecutable;
}

rx::ShaderExecutable *ProgramD3D::getVertexExecutableForInputLayout(gl::InfoLog &infoLog,
                                                                    const gl::VertexFormat inputLayout[gl::MAX_VERTEX_ATTRIBS],
                                                                    const sh::Attribute shaderAttributes[],
                                                                    const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                                                    bool separatedOutputBuffers)
{
    // Generate new dynamic layout with attribute conversions
    std::string finalVertexHLSL = mDynamicHLSL->generateVertexShaderForInputLayout(mVertexHLSL, inputLayout, shaderAttributes);

    // Generate new vertex executable
    rx::ShaderExecutable *vertexExecutable = mRenderer->compileToExecutable(infoLog, finalVertexHLSL.c_str(),
                                                                            rx::SHADER_VERTEX,
                                                                            transformFeedbackLinkedVaryings, separatedOutputBuffers,
                                                                            mVertexWorkarounds);

    return vertexExecutable;
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

    return true;
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

void ProgramD3D::reset()
{
    mVertexHLSL.clear();
    mVertexWorkarounds = rx::ANGLE_D3D_WORKAROUND_NONE;

    mPixelHLSL.clear();
    mPixelWorkarounds = rx::ANGLE_D3D_WORKAROUND_NONE;
    mUsesFragDepth = false;
    mPixelShaderKey.clear();

    SafeDelete(mVertexUniformStorage);
    SafeDelete(mFragmentUniformStorage);
}

}
