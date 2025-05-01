//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramExecutableWgpu.cpp: Implementation of ProgramExecutableWgpu.

#include "libANGLE/renderer/wgpu/ProgramExecutableWgpu.h"

#include <iterator>

#include "angle_gl.h"
#include "anglebase/numerics/safe_conversions.h"
#include "common/PackedGLEnums_autogen.h"
#include "common/log_utils.h"
#include "common/mathutil.h"
#include "common/utilities.h"
#include "compiler/translator/wgsl/OutputUniformBlocks.h"
#include "libANGLE/Error.h"
#include "libANGLE/Program.h"
#include "libANGLE/ProgramExecutable.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/wgpu/ContextWgpu.h"
#include "libANGLE/renderer/wgpu/TextureWgpu.h"
#include "libANGLE/renderer/wgpu/wgpu_helpers.h"
#include "libANGLE/renderer/wgpu/wgpu_pipeline_state.h"
#include "libANGLE/renderer/wgpu/wgpu_utils.h"

namespace rx
{

ProgramExecutableWgpu::ProgramExecutableWgpu(const gl::ProgramExecutable *executable)
    : ProgramExecutableImpl(executable)
{
    for (std::shared_ptr<BufferAndLayout> &defaultBlock : mDefaultUniformBlocks)
    {
        defaultBlock = std::make_shared<BufferAndLayout>();
    }
}

ProgramExecutableWgpu::~ProgramExecutableWgpu() = default;

void ProgramExecutableWgpu::destroy(const gl::Context *context) {}

angle::Result ProgramExecutableWgpu::updateUniformsAndGetBindGroup(ContextWgpu *contextWgpu,
                                                                   wgpu::BindGroup *outBindGroup)
{
    if (mDefaultUniformBlocksDirty.any())
    {
        // TODO(anglebug.com/376553328): this creates an entire new buffer every time a single
        // uniform changes, and the old ones are just garbage collected. This should be optimized.
        webgpu::BufferHelper defaultUniformBuffer;

        gl::ShaderMap<uint64_t> offsets =
            {};  // offset in the GPU-side buffer of each shader stage's uniform data.
        size_t requiredSpace;

        angle::CheckedNumeric<size_t> requiredSpaceChecked =
            calcUniformUpdateRequiredSpace(contextWgpu, &offsets);
        if (!requiredSpaceChecked.AssignIfValid(&requiredSpace))
        {
            return angle::Result::Stop;
        }

        ANGLE_TRY(defaultUniformBuffer.initBuffer(
            contextWgpu->getDevice(), requiredSpace,
            wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, webgpu::MapAtCreation::Yes));

        ASSERT(defaultUniformBuffer.valid());

        // Copy all of the CPU-side data into this buffer which will be visible to the GPU after it
        // is unmapped here on the CPU.
        uint8_t *bufferData = defaultUniformBuffer.getMapWritePointer(0, requiredSpace);
        for (gl::ShaderType shaderType : mExecutable->getLinkedShaderStages())
        {
            const angle::MemoryBuffer &uniformData = mDefaultUniformBlocks[shaderType]->uniformData;
            memcpy(&bufferData[offsets[shaderType]], uniformData.data(), uniformData.size());
            mDefaultUniformBlocksDirty.reset(shaderType);
        }
        ANGLE_TRY(defaultUniformBuffer.unmap());

        // Create the BindGroupEntries
        std::vector<wgpu::BindGroupEntry> bindings;
        auto addBindingToGroupIfNecessary = [&](uint32_t bindingIndex, gl::ShaderType shaderType) {
            if (mDefaultUniformBlocks[shaderType]->uniformData.size() != 0)
            {
                wgpu::BindGroupEntry bindGroupEntry;
                bindGroupEntry.binding = bindingIndex;
                bindGroupEntry.buffer  = defaultUniformBuffer.getBuffer();
                bindGroupEntry.offset  = offsets[shaderType];
                bindGroupEntry.size    = mDefaultUniformBlocks[shaderType]->uniformData.size();
                bindings.push_back(bindGroupEntry);
            }
        };

        // Add the BindGroupEntry for the default blocks of both the vertex and fragment shaders.
        // They will use the same buffer with a different offset.
        addBindingToGroupIfNecessary(sh::kDefaultVertexUniformBlockBinding, gl::ShaderType::Vertex);
        addBindingToGroupIfNecessary(sh::kDefaultFragmentUniformBlockBinding,
                                     gl::ShaderType::Fragment);

        wgpu::BindGroupDescriptor bindGroupDesc{};
        bindGroupDesc.layout = mDefaultBindGroupLayout;
        // There must be as many bindings as declared in the layout!
        bindGroupDesc.entryCount = bindings.size();
        bindGroupDesc.entries    = bindings.data();
        mDefaultBindGroup        = contextWgpu->getDevice().CreateBindGroup(&bindGroupDesc);
    }

    ASSERT(mDefaultBindGroup);
    *outBindGroup = mDefaultBindGroup;

    return angle::Result::Continue;
}

angle::Result ProgramExecutableWgpu::getSamplerAndTextureBindGroup(ContextWgpu *contextWgpu,
                                                                   wgpu::BindGroup *outBindGroup)
{
    if (mSamplerBindingsDirty)
    {
        const gl::ActiveTexturesCache &completeTextures =
            contextWgpu->getState().getActiveTexturesCache();

        std::vector<wgpu::BindGroupEntry> bindings;
        bindings.reserve(mExecutable->getSamplerBindings().size() * 2);

        for (uint32_t textureIndex = 0; textureIndex < mExecutable->getSamplerBindings().size();
             ++textureIndex)
        {
            const gl::SamplerBinding &samplerBinding =
                mExecutable->getSamplerBindings()[textureIndex];

            if (samplerBinding.textureUnitsCount != 1)
            {
                // TODO(anglebug.com/389145696): arrays of samplers not yet supported
                UNIMPLEMENTED();
                return angle::Result::Stop;
            }
            for (uint32_t arrayElement = 0; arrayElement < samplerBinding.textureUnitsCount;
                 ++arrayElement)
            {
                GLuint textureUnit = samplerBinding.getTextureUnit(
                    mExecutable->getSamplerBoundTextureUnits(), arrayElement);
                gl::Texture *texture = completeTextures[textureUnit];
                gl::Sampler *sampler = contextWgpu->getState().getSampler(textureUnit);
                uint32_t samplerSlot = (textureIndex + arrayElement) * 2;
                uint32_t textureSlot = samplerSlot + 1;
                if (!texture)
                {
                    // TODO(anglebug.com/389145696): no support for incomplete textures.
                    UNIMPLEMENTED();
                    return angle::Result::Stop;
                }
                const gl::SamplerState *samplerState =
                    sampler ? &sampler->getSamplerState() : &texture->getSamplerState();
                if (samplerBinding.format == gl::SamplerFormat::Shadow)
                {
                    // TODO(anglebug.com/389145696): no support for shadow samplers yet.
                    UNIMPLEMENTED();
                    return angle::Result::Stop;
                }
                TextureWgpu *textureWgpu = webgpu::GetImpl(texture);

                // TODO(anglebug.com/389145696): potentially cache sampler.
                wgpu::SamplerDescriptor sampleDesc = gl_wgpu::GetWgpuSamplerDesc(samplerState);
                // TODO(geofflang): Store this sampler since the descriptor will not hold a ref.
                wgpu::Sampler wgpuSampler = contextWgpu->getDevice().CreateSampler(&sampleDesc);

                wgpu::BindGroupEntry samplerBindGroupEntry;
                samplerBindGroupEntry.binding = samplerSlot;
                samplerBindGroupEntry.sampler = wgpuSampler;

                bindings.push_back(samplerBindGroupEntry);

                wgpu::BindGroupEntry textureBindGroupEntry;
                textureBindGroupEntry.binding = textureSlot;

                webgpu::TextureViewHandle textureView;
                ANGLE_TRY(textureWgpu->getImage()->createFullTextureView(
                    textureView,
                    /*desiredViewDimension=*/gl_wgpu::GetWgpuTextureViewDimension(
                        samplerBinding.textureType)));
                // TODO(geofflang): Store this texture view since the descriptor will not hold a
                // ref.
                textureBindGroupEntry.textureView = wgpu::TextureView(textureView.get());
                bindings.push_back(textureBindGroupEntry);

            }  // for array elements
        }  // for sampler bindings

        // A bind group contains one or multiple bindings
        wgpu::BindGroupDescriptor bindGroupDesc{};
        ASSERT(mSamplersAndTexturesBindGroupLayout);
        bindGroupDesc.layout = mSamplersAndTexturesBindGroupLayout;
        // There must be as many bindings as declared in the layout!
        bindGroupDesc.entryCount      = bindings.size();
        bindGroupDesc.entries         = bindings.data();
        mSamplersAndTexturesBindGroup = contextWgpu->getDevice().CreateBindGroup(&bindGroupDesc);

        mSamplerBindingsDirty = false;
    }

    ASSERT(mSamplersAndTexturesBindGroup);
    *outBindGroup = mSamplersAndTexturesBindGroup;

    return angle::Result::Continue;
}

angle::CheckedNumeric<size_t> ProgramExecutableWgpu::getDefaultUniformAlignedSize(
    ContextWgpu *context,
    gl::ShaderType shaderType) const
{
    size_t alignment = angle::base::checked_cast<size_t>(
        context->getDisplay()->getLimitsWgpu().minUniformBufferOffsetAlignment);
    return CheckedRoundUp(mDefaultUniformBlocks[shaderType]->uniformData.size(), alignment);
}

angle::CheckedNumeric<size_t> ProgramExecutableWgpu::calcUniformUpdateRequiredSpace(
    ContextWgpu *context,
    gl::ShaderMap<uint64_t> *uniformOffsets) const
{
    angle::CheckedNumeric<size_t> requiredSpace = 0;
    for (gl::ShaderType shaderType : mExecutable->getLinkedShaderStages())
    {
        (*uniformOffsets)[shaderType] = requiredSpace.ValueOrDie();
        requiredSpace += getDefaultUniformAlignedSize(context, shaderType);
        if (!requiredSpace.IsValid())
        {
            break;
        }
    }
    return requiredSpace;
}

angle::Result ProgramExecutableWgpu::resizeUniformBlockMemory(
    const gl::ShaderMap<size_t> &requiredBufferSize)
{
    for (gl::ShaderType shaderType : mExecutable->getLinkedShaderStages())
    {
        if (requiredBufferSize[shaderType] > 0)
        {
            if (!mDefaultUniformBlocks[shaderType]->uniformData.resize(
                    requiredBufferSize[shaderType]))
            {
                return angle::Result::Stop;
            }

            // Initialize uniform buffer memory to zero by default.
            mDefaultUniformBlocks[shaderType]->uniformData.fill(0);
            mDefaultUniformBlocksDirty.set(shaderType);
        }
    }

    return angle::Result::Continue;
}

void ProgramExecutableWgpu::markDefaultUniformsDirty()
{
    // Mark all linked stages as having dirty default uniforms
    mDefaultUniformBlocksDirty = getExecutable()->getLinkedShaderStages();
}

void ProgramExecutableWgpu::setUniform1fv(GLint location, GLsizei count, const GLfloat *v)
{
    SetUniform(mExecutable, location, count, v, GL_FLOAT, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    SetUniform(mExecutable, location, count, v, GL_FLOAT_VEC2, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    SetUniform(mExecutable, location, count, v, GL_FLOAT_VEC3, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    SetUniform(mExecutable, location, count, v, GL_FLOAT_VEC4, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    const gl::VariableLocation &locationInfo = mExecutable->getUniformLocations()[location];
    const gl::LinkedUniform &linkedUniform   = mExecutable->getUniforms()[locationInfo.index];
    if (linkedUniform.isSampler())
    {
        mSamplerBindingsDirty = true;
        return;
    }

    SetUniform(mExecutable, location, count, v, GL_INT, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    SetUniform(mExecutable, location, count, v, GL_INT_VEC2, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    SetUniform(mExecutable, location, count, v, GL_INT_VEC3, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    SetUniform(mExecutable, location, count, v, GL_INT_VEC4, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
    SetUniform(mExecutable, location, count, v, GL_UNSIGNED_INT, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
    SetUniform(mExecutable, location, count, v, GL_UNSIGNED_INT_VEC2, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
    SetUniform(mExecutable, location, count, v, GL_UNSIGNED_INT_VEC3, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
    SetUniform(mExecutable, location, count, v, GL_UNSIGNED_INT_VEC4, &mDefaultUniformBlocks,
               &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix2fv(GLint location,
                                                GLsizei count,
                                                GLboolean transpose,
                                                const GLfloat *value)
{
    SetUniformMatrixfv<2, 2>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix3fv(GLint location,
                                                GLsizei count,
                                                GLboolean transpose,
                                                const GLfloat *value)
{
    SetUniformMatrixfv<3, 3>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix4fv(GLint location,
                                                GLsizei count,
                                                GLboolean transpose,
                                                const GLfloat *value)
{
    SetUniformMatrixfv<4, 4>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix2x3fv(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat *value)
{
    SetUniformMatrixfv<2, 3>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix3x2fv(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat *value)
{
    SetUniformMatrixfv<3, 2>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix2x4fv(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat *value)
{
    SetUniformMatrixfv<2, 4>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix4x2fv(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat *value)
{
    SetUniformMatrixfv<4, 2>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix3x4fv(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat *value)
{
    SetUniformMatrixfv<3, 4>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::setUniformMatrix4x3fv(GLint location,
                                                  GLsizei count,
                                                  GLboolean transpose,
                                                  const GLfloat *value)
{
    SetUniformMatrixfv<4, 3>(mExecutable, location, count, transpose, value, &mDefaultUniformBlocks,
                             &mDefaultUniformBlocksDirty);
}

void ProgramExecutableWgpu::getUniformfv(const gl::Context *context,
                                         GLint location,
                                         GLfloat *params) const
{
    GetUniform(mExecutable, location, params, GL_FLOAT, &mDefaultUniformBlocks);
}

void ProgramExecutableWgpu::getUniformiv(const gl::Context *context,
                                         GLint location,
                                         GLint *params) const
{
    GetUniform(mExecutable, location, params, GL_INT, &mDefaultUniformBlocks);
}

void ProgramExecutableWgpu::getUniformuiv(const gl::Context *context,
                                          GLint location,
                                          GLuint *params) const
{
    GetUniform(mExecutable, location, params, GL_UNSIGNED_INT, &mDefaultUniformBlocks);
}

TranslatedWGPUShaderModule &ProgramExecutableWgpu::getShaderModule(gl::ShaderType type)
{
    return mShaderModules[type];
}

angle::Result ProgramExecutableWgpu::getRenderPipeline(ContextWgpu *context,
                                                       const webgpu::RenderPipelineDesc &desc,
                                                       webgpu::RenderPipelineHandle *pipelineOut)
{
    gl::ShaderMap<webgpu::ShaderModuleHandle> shaders;
    for (gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        shaders[shaderType] = mShaderModules[shaderType].module;
    }

    genBindingLayoutIfNecessary(context);

    return mPipelineCache.getRenderPipeline(context, desc, mPipelineLayout, shaders, pipelineOut);
}

void ProgramExecutableWgpu::genBindingLayoutIfNecessary(ContextWgpu *context)
{
    if (mPipelineLayout)
    {
        return;
    }
    // TODO(anglebug.com/42267100): for now, only create a wgpu::PipelineLayout with the default
    // uniform block, driver uniform block, and textures/samplers. Will need to be extended for
    // UBOs. Also, possibly provide this layout as a compilation hint to createShaderModule().

    std::vector<wgpu::BindGroupLayoutEntry> defaultBindGroupLayoutEntries;
    auto addDefaultBindGroupLayoutEntryIfNecessary =
        [&](uint32_t bindingIndex, gl::ShaderType shaderType, wgpu::ShaderStage wgpuVisibility) {
            if (mDefaultUniformBlocks[shaderType]->uniformData.size() != 0)
            {
                wgpu::BindGroupLayoutEntry bindGroupLayoutEntry;
                bindGroupLayoutEntry.visibility  = wgpuVisibility;
                bindGroupLayoutEntry.binding     = bindingIndex;
                bindGroupLayoutEntry.buffer.type = wgpu::BufferBindingType::Uniform;
                // By setting a `minBindingSize`, some validation is pushed from every draw call to
                // pipeline creation time.
                bindGroupLayoutEntry.buffer.minBindingSize =
                    mDefaultUniformBlocks[shaderType]->uniformData.size();
                defaultBindGroupLayoutEntries.push_back(bindGroupLayoutEntry);
            }
        };
    // Default uniform blocks for each of the vertex shader and the fragment shader.
    addDefaultBindGroupLayoutEntryIfNecessary(sh::kDefaultVertexUniformBlockBinding,
                                              gl::ShaderType::Vertex, wgpu::ShaderStage::Vertex);
    addDefaultBindGroupLayoutEntryIfNecessary(sh::kDefaultFragmentUniformBlockBinding,
                                              gl::ShaderType::Fragment,
                                              wgpu::ShaderStage::Fragment);

    // Create a bind group layout with these entries.
    wgpu::BindGroupLayoutDescriptor defaultBindGroupLayoutDesc{};
    defaultBindGroupLayoutDesc.entryCount = defaultBindGroupLayoutEntries.size();
    defaultBindGroupLayoutDesc.entries    = defaultBindGroupLayoutEntries.data();
    mDefaultBindGroupLayout =
        context->getDevice().CreateBindGroupLayout(&defaultBindGroupLayoutDesc);

    // Add the textures/samplers to the second bind group.
    std::vector<wgpu::BindGroupLayoutEntry> samplersAndTexturesBindGroupLayoutEntries;

    // For each sampler binding, the translator should have generated 2 WGSL bindings, a sampler and
    // a texture, with incrementing binding numbers starting from 0.
    for (size_t i = 0; i < mExecutable->getSamplerBindings().size(); i++)
    {
        const gl::SamplerBinding &samplerBinding = mExecutable->getSamplerBindings()[i];

        wgpu::BindGroupLayoutEntry samplerBindGroupLayoutEntry;
        samplerBindGroupLayoutEntry.visibility =
            wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        samplerBindGroupLayoutEntry.binding = angle::base::checked_cast<uint32_t>(i * 2);
        samplerBindGroupLayoutEntry.sampler.type =
            samplerBinding.format == gl::SamplerFormat::Shadow
                ? wgpu::SamplerBindingType::Comparison
                : wgpu::SamplerBindingType::Filtering;
        samplersAndTexturesBindGroupLayoutEntries.push_back(samplerBindGroupLayoutEntry);

        wgpu::BindGroupLayoutEntry textureBindGroupLayoutEntry;
        textureBindGroupLayoutEntry.visibility =
            wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
        textureBindGroupLayoutEntry.binding = angle::base::checked_cast<uint32_t>(i * 2 + 1);
        textureBindGroupLayoutEntry.texture.sampleType =
            gl_wgpu::GetTextureSampleType(samplerBinding.format);
        textureBindGroupLayoutEntry.texture.viewDimension = static_cast<wgpu::TextureViewDimension>(
            gl_wgpu::GetWgpuTextureViewDimension(samplerBinding.textureType));
        samplersAndTexturesBindGroupLayoutEntries.push_back(textureBindGroupLayoutEntry);
    }

    // Create a bind group layout with these entries.
    wgpu::BindGroupLayoutDescriptor texturesAndSamplersBindGroupLayoutDesc{};
    texturesAndSamplersBindGroupLayoutDesc.entryCount =
        samplersAndTexturesBindGroupLayoutEntries.size();
    texturesAndSamplersBindGroupLayoutDesc.entries =
        samplersAndTexturesBindGroupLayoutEntries.data();
    mSamplersAndTexturesBindGroupLayout =
        context->getDevice().CreateBindGroupLayout(&texturesAndSamplersBindGroupLayoutDesc);

    // Driver uniforms bind groups are handled by ContextWgpu.

    // TODO(anglebug.com/376553328): now add UBO bindings.

    // Create the pipeline layout. This is a list where each element N corresponds to the
    // @group(N) in the compiled shaders.
    std::array<wgpu::BindGroupLayout, sh::kMaxBindGroup + 1> groupLayouts = {};

    groupLayouts[sh::kDefaultUniformBlockBindGroup] = mDefaultBindGroupLayout;
    groupLayouts[sh::kTextureAndSamplerBindGroup]   = mSamplersAndTexturesBindGroupLayout;
    groupLayouts[sh::kDriverUniformBindGroup]       = context->getDriverUniformBindGroupLayout();
    static_assert(sh::kDriverUniformBindGroup == sh::kMaxBindGroup,
                  "More bind groups added without changing the layout");

    wgpu::PipelineLayoutDescriptor layoutDesc{};
    layoutDesc.bindGroupLayoutCount = groupLayouts.size();
    layoutDesc.bindGroupLayouts     = groupLayouts.data();
    mPipelineLayout                 = context->getDevice().CreatePipelineLayout(&layoutDesc);
}

}  // namespace rx
