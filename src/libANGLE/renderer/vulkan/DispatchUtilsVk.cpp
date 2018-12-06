//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DispatchUtilsVk.cpp:
//    Implements the DispatchUtilsVk class.
//

#include "libANGLE/renderer/vulkan/DispatchUtilsVk.h"

#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{

namespace BufferUtils_comp   = vk::InternalShader::BufferUtils_comp;
namespace ConvertVertex_comp = vk::InternalShader::ConvertVertex_comp;

namespace
{
// All internal shaders assume there is only one descriptor set, indexed at 0
constexpr uint32_t kSetIndex = 0;

constexpr uint32_t kBufferClearOutputBinding        = 0;
constexpr uint32_t kBufferCopyDestinationBinding    = 0;
constexpr uint32_t kBufferCopySourceBinding         = 1;
constexpr uint32_t kConvertVertexDestinationBinding = 0;
constexpr uint32_t kConvertVertexSourceBinding      = 1;

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

uint32_t GetConvertVertexFlags(const DispatchUtilsVk::ConvertVertexParameters &params)
{
    bool srcIsInt   = params.srcFormat->componentType == GL_INT;
    bool srcIsUint  = params.srcFormat->componentType == GL_UNSIGNED_INT;
    bool srcIsSnorm = params.srcFormat->componentType == GL_SIGNED_NORMALIZED;
    bool srcIsUnorm = params.srcFormat->componentType == GL_UNSIGNED_NORMALIZED;
    bool srcIsFixed = params.srcFormat->isFixed;
    bool srcIsFloat = params.srcFormat->componentType == GL_FLOAT;

    bool destIsInt   = params.destFormat->componentType == GL_INT;
    bool destIsUint  = params.destFormat->componentType == GL_UNSIGNED_INT;
    bool destIsFloat = params.destFormat->componentType == GL_FLOAT;

    // Assert on the types to make sure the shader supports its.  These are based on
    // ConvertVertex_comp::Conversion values.
    ASSERT(!destIsInt || srcIsInt);      // If destination is int, src must be int too
    ASSERT(!destIsUint || srcIsUint);    // If destination is uint, src must be uint too
    ASSERT(!srcIsFixed || destIsFloat);  // If source is fixed, dest must be float
    // One of each bool set must be true
    ASSERT(srcIsInt || srcIsUint || srcIsSnorm || srcIsUnorm || srcIsFixed || srcIsFloat);
    ASSERT(destIsInt || destIsUint || destIsFloat);

    // We currently don't have any big-endian devices in the list of supported platforms.  The
    // shader is capable of supporting big-endian architectures, but the relevant flag (IsBigEndian)
    // is not added to the build configuration file (to reduce binary size).  If necessary, add
    // IsBigEndian to ConvertVertex.comp.json and select the appropriate flag based on the
    // endian-ness test here.
    uint32_t endiannessTest                       = 0;
    *reinterpret_cast<uint8_t *>(&endiannessTest) = 1;
    ASSERT(endiannessTest == 1);

    uint32_t flags = 0;

    if (srcIsInt && destIsInt)
    {
        flags |= ConvertVertex_comp::kIntToInt;
    }
    else if (srcIsUint && destIsUint)
    {
        flags |= ConvertVertex_comp::kUintToUint;
    }
    else if (srcIsInt)
    {
        flags |= ConvertVertex_comp::kIntToFloat;
    }
    else if (srcIsUint)
    {
        flags |= ConvertVertex_comp::kUintToFloat;
    }
    else if (srcIsSnorm)
    {
        flags |= ConvertVertex_comp::kSnormToFloat;
    }
    else if (srcIsUnorm)
    {
        flags |= ConvertVertex_comp::kUnormToFloat;
    }
    else if (srcIsFixed)
    {
        flags |= ConvertVertex_comp::kFixedToFloat;
    }
    else if (srcIsFloat)
    {
        flags |= ConvertVertex_comp::kFloatToFloat;
    }
    else
    {
        UNREACHABLE();
    }

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

    for (vk::ShaderProgramHelper &program : mBufferUtilsPrograms)
    {
        program.destroy(device);
    }
    for (vk::ShaderProgramHelper &program : mConvertVertexPrograms)
    {
        program.destroy(device);
    }
}

angle::Result DispatchUtilsVk::ensureResourcesInitialized(vk::Context *context,
                                                          Function function,
                                                          VkDescriptorPoolSize *setSizes,
                                                          size_t setSizesCount,
                                                          size_t pushConstantsSize)
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
    pipelineLayoutDesc.updatePushConstantRange(gl::ShaderType::Compute, 0, pushConstantsSize);

