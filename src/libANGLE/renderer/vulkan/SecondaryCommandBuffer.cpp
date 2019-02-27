//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SecondaryCommandBuffer:
//    CPU-side storage of commands to delay GPU-side allocation until commands are submitted.
//

#include "libANGLE/renderer/vulkan/SecondaryCommandBuffer.h"
#include "common/debug.h"

namespace rx
{
namespace vk
{

// Allocate/initialize memory for the command and return pointer to Cmd Header
template <class StructType>
StructType *SecondaryCommandBuffer::initCommand(CommandID cmdID, size_t variableSize)
{
    size_t paramSize      = sizeof(StructType);
    size_t completeSize   = sizeof(CommandHeader) + paramSize + variableSize;
    CommandHeader *header = static_cast<CommandHeader *>(mAllocator->allocate(completeSize));
    // Update cmd ID in header
    header->id   = cmdID;
    header->next = nullptr;
    // Update mHead ptr
    mHead = (mHead == nullptr) ? header : mHead;
    // Update prev cmd's "next" ptr and mLast ptr
    if (mLast)
    {
        mLast->next = header;
    }
    // Update mLast ptr
    mLast = header;

    uint8_t *fixedParamPtr = reinterpret_cast<uint8_t *>(header) + sizeof(CommandHeader);
    mPtrCmdData            = fixedParamPtr + sizeof(StructType);
    return reinterpret_cast<StructType *>(fixedParamPtr);
}

template <class PtrType>
void SecondaryCommandBuffer::storePointerParameter(const PtrType *paramData,
                                                   const PtrType **writePtr,
                                                   size_t sizeInBytes)
{
    *writePtr = reinterpret_cast<const PtrType *>(mPtrCmdData);
    memcpy(mPtrCmdData, paramData, sizeInBytes);
    mPtrCmdData += sizeInBytes;
}

void SecondaryCommandBuffer::bindDescriptorSets(VkPipelineBindPoint bindPoint,
                                                VkPipelineLayout layout,
                                                uint32_t firstSet,
                                                uint32_t descriptorSetCount,
                                                const VkDescriptorSet *descriptorSets,
                                                uint32_t dynamicOffsetCount,
                                                const uint32_t *dynamicOffsets)
{
    size_t descSize   = descriptorSetCount * sizeof(VkDescriptorSet);
    size_t offsetSize = dynamicOffsetCount * sizeof(uint32_t);
    size_t varSize    = descSize + offsetSize;
    BindDescriptorSetParams *paramStruct =
        initCommand<BindDescriptorSetParams>(CommandID::BindDescriptorSets, varSize);
    // Copy params into memory
    paramStruct->bindPoint          = bindPoint;
    paramStruct->layout             = layout;
    paramStruct->firstSet           = firstSet;
    paramStruct->descriptorSetCount = descriptorSetCount;
    paramStruct->dynamicOffsetCount = dynamicOffsetCount;
    // Copy variable sized data
    storePointerParameter(descriptorSets, &paramStruct->descriptorSets, descSize);
    storePointerParameter(dynamicOffsets, &paramStruct->dynamicOffsets, offsetSize);
}

void SecondaryCommandBuffer::bindIndexBuffer(const VkBuffer &buffer,
                                             VkDeviceSize offset,
                                             VkIndexType indexType)
{
    BindIndexBufferParams *paramStruct =
        initCommand<BindIndexBufferParams>(CommandID::BindIndexBuffer, 0);
    paramStruct->buffer    = buffer;
    paramStruct->offset    = offset;
    paramStruct->indexType = indexType;
}

void SecondaryCommandBuffer::bindPipeline(VkPipelineBindPoint pipelineBindPoint,
                                          VkPipeline pipeline)
{
    BindPipelineParams *paramStruct = initCommand<BindPipelineParams>(CommandID::BindPipeline, 0);
    paramStruct->pipelineBindPoint  = pipelineBindPoint;
    paramStruct->pipeline           = pipeline;
}

void SecondaryCommandBuffer::bindVertexBuffers(uint32_t firstBinding,
                                               uint32_t bindingCount,
                                               const VkBuffer *buffers,
                                               const VkDeviceSize *offsets)
{
    size_t buffSize   = bindingCount * sizeof(VkBuffer);
    size_t offsetSize = bindingCount * sizeof(VkDeviceSize);
    BindVertexBuffersParams *paramStruct =
        initCommand<BindVertexBuffersParams>(CommandID::BindVertexBuffers, buffSize + offsetSize);
    // Copy params
    paramStruct->firstBinding = firstBinding;
    paramStruct->bindingCount = bindingCount;
    // Copy variable sized data
    storePointerParameter(buffers, &paramStruct->buffers, buffSize);
    storePointerParameter(offsets, &paramStruct->offsets, offsetSize);
}

void SecondaryCommandBuffer::blitImage(VkImage srcImage,
                                       VkImageLayout srcImageLayout,
                                       VkImage dstImage,
                                       VkImageLayout dstImageLayout,
                                       uint32_t regionCount,
                                       const VkImageBlit *pRegions,
                                       VkFilter filter)
{
    size_t regionSize            = regionCount * sizeof(VkImageBlit);
    BlitImageParams *paramStruct = initCommand<BlitImageParams>(CommandID::BlitImage, regionSize);
    paramStruct->srcImage        = srcImage;
    paramStruct->srcImageLayout  = srcImageLayout;
    paramStruct->dstImage        = dstImage;
    paramStruct->dstImageLayout  = dstImageLayout;
    paramStruct->regionCount     = regionCount;
    paramStruct->filter          = filter;
    // Copy variable sized data
    storePointerParameter(pRegions, &paramStruct->pRegions, regionSize);
}

void SecondaryCommandBuffer::copyBuffer(const VkBuffer &srcBuffer,
                                        const VkBuffer &destBuffer,
                                        uint32_t regionCount,
                                        const VkBufferCopy *regions)
{
    size_t regionSize = regionCount * sizeof(VkBufferCopy);
    CopyBufferParams *paramStruct =
        initCommand<CopyBufferParams>(CommandID::CopyBuffer, regionSize);
    paramStruct->srcBuffer   = srcBuffer;
    paramStruct->destBuffer  = destBuffer;
    paramStruct->regionCount = regionCount;
    // Copy variable sized data
    storePointerParameter(regions, &paramStruct->regions, regionSize);
}

void SecondaryCommandBuffer::copyBufferToImage(VkBuffer srcBuffer,
                                               VkImage dstImage,
                                               VkImageLayout dstImageLayout,
                                               uint32_t regionCount,
                                               const VkBufferImageCopy *regions)
{
    size_t regionSize = regionCount * sizeof(VkBufferImageCopy);
    CopyBufferToImageParams *paramStruct =
        initCommand<CopyBufferToImageParams>(CommandID::CopyBufferToImage, regionSize);
    paramStruct->srcBuffer      = srcBuffer;
    paramStruct->dstImage       = dstImage;
    paramStruct->dstImageLayout = dstImageLayout;
    paramStruct->regionCount    = regionCount;
    // Copy variable sized data
    storePointerParameter(regions, &paramStruct->regions, regionSize);
}

void SecondaryCommandBuffer::copyImage(VkImage srcImage,
                                       VkImageLayout srcImageLayout,
                                       VkImage dstImage,
                                       VkImageLayout dstImageLayout,
                                       uint32_t regionCount,
                                       const VkImageCopy *regions)
{
    size_t regionSize            = regionCount * sizeof(VkImageCopy);
    CopyImageParams *paramStruct = initCommand<CopyImageParams>(CommandID::CopyImage, regionSize);
    paramStruct->srcImage        = srcImage;
    paramStruct->srcImageLayout  = srcImageLayout;
    paramStruct->dstImage        = dstImage;
    paramStruct->dstImageLayout  = dstImageLayout;
    paramStruct->regionCount     = regionCount;
    // Copy variable sized data
    storePointerParameter(regions, &paramStruct->regions, regionSize);
}

void SecondaryCommandBuffer::copyImageToBuffer(VkImage srcImage,
                                               VkImageLayout srcImageLayout,
                                               VkBuffer dstBuffer,
                                               uint32_t regionCount,
                                               const VkBufferImageCopy *regions)
{
    size_t regionSize = regionCount * sizeof(VkBufferImageCopy);
    CopyImageToBufferParams *paramStruct =
        initCommand<CopyImageToBufferParams>(CommandID::CopyImageToBuffer, regionSize);
    paramStruct->srcImage       = srcImage;
    paramStruct->srcImageLayout = srcImageLayout;
    paramStruct->dstBuffer      = dstBuffer;
    paramStruct->regionCount    = regionCount;
    // Copy variable sized data
    storePointerParameter(regions, &paramStruct->regions, regionSize);
}

void SecondaryCommandBuffer::clearAttachments(uint32_t attachmentCount,
                                              const VkClearAttachment *attachments,
                                              uint32_t rectCount,
                                              const VkClearRect *rects)
{
    size_t attachSize = attachmentCount * sizeof(VkClearAttachment);
    size_t rectSize   = rectCount * sizeof(VkClearRect);
    ClearAttachmentsParams *paramStruct =
        initCommand<ClearAttachmentsParams>(CommandID::ClearAttachments, attachSize + rectSize);
    paramStruct->attachmentCount = attachmentCount;
    paramStruct->rectCount       = rectCount;
    // Copy variable sized data
    storePointerParameter(attachments, &paramStruct->attachments, attachSize);
    storePointerParameter(rects, &paramStruct->rects, rectSize);
}

void SecondaryCommandBuffer::clearColorImage(VkImage image,
                                             VkImageLayout imageLayout,
                                             const VkClearColorValue &color,
                                             uint32_t rangeCount,
                                             const VkImageSubresourceRange *ranges)
{
    size_t rangeSize = rangeCount * sizeof(VkImageSubresourceRange);
    ClearColorImageParams *paramStruct =
        initCommand<ClearColorImageParams>(CommandID::ClearColorImage, rangeSize);
    paramStruct->image       = image;
    paramStruct->imageLayout = imageLayout;
    paramStruct->color       = color;
    paramStruct->rangeCount  = rangeCount;
    // Copy variable sized data
    storePointerParameter(ranges, &paramStruct->ranges, rangeSize);
}

void SecondaryCommandBuffer::clearDepthStencilImage(VkImage image,
                                                    VkImageLayout imageLayout,
                                                    const VkClearDepthStencilValue &depthStencil,
                                                    uint32_t rangeCount,
                                                    const VkImageSubresourceRange *ranges)
{
    size_t rangeSize = rangeCount * sizeof(VkImageSubresourceRange);
    ClearDepthStencilImageParams *paramStruct =
        initCommand<ClearDepthStencilImageParams>(CommandID::ClearDepthStencilImage, rangeSize);
    paramStruct->image        = image;
    paramStruct->imageLayout  = imageLayout;
    paramStruct->depthStencil = depthStencil;
    paramStruct->rangeCount   = rangeCount;
    // Copy variable sized data
    storePointerParameter(ranges, &paramStruct->ranges, rangeSize);
}

void SecondaryCommandBuffer::updateBuffer(VkBuffer buffer,
                                          VkDeviceSize dstOffset,
                                          VkDeviceSize dataSize,
                                          const void *data)
{
    ASSERT(dataSize == static_cast<size_t>(dataSize));
    UpdateBufferParams *paramStruct =
        initCommand<UpdateBufferParams>(CommandID::UpdateBuffer, static_cast<size_t>(dataSize));
    paramStruct->buffer    = buffer;
    paramStruct->dstOffset = dstOffset;
    paramStruct->dataSize  = dataSize;
    // Copy variable sized data
    storePointerParameter(data, &paramStruct->data, static_cast<size_t>(dataSize));
}

void SecondaryCommandBuffer::pushConstants(VkPipelineLayout layout,
                                           VkShaderStageFlags flag,
                                           uint32_t offset,
                                           uint32_t size,
                                           const void *data)
{
    ASSERT(size == static_cast<size_t>(size));
    PushConstantsParams *paramStruct =
        initCommand<PushConstantsParams>(CommandID::PushConstants, static_cast<size_t>(size));
    paramStruct->layout = layout;
    paramStruct->flag   = flag;
    paramStruct->offset = offset;
    paramStruct->size   = size;
    // Copy variable sized data
    storePointerParameter(data, &paramStruct->data, static_cast<size_t>(size));
}

void SecondaryCommandBuffer::setViewport(uint32_t firstViewport,
                                         uint32_t viewportCount,
                                         const VkViewport *viewports)
{
    size_t viewportSize = viewportCount * sizeof(VkViewport);
    SetViewportParams *paramStruct =
        initCommand<SetViewportParams>(CommandID::SetViewport, viewportSize);
    paramStruct->firstViewport = firstViewport;
    paramStruct->viewportCount = viewportCount;
    // Copy variable sized data
    storePointerParameter(viewports, &paramStruct->viewports, viewportSize);
}

void SecondaryCommandBuffer::setScissor(uint32_t firstScissor,
                                        uint32_t scissorCount,
                                        const VkRect2D *scissors)
{
    size_t scissorSize = scissorCount * sizeof(VkRect2D);
    SetScissorParams *paramStruct =
        initCommand<SetScissorParams>(CommandID::SetScissor, scissorSize);
    paramStruct->firstScissor = firstScissor;
    paramStruct->scissorCount = scissorCount;
    // Copy variable sized data
    storePointerParameter(scissors, &paramStruct->scissors, scissorSize);
}

void SecondaryCommandBuffer::draw(uint32_t vertexCount,
                                  uint32_t instanceCount,
                                  uint32_t firstVertex,
                                  uint32_t firstInstance)
{
    DrawParams *paramStruct    = initCommand<DrawParams>(CommandID::Draw, 0);
    paramStruct->vertexCount   = vertexCount;
    paramStruct->instanceCount = instanceCount;
    paramStruct->firstVertex   = firstVertex;
    paramStruct->firstInstance = firstInstance;
}

void SecondaryCommandBuffer::drawIndexed(uint32_t indexCount,
                                         uint32_t instanceCount,
                                         uint32_t firstIndex,
                                         int32_t vertexOffset,
                                         uint32_t firstInstance)
{
    DrawIndexedParams *paramStruct = initCommand<DrawIndexedParams>(CommandID::DrawIndexed, 0);
    paramStruct->indexCount        = indexCount;
    paramStruct->instanceCount     = instanceCount;
    paramStruct->firstIndex        = firstIndex;
    paramStruct->vertexOffset      = vertexOffset;
    paramStruct->firstInstance     = firstInstance;
}

void SecondaryCommandBuffer::dispatch(uint32_t groupCountX,
                                      uint32_t groupCountY,
                                      uint32_t groupCountZ)
{
    DispatchParams *paramStruct = initCommand<DispatchParams>(CommandID::Dispatch, 0);
    paramStruct->groupCountX    = groupCountX;
    paramStruct->groupCountY    = groupCountY;
    paramStruct->groupCountZ    = groupCountZ;
}

void SecondaryCommandBuffer::pipelineBarrier(VkPipelineStageFlags srcStageMask,
                                             VkPipelineStageFlags dstStageMask,
                                             VkDependencyFlags dependencyFlags,
                                             uint32_t memoryBarrierCount,
                                             const VkMemoryBarrier *memoryBarriers,
                                             uint32_t bufferMemoryBarrierCount,
                                             const VkBufferMemoryBarrier *bufferMemoryBarriers,
                                             uint32_t imageMemoryBarrierCount,
                                             const VkImageMemoryBarrier *imageMemoryBarriers)
{
    size_t memBarrierSize              = memoryBarrierCount * sizeof(VkMemoryBarrier);
    size_t buffBarrierSize             = bufferMemoryBarrierCount * sizeof(VkBufferMemoryBarrier);
    size_t imgBarrierSize              = imageMemoryBarrierCount * sizeof(VkImageMemoryBarrier);
    PipelineBarrierParams *paramStruct = initCommand<PipelineBarrierParams>(
        CommandID::PipelinBarrier, memBarrierSize + buffBarrierSize + imgBarrierSize);
    paramStruct->srcStageMask             = srcStageMask;
    paramStruct->dstStageMask             = dstStageMask;
    paramStruct->memoryBarrierCount       = memoryBarrierCount;
    paramStruct->bufferMemoryBarrierCount = bufferMemoryBarrierCount;
    paramStruct->imageMemoryBarrierCount  = imageMemoryBarrierCount;
    // Copy variable sized data
    storePointerParameter(memoryBarriers, &paramStruct->memoryBarriers, memBarrierSize);
    storePointerParameter(bufferMemoryBarriers, &paramStruct->bufferMemoryBarriers,
                          buffBarrierSize);
    storePointerParameter(imageMemoryBarriers, &paramStruct->imageMemoryBarriers, imgBarrierSize);
}

void SecondaryCommandBuffer::setEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    SetEventParams *paramStruct = initCommand<SetEventParams>(CommandID::SetEvent, 0);
    paramStruct->event          = event;
    paramStruct->stageMask      = stageMask;
}

void SecondaryCommandBuffer::resetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    ResetEventParams *paramStruct = initCommand<ResetEventParams>(CommandID::ResetEvent, 0);
    paramStruct->event            = event;
    paramStruct->stageMask        = stageMask;
}

