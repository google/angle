//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DispatchUtilsVk.cpp:
//    Implements the DispatchUtilsVk class.
//

#include "libANGLE/renderer/vulkan/DispatchUtilsVk.h"

#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/TextureVk.h"

namespace rx
{

namespace BufferUtils_comp = vk::InternalShader::BufferUtils_comp;

namespace
{
// All internal shaders assume there is only one descriptor set, indexed at 0
constexpr uint32_t kSetIndex = 0;

constexpr uint32_t kClearOutputBinding     = 0;
constexpr uint32_t kCopyDestinationBinding = 0;
constexpr uint32_t kCopySourceBinding      = 1;

uint32_t GetBufferUtilsFlags(size_t dispatchSize, const vk::Format &format)
{
    uint32_t flags                    = dispatchSize % 64 == 0 ? BufferUtils_comp::kIsAligned : 0;
    const angle::Format &bufferFormat = format.bufferFormat();

    flags |= bufferFormat.componentType == GL_INT
                 ? BufferUtils_comp::kIsInt
                 : bufferFormat.componentType == GL_UNSIGNED_INT ? BufferUtils_comp::kIsUint
                                                                 : BufferUtils_comp::kIsFloat;

    return flags;
}
}  // namespace

DispatchUtilsVk::DispatchUtilsVk() = default;

DispatchUtilsVk::~DispatchUtilsVk() = default;

void DispatchUtilsVk::destroy(VkDevice device)
{
    for (Function f : angle::AllEnums<Function>())
    {
        for (auto &descriptorSetLayout : mDescriptorSetLayouts[f])
        {
            descriptorSetLayout.reset();
        }
        mPipelineLayouts[f].reset();
        mDescriptorPools[f].destroy(device);
    }

    for (vk::ShaderProgramHelper &program : mPrograms)
    {
        program.destroy(device);
    }
}

angle::Result DispatchUtilsVk::ensureResourcesInitialized(vk::Context *context,
                                                          Function function,
                                                          VkDescriptorPoolSize *setSizes,
                                                          size_t setSizesCount)
{
    RendererVk *renderer = context->getRenderer();

    vk::DescriptorSetLayoutDesc descriptorSetDesc;

    uint32_t currentBinding = 0;
    for (size_t i = 0; i < setSizesCount; ++i)
    {
        descriptorSetDesc.update(currentBinding, setSizes[i].type, setSizes[i].descriptorCount);
        currentBinding += setSizes[i].descriptorCount;
    }

    ANGLE_TRY(renderer->getDescriptorSetLayout(context, descriptorSetDesc,
                                               &mDescriptorSetLayouts[function][kSetIndex]));

    // Corresponding pipeline layouts:
    vk::PipelineLayoutDesc pipelineLayoutDesc;

    pipelineLayoutDesc.updateDescriptorSetLayout(kSetIndex, descriptorSetDesc);
    pipelineLayoutDesc.updatePushConstantRange(gl::ShaderType::Compute, 0, sizeof(ShaderParams));

    ANGLE_TRY(renderer->getPipelineLayout(
        context, pipelineLayoutDesc, mDescriptorSetLayouts[function], &mPipelineLayouts[function]));

    ANGLE_TRY(mDescriptorPools[function].init(context, setSizes, setSizesCount));

    return angle::Result::Continue();
}

angle::Result DispatchUtilsVk::ensureBufferClearInitialized(vk::Context *context)
{
    if (mPipelineLayouts[Function::BufferClear].valid())
    {
        return angle::Result::Continue();
    }

    VkDescriptorPoolSize setSizes[1] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1},
    };

    return ensureResourcesInitialized(context, Function::BufferClear, setSizes,
                                      ArraySize(setSizes));
}

angle::Result DispatchUtilsVk::ensureBufferCopyInitialized(vk::Context *context)
{
    if (mPipelineLayouts[Function::BufferCopy].valid())
    {
        return angle::Result::Continue();
    }

    VkDescriptorPoolSize setSizes[2] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1},
    };

    return ensureResourcesInitialized(context, Function::BufferCopy, setSizes, ArraySize(setSizes));
}

angle::Result DispatchUtilsVk::setupProgram(vk::Context *context,
                                            uint32_t function,
                                            const VkDescriptorSet &descriptorSet,
                                            const ShaderParams &params,
                                            vk::CommandBuffer *commandBuffer)
{
    RendererVk *renderer             = context->getRenderer();
    vk::ShaderLibrary &shaderLibrary = renderer->getShaderLibrary();

    vk::RefCounted<vk::ShaderAndSerial> *shader = nullptr;
    ANGLE_TRY(shaderLibrary.getBufferUtils_comp(context, function, &shader));

    mPrograms[function].setShader(gl::ShaderType::Compute, shader);

    bool isClear = (function & BufferUtils_comp::kFunctionMask) == BufferUtils_comp::kIsClear;
    Function pipelineLayoutIndex = isClear ? Function::BufferClear : Function::BufferCopy;
    const vk::BindingPointer<vk::PipelineLayout> &pipelineLayout =
        mPipelineLayouts[pipelineLayoutIndex];

    vk::PipelineAndSerial *pipelineAndSerial;
    ANGLE_TRY(
        mPrograms[function].getComputePipeline(context, pipelineLayout.get(), &pipelineAndSerial));

    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, pipelineAndSerial->get());
    pipelineAndSerial->updateSerial(renderer->getCurrentQueueSerial());

    commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout.get(), 0, 1,
                                      &descriptorSet, 0, nullptr);
    commandBuffer->pushConstants(pipelineLayout.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                 sizeof(params), &params);

    return angle::Result::Continue();
}