    ANGLE_TRY(renderer->getPipelineLayout(
        context, pipelineLayoutDesc, mDescriptorSetLayouts[function], &mPipelineLayouts[function]));

    ANGLE_TRY(mDescriptorPools[function].init(context, setSizes, setSizesCount));

    return angle::Result::Continue;
}

angle::Result DispatchUtilsVk::ensureBufferClearInitialized(vk::Context *context)
{
    if (mPipelineLayouts[Function::BufferClear].valid())
    {
        return angle::Result::Continue;
    }

    VkDescriptorPoolSize setSizes[1] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1},
    };

    return ensureResourcesInitialized(context, Function::BufferClear, setSizes, ArraySize(setSizes),
                                      sizeof(BufferUtilsShaderParams));
}

angle::Result DispatchUtilsVk::ensureBufferCopyInitialized(vk::Context *context)
{
    if (mPipelineLayouts[Function::BufferCopy].valid())
    {
        return angle::Result::Continue;
    }

    VkDescriptorPoolSize setSizes[2] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1},
    };

    return ensureResourcesInitialized(context, Function::BufferCopy, setSizes, ArraySize(setSizes),
                                      sizeof(BufferUtilsShaderParams));
}

angle::Result DispatchUtilsVk::ensureConvertVertexInitialized(vk::Context *context)
{
    if (mPipelineLayouts[Function::ConvertVertexBuffer].valid())
    {
        return angle::Result::Continue;
    }

    VkDescriptorPoolSize setSizes[2] = {
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    };

    return ensureResourcesInitialized(context, Function::ConvertVertexBuffer, setSizes,
                                      ArraySize(setSizes), sizeof(ConvertVertexShaderParams));
}

angle::Result DispatchUtilsVk::setupProgramCommon(vk::Context *context,
                                                  Function function,
                                                  vk::RefCounted<vk::ShaderAndSerial> *shader,
                                                  vk::ShaderProgramHelper *program,
                                                  const VkDescriptorSet descriptorSet,
                                                  const void *pushConstants,
                                                  size_t pushConstantsSize,
                                                  vk::CommandBuffer *commandBuffer)
{
    RendererVk *renderer = context->getRenderer();

    program->setShader(gl::ShaderType::Compute, shader);

    const vk::BindingPointer<vk::PipelineLayout> &pipelineLayout = mPipelineLayouts[function];

    vk::PipelineAndSerial *pipelineAndSerial;
    ANGLE_TRY(program->getComputePipeline(context, pipelineLayout.get(), &pipelineAndSerial));

    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, pipelineAndSerial->get());
    pipelineAndSerial->updateSerial(renderer->getCurrentQueueSerial());

    commandBuffer->bindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout.get(), 0, 1,
                                      &descriptorSet, 0, nullptr);

    commandBuffer->pushConstants(pipelineLayout.get(), VK_SHADER_STAGE_COMPUTE_BIT, 0,
                                 pushConstantsSize, pushConstants);

    return angle::Result::Continue;
}

template <angle::Result (vk::ShaderLibrary::*getShader)(vk::Context *,
                                                        uint32_t,
                                                        vk::RefCounted<vk::ShaderAndSerial> **),
          DispatchUtilsVk::Function function,
          typename ShaderParams>