void SecondaryCommandBuffer::waitEvents(uint32_t eventCount,
                                        const VkEvent *events,
                                        VkPipelineStageFlags srcStageMask,
                                        VkPipelineStageFlags dstStageMask,
                                        uint32_t memoryBarrierCount,
                                        const VkMemoryBarrier *memoryBarriers,
                                        uint32_t bufferMemoryBarrierCount,
                                        const VkBufferMemoryBarrier *bufferMemoryBarriers,
                                        uint32_t imageMemoryBarrierCount,
                                        const VkImageMemoryBarrier *imageMemoryBarriers)
{
    size_t eventSize              = eventCount * sizeof(VkEvent);
    size_t memBarrierSize         = memoryBarrierCount * sizeof(VkMemoryBarrier);
    size_t buffBarrierSize        = bufferMemoryBarrierCount * sizeof(VkBufferMemoryBarrier);
    size_t imgBarrierSize         = imageMemoryBarrierCount * sizeof(VkImageMemoryBarrier);
    WaitEventsParams *paramStruct = initCommand<WaitEventsParams>(
        CommandID::WaitEvents, eventSize + memBarrierSize + buffBarrierSize + imgBarrierSize);
    paramStruct->eventCount               = eventCount;
    paramStruct->srcStageMask             = srcStageMask;
    paramStruct->dstStageMask             = dstStageMask;
    paramStruct->memoryBarrierCount       = memoryBarrierCount;
    paramStruct->bufferMemoryBarrierCount = bufferMemoryBarrierCount;
    paramStruct->imageMemoryBarrierCount  = imageMemoryBarrierCount;
    // Copy variable sized data
    storePointerParameter(events, &paramStruct->events, eventSize);
    storePointerParameter(memoryBarriers, &paramStruct->memoryBarriers, memBarrierSize);
    storePointerParameter(bufferMemoryBarriers, &paramStruct->bufferMemoryBarriers,
                          buffBarrierSize);
    storePointerParameter(imageMemoryBarriers, &paramStruct->imageMemoryBarriers, imgBarrierSize);
}