angle::Result DispatchUtilsVk::clearBuffer(vk::Context *context,
                                           vk::BufferHelper *dest,
                                           const ClearParameters &params)
{
    ANGLE_TRY(ensureBufferClearInitialized(context));

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(dest->recordCommands(context, &commandBuffer));

    // Tell dest it's being written to.
    dest->onWrite(VK_ACCESS_SHADER_WRITE_BIT);

    const vk::Format &destFormat = dest->getViewFormat();

    uint32_t function = BufferUtils_comp::kIsClear | GetBufferUtilsFlags(params.size, destFormat);

    ShaderParams shaderParams;
    shaderParams.destOffset = params.offset;
    shaderParams.size       = params.size;
    shaderParams.clearValue = params.clearValue;

    VkDescriptorSet descriptorSet;
    vk::SharedDescriptorPoolBinding descriptorPoolBinding;
    ANGLE_TRY(mDescriptorPools[Function::BufferClear].allocateSets(
        context, mDescriptorSetLayouts[Function::BufferClear][kSetIndex].get().ptr(), 1,
        &descriptorPoolBinding, &descriptorSet));

    VkWriteDescriptorSet writeInfo = {};

    writeInfo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet           = descriptorSet;
    writeInfo.dstBinding       = kClearOutputBinding;
    writeInfo.descriptorCount  = 1;
    writeInfo.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    writeInfo.pTexelBufferView = dest->getBufferView().ptr();

    vkUpdateDescriptorSets(context->getDevice(), 1, &writeInfo, 0, nullptr);

    ANGLE_TRY(setupProgram(context, function, descriptorSet, shaderParams, commandBuffer));

    commandBuffer->dispatch(UnsignedCeilDivide(params.size, 64), 1, 1);

    descriptorPoolBinding.reset();

    return angle::Result::Continue();
}

angle::Result DispatchUtilsVk::copyBuffer(vk::Context *context,
                                          vk::BufferHelper *dest,
                                          vk::BufferHelper *src,
                                          const CopyParameters &params)
{
    ANGLE_TRY(ensureBufferCopyInitialized(context));

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(dest->recordCommands(context, &commandBuffer));

    // Tell src we are going to read from it.
    src->onRead(dest, VK_ACCESS_SHADER_READ_BIT);
    // Tell dest it's being written to.
    dest->onWrite(VK_ACCESS_SHADER_WRITE_BIT);

    const vk::Format &destFormat = dest->getViewFormat();
    const vk::Format &srcFormat  = src->getViewFormat();

    ASSERT(destFormat.vkFormatIsInt == srcFormat.vkFormatIsInt);
    ASSERT(destFormat.vkFormatIsUnsigned == srcFormat.vkFormatIsUnsigned);

    uint32_t function = BufferUtils_comp::kIsCopy | GetBufferUtilsFlags(params.size, destFormat);

    ShaderParams shaderParams;
    shaderParams.destOffset = params.destOffset;
    shaderParams.size       = params.size;
    shaderParams.srcOffset  = params.srcOffset;

    VkDescriptorSet descriptorSet;
    vk::SharedDescriptorPoolBinding descriptorPoolBinding;
    ANGLE_TRY(mDescriptorPools[Function::BufferCopy].allocateSets(
        context, mDescriptorSetLayouts[Function::BufferCopy][kSetIndex].get().ptr(), 1,
        &descriptorPoolBinding, &descriptorSet));

    VkWriteDescriptorSet writeInfo[2] = {};

    writeInfo[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[0].dstSet           = descriptorSet;
    writeInfo[0].dstBinding       = kCopyDestinationBinding;
    writeInfo[0].descriptorCount  = 1;
    writeInfo[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    writeInfo[0].pTexelBufferView = dest->getBufferView().ptr();

    writeInfo[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[1].dstSet           = descriptorSet;
    writeInfo[1].dstBinding       = kCopySourceBinding;
    writeInfo[1].descriptorCount  = 1;
    writeInfo[1].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    writeInfo[1].pTexelBufferView = src->getBufferView().ptr();

    vkUpdateDescriptorSets(context->getDevice(), 2, writeInfo, 0, nullptr);

    ANGLE_TRY(setupProgram(context, function, descriptorSet, shaderParams, commandBuffer));

    commandBuffer->dispatch(UnsignedCeilDivide(params.size, 64), 1, 1);

    descriptorPoolBinding.reset();

    return angle::Result::Continue();
}

}  // namespace rx