angle::Result DispatchUtilsVk::setupProgram(vk::Context *context,
                                            vk::ShaderProgramHelper *program,
                                            uint32_t flags,
                                            const VkDescriptorSet &descriptorSet,
                                            const ShaderParams &params,
                                            vk::CommandBuffer *commandBuffer)
{
    RendererVk *renderer             = context->getRenderer();
    vk::ShaderLibrary &shaderLibrary = renderer->getShaderLibrary();

    vk::RefCounted<vk::ShaderAndSerial> *shader = nullptr;
    ANGLE_TRY((shaderLibrary.*getShader)(context, flags, &shader));

    ANGLE_TRY(setupProgramCommon(context, function, shader, program, descriptorSet, &params,
                                 sizeof(params), commandBuffer));

    return angle::Result::Continue;
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

    uint32_t flags = BufferUtils_comp::kIsClear | GetBufferUtilsFlags(params.size, destFormat);

    BufferUtilsShaderParams shaderParams;
    shaderParams.destOffset = params.offset;
    shaderParams.size       = params.size;
    shaderParams.clearValue = params.clearValue;

    VkDescriptorSet descriptorSet;
    vk::SharedDescriptorPoolBinding descriptorPoolBinding;
    ANGLE_TRY(mDescriptorPools[Function::BufferClear].allocateSets(
        context, mDescriptorSetLayouts[Function::BufferClear][kSetIndex].get().ptr(), 1,
        &descriptorPoolBinding, &descriptorSet));
    descriptorPoolBinding.get().updateSerial(context->getRenderer()->getCurrentQueueSerial());

    VkWriteDescriptorSet writeInfo = {};

    writeInfo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet           = descriptorSet;
    writeInfo.dstBinding       = kBufferClearOutputBinding;
    writeInfo.descriptorCount  = 1;
    writeInfo.descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    writeInfo.pTexelBufferView = dest->getBufferView().ptr();

    vkUpdateDescriptorSets(context->getDevice(), 1, &writeInfo, 0, nullptr);

    ANGLE_TRY((setupProgram<&vk::ShaderLibrary::getBufferUtils_comp, Function::BufferClear,
                            BufferUtilsShaderParams>(context, &mBufferUtilsPrograms[flags], flags,
                                                     descriptorSet, shaderParams, commandBuffer)));

    commandBuffer->dispatch(UnsignedCeilDivide(params.size, 64), 1, 1);

    descriptorPoolBinding.reset();

    return angle::Result::Continue;
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

    uint32_t flags = BufferUtils_comp::kIsCopy | GetBufferUtilsFlags(params.size, destFormat);

    BufferUtilsShaderParams shaderParams;
    shaderParams.destOffset = params.destOffset;
    shaderParams.size       = params.size;
    shaderParams.srcOffset  = params.srcOffset;

    VkDescriptorSet descriptorSet;
    vk::SharedDescriptorPoolBinding descriptorPoolBinding;
    ANGLE_TRY(mDescriptorPools[Function::BufferCopy].allocateSets(
        context, mDescriptorSetLayouts[Function::BufferCopy][kSetIndex].get().ptr(), 1,
        &descriptorPoolBinding, &descriptorSet));
    descriptorPoolBinding.get().updateSerial(context->getRenderer()->getCurrentQueueSerial());

    VkWriteDescriptorSet writeInfo[2] = {};

    writeInfo[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[0].dstSet           = descriptorSet;
    writeInfo[0].dstBinding       = kBufferCopyDestinationBinding;
    writeInfo[0].descriptorCount  = 1;
    writeInfo[0].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    writeInfo[0].pTexelBufferView = dest->getBufferView().ptr();

    writeInfo[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo[1].dstSet           = descriptorSet;
    writeInfo[1].dstBinding       = kBufferCopySourceBinding;
    writeInfo[1].descriptorCount  = 1;
    writeInfo[1].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    writeInfo[1].pTexelBufferView = src->getBufferView().ptr();

    vkUpdateDescriptorSets(context->getDevice(), 2, writeInfo, 0, nullptr);

    ANGLE_TRY((setupProgram<&vk::ShaderLibrary::getBufferUtils_comp, Function::BufferCopy,
                            BufferUtilsShaderParams>(context, &mBufferUtilsPrograms[flags], flags,
                                                     descriptorSet, shaderParams, commandBuffer)));

    commandBuffer->dispatch(UnsignedCeilDivide(params.size, 64), 1, 1);

    descriptorPoolBinding.reset();

    return angle::Result::Continue;
}

angle::Result DispatchUtilsVk::convertVertexBuffer(vk::Context *context,
                                                   vk::BufferHelper *dest,
                                                   vk::BufferHelper *src,
                                                   const ConvertVertexParameters &params)
{
    ANGLE_TRY(ensureConvertVertexInitialized(context));

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(dest->recordCommands(context, &commandBuffer));

    // Tell src we are going to read from it.
    src->onRead(dest, VK_ACCESS_SHADER_READ_BIT);
    // Tell dest it's being written to.
    dest->onWrite(VK_ACCESS_SHADER_WRITE_BIT);

    ConvertVertexShaderParams shaderParams;
    shaderParams.Ns = params.srcFormat->channelCount();
    shaderParams.Bs = params.srcFormat->pixelBytes / params.srcFormat->channelCount();
    shaderParams.Ss = params.srcStride;
    shaderParams.Nd = params.destFormat->channelCount();
    shaderParams.Bd = params.destFormat->pixelBytes / params.destFormat->channelCount();
    shaderParams.Sd = shaderParams.Nd * shaderParams.Bd;
    // The component size is expected to either be 1, 2 or 4 bytes.
    ASSERT(4 % shaderParams.Bs == 0);
    ASSERT(4 % shaderParams.Bd == 0);
    shaderParams.Es = 4 / shaderParams.Bs;
    shaderParams.Ed = 4 / shaderParams.Bd;
    // Total number of output components is simply the number of vertices by number of components in
    // each.
    shaderParams.componentCount = params.vertexCount * shaderParams.Nd;
    // Total number of 4-byte outputs is the number of components divided by how many components can
    // fit in a 4-byte value.  Note that this value is also the invocation size of the shader.
    shaderParams.outputCount = shaderParams.componentCount / shaderParams.Ed;
    shaderParams.srcOffset   = params.srcOffset;
    shaderParams.destOffset  = params.destOffset;

    uint32_t flags = GetConvertVertexFlags(params);

    bool isAligned =
        shaderParams.outputCount % 64 == 0 && shaderParams.componentCount % shaderParams.Ed == 0;
    flags |= isAligned ? ConvertVertex_comp::kIsAligned : 0;

    VkDescriptorSet descriptorSet;
    vk::SharedDescriptorPoolBinding descriptorPoolBinding;
    ANGLE_TRY(mDescriptorPools[Function::ConvertVertexBuffer].allocateSets(
        context, mDescriptorSetLayouts[Function::ConvertVertexBuffer][kSetIndex].get().ptr(), 1,
        &descriptorPoolBinding, &descriptorSet));
    descriptorPoolBinding.get().updateSerial(context->getRenderer()->getCurrentQueueSerial());

    VkWriteDescriptorSet writeInfo    = {};
    VkDescriptorBufferInfo buffers[2] = {
        {dest->getBuffer().getHandle(), 0, VK_WHOLE_SIZE},
        {src->getBuffer().getHandle(), 0, VK_WHOLE_SIZE},
    };
    static_assert(kConvertVertexDestinationBinding + 1 == kConvertVertexSourceBinding,
                  "Update write info");

    writeInfo.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet          = descriptorSet;
    writeInfo.dstBinding      = kConvertVertexDestinationBinding;
    writeInfo.descriptorCount = 2;
    writeInfo.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeInfo.pBufferInfo     = buffers;

    vkUpdateDescriptorSets(context->getDevice(), 1, &writeInfo, 0, nullptr);

    ANGLE_TRY(
        (setupProgram<&vk::ShaderLibrary::getConvertVertex_comp, Function::ConvertVertexBuffer,
                      ConvertVertexShaderParams>(context, &mConvertVertexPrograms[flags], flags,
                                                 descriptorSet, shaderParams, commandBuffer)));

    commandBuffer->dispatch(UnsignedCeilDivide(shaderParams.outputCount, 64), 1, 1);

    descriptorPoolBinding.reset();

    return angle::Result::Continue;
}

}  // namespace rx