void SecondaryCommandBuffer::resetQueryPool(VkQueryPool queryPool,
                                            uint32_t firstQuery,
                                            uint32_t queryCount)
{
    ResetQueryPoolParams *paramStruct =
        initCommand<ResetQueryPoolParams>(CommandID::ResetQueryPool, 0);
    paramStruct->queryPool  = queryPool;
    paramStruct->firstQuery = firstQuery;
    paramStruct->queryCount = queryCount;
}

void SecondaryCommandBuffer::beginQuery(VkQueryPool queryPool,
                                        uint32_t query,
                                        VkQueryControlFlags flags)
{
    BeginQueryParams *paramStruct = initCommand<BeginQueryParams>(CommandID::BeginQuery, 0);
    paramStruct->queryPool        = queryPool;
    paramStruct->query            = query;
    paramStruct->flags            = flags;
}

void SecondaryCommandBuffer::endQuery(VkQueryPool queryPool, uint32_t query)
{
    EndQueryParams *paramStruct = initCommand<EndQueryParams>(CommandID::EndQuery, 0);
    paramStruct->queryPool      = queryPool;
    paramStruct->query          = query;
}

void SecondaryCommandBuffer::writeTimestamp(VkPipelineStageFlagBits pipelineStage,
                                            VkQueryPool queryPool,
                                            uint32_t query)
{
    WriteTimestampParams *paramStruct =
        initCommand<WriteTimestampParams>(CommandID::WriteTimestamp, 0);
    paramStruct->pipelineStage = pipelineStage;
    paramStruct->queryPool     = queryPool;
    paramStruct->query         = query;
}

// Parse the cmds in this cmd buffer into given primary cmd buffer
void SecondaryCommandBuffer::executeCommands(VkCommandBuffer cmdBuffer)
{
    for (CommandHeader *currentCommand = mHead; currentCommand;
         currentCommand                = currentCommand->next)
    {
        switch (currentCommand->id)
        {
            case CommandID::BindDescriptorSets:
            {
                BindDescriptorSetParams *params =
                    getParamPtr<BindDescriptorSetParams>(currentCommand);
                vkCmdBindDescriptorSets(cmdBuffer, params->bindPoint, params->layout,
                                        params->firstSet, params->descriptorSetCount,
                                        params->descriptorSets, params->dynamicOffsetCount,
                                        params->dynamicOffsets);
                break;
            }
            case CommandID::BindIndexBuffer:
            {
                BindIndexBufferParams *params = getParamPtr<BindIndexBufferParams>(currentCommand);
                vkCmdBindIndexBuffer(cmdBuffer, params->buffer, params->offset, params->indexType);
                break;
            }
            case CommandID::BindPipeline:
            {
                BindPipelineParams *params = getParamPtr<BindPipelineParams>(currentCommand);
                vkCmdBindPipeline(cmdBuffer, params->pipelineBindPoint, params->pipeline);
                break;
            }
            case CommandID::BindVertexBuffers:
            {
                BindVertexBuffersParams *params =
                    getParamPtr<BindVertexBuffersParams>(currentCommand);
                vkCmdBindVertexBuffers(cmdBuffer, params->firstBinding, params->bindingCount,
                                       params->buffers, params->offsets);
                break;
            }
            case CommandID::BlitImage:
            {
                BlitImageParams *params = getParamPtr<BlitImageParams>(currentCommand);
                vkCmdBlitImage(cmdBuffer, params->srcImage, params->srcImageLayout,
                               params->dstImage, params->dstImageLayout, params->regionCount,
                               params->pRegions, params->filter);
                break;
            }
            case CommandID::CopyBuffer:
            {
                CopyBufferParams *params = getParamPtr<CopyBufferParams>(currentCommand);
                vkCmdCopyBuffer(cmdBuffer, params->srcBuffer, params->destBuffer,
                                params->regionCount, params->regions);
                break;
            }
            case CommandID::CopyBufferToImage:
            {
                CopyBufferToImageParams *params =
                    getParamPtr<CopyBufferToImageParams>(currentCommand);
                vkCmdCopyBufferToImage(cmdBuffer, params->srcBuffer, params->dstImage,
                                       params->dstImageLayout, params->regionCount,
                                       params->regions);
                break;
            }
            case CommandID::CopyImage:
            {
                CopyImageParams *params = getParamPtr<CopyImageParams>(currentCommand);
                vkCmdCopyImage(cmdBuffer, params->srcImage, params->srcImageLayout,
                               params->dstImage, params->dstImageLayout, params->regionCount,
                               params->regions);
                break;
            }
            case CommandID::CopyImageToBuffer:
            {
                CopyImageToBufferParams *params =
                    getParamPtr<CopyImageToBufferParams>(currentCommand);
                vkCmdCopyImageToBuffer(cmdBuffer, params->srcImage, params->srcImageLayout,
                                       params->dstBuffer, params->regionCount, params->regions);
                break;
            }
            case CommandID::ClearAttachments:
            {
                ClearAttachmentsParams *params =
                    getParamPtr<ClearAttachmentsParams>(currentCommand);
                vkCmdClearAttachments(cmdBuffer, params->attachmentCount, params->attachments,
                                      params->rectCount, params->rects);
                break;
            }
            case CommandID::ClearColorImage:
            {
                ClearColorImageParams *params = getParamPtr<ClearColorImageParams>(currentCommand);
                vkCmdClearColorImage(cmdBuffer, params->image, params->imageLayout, &params->color,
                                     params->rangeCount, params->ranges);
                break;
            }
            case CommandID::ClearDepthStencilImage:
            {
                ClearDepthStencilImageParams *params =
                    getParamPtr<ClearDepthStencilImageParams>(currentCommand);
                vkCmdClearDepthStencilImage(cmdBuffer, params->image, params->imageLayout,
                                            &params->depthStencil, params->rangeCount,
                                            params->ranges);
                break;
            }
            case CommandID::UpdateBuffer:
            {
                UpdateBufferParams *params = getParamPtr<UpdateBufferParams>(currentCommand);
                vkCmdUpdateBuffer(cmdBuffer, params->buffer, params->dstOffset, params->dataSize,
                                  params->data);
                break;
            }
            case CommandID::PushConstants:
            {
                PushConstantsParams *params = getParamPtr<PushConstantsParams>(currentCommand);
                vkCmdPushConstants(cmdBuffer, params->layout, params->flag, params->offset,
                                   params->size, params->data);
                break;
            }
            case CommandID::SetViewport:
            {
                SetViewportParams *params = getParamPtr<SetViewportParams>(currentCommand);
                vkCmdSetViewport(cmdBuffer, params->firstViewport, params->viewportCount,
                                 params->viewports);
                break;
            }
            case CommandID::SetScissor:
            {
                SetScissorParams *params = getParamPtr<SetScissorParams>(currentCommand);
                vkCmdSetScissor(cmdBuffer, params->firstScissor, params->scissorCount,
                                params->scissors);
                break;
            }
            case CommandID::Draw:
            {
                DrawParams *params = getParamPtr<DrawParams>(currentCommand);
                vkCmdDraw(cmdBuffer, params->vertexCount, params->instanceCount,
                          params->firstVertex, params->firstInstance);
                break;
            }
            case CommandID::DrawIndexed:
            {
                DrawIndexedParams *params = getParamPtr<DrawIndexedParams>(currentCommand);
                vkCmdDrawIndexed(cmdBuffer, params->indexCount, params->instanceCount,
                                 params->firstIndex, params->vertexOffset, params->firstInstance);
                break;
            }
            case CommandID::Dispatch:
            {
                DispatchParams *params = getParamPtr<DispatchParams>(currentCommand);
                vkCmdDispatch(cmdBuffer, params->groupCountX, params->groupCountY,
                              params->groupCountZ);
                break;
            }
            case CommandID::PipelinBarrier:
            {
                PipelineBarrierParams *params = getParamPtr<PipelineBarrierParams>(currentCommand);
                vkCmdPipelineBarrier(cmdBuffer, params->srcStageMask, params->dstStageMask,
                                     params->dependencyFlags, params->memoryBarrierCount,
                                     params->memoryBarriers, params->bufferMemoryBarrierCount,
                                     params->bufferMemoryBarriers, params->imageMemoryBarrierCount,
                                     params->imageMemoryBarriers);
                break;
            }
            case CommandID::SetEvent:
            {
                SetEventParams *params = getParamPtr<SetEventParams>(currentCommand);
                vkCmdSetEvent(cmdBuffer, params->event, params->stageMask);
                break;
            }
            case CommandID::ResetEvent:
            {
                ResetEventParams *params = getParamPtr<ResetEventParams>(currentCommand);
                vkCmdResetEvent(cmdBuffer, params->event, params->stageMask);
                break;
            }
            case CommandID::WaitEvents:
            {
                WaitEventsParams *params = getParamPtr<WaitEventsParams>(currentCommand);
                vkCmdWaitEvents(cmdBuffer, params->eventCount, params->events, params->srcStageMask,
                                params->dstStageMask, params->memoryBarrierCount,
                                params->memoryBarriers, params->bufferMemoryBarrierCount,
                                params->bufferMemoryBarriers, params->imageMemoryBarrierCount,
                                params->imageMemoryBarriers);
                break;
            }
            case CommandID::ResetQueryPool:
            {
                ResetQueryPoolParams *params = getParamPtr<ResetQueryPoolParams>(currentCommand);
                vkCmdResetQueryPool(cmdBuffer, params->queryPool, params->firstQuery,
                                    params->queryCount);
                break;
            }
            case CommandID::BeginQuery:
            {
                BeginQueryParams *params = getParamPtr<BeginQueryParams>(currentCommand);
                vkCmdBeginQuery(cmdBuffer, params->queryPool, params->query, params->flags);
                break;
            }
            case CommandID::EndQuery:
            {
                EndQueryParams *params = getParamPtr<EndQueryParams>(currentCommand);
                vkCmdEndQuery(cmdBuffer, params->queryPool, params->query);
                break;
            }
            case CommandID::WriteTimestamp:
            {
                WriteTimestampParams *params = getParamPtr<WriteTimestampParams>(currentCommand);
                vkCmdWriteTimestamp(cmdBuffer, params->pipelineStage, params->queryPool,
                                    params->query);
                break;
            }
            default:
            {
                UNREACHABLE();
                break;
            }
        }
    }
}

}  // namespace vk
}  // namespace rx
