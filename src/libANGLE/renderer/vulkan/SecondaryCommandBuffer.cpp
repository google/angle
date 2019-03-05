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
void SecondaryCommandBuffer::blitImage(const Image &srcImage,
                                       VkImageLayout srcImageLayout,
                                       const Image &dstImage,
                                       VkImageLayout dstImageLayout,
                                       uint32_t regionCount,
                                       const VkImageBlit *pRegions,
                                       VkFilter filter)
{
    size_t regionSize            = regionCount * sizeof(VkImageBlit);
    BlitImageParams *paramStruct = initCommand<BlitImageParams>(CommandID::BlitImage, regionSize);
    paramStruct->srcImage        = srcImage.getHandle();
    paramStruct->srcImageLayout  = srcImageLayout;
    paramStruct->dstImage        = dstImage.getHandle();
    paramStruct->dstImageLayout  = dstImageLayout;
    paramStruct->regionCount     = regionCount;
    paramStruct->filter          = filter;
    // Copy variable sized data
    storePointerParameter(pRegions, &paramStruct->pRegions, regionSize);
}

void SecondaryCommandBuffer::copyBuffer(const Buffer &srcBuffer,
                                        const Buffer &destBuffer,
                                        uint32_t regionCount,
                                        const VkBufferCopy *regions)
{
    size_t regionSize = regionCount * sizeof(VkBufferCopy);
    CopyBufferParams *paramStruct =
        initCommand<CopyBufferParams>(CommandID::CopyBuffer, regionSize);
    paramStruct->srcBuffer   = srcBuffer.getHandle();
    paramStruct->destBuffer  = destBuffer.getHandle();
    paramStruct->regionCount = regionCount;
    // Copy variable sized data
    storePointerParameter(regions, &paramStruct->regions, regionSize);
}

void SecondaryCommandBuffer::copyBufferToImage(VkBuffer srcBuffer,
                                               const Image &dstImage,
                                               VkImageLayout dstImageLayout,
                                               uint32_t regionCount,
                                               const VkBufferImageCopy *regions)
{
    size_t regionSize = regionCount * sizeof(VkBufferImageCopy);
    CopyBufferToImageParams *paramStruct =
        initCommand<CopyBufferToImageParams>(CommandID::CopyBufferToImage, regionSize);
    paramStruct->srcBuffer      = srcBuffer;
    paramStruct->dstImage       = dstImage.getHandle();
    paramStruct->dstImageLayout = dstImageLayout;
    paramStruct->regionCount    = regionCount;
    // Copy variable sized data
    storePointerParameter(regions, &paramStruct->regions, regionSize);
}

void SecondaryCommandBuffer::copyImage(const Image &srcImage,
                                       VkImageLayout srcImageLayout,
                                       const Image &dstImage,
                                       VkImageLayout dstImageLayout,
                                       uint32_t regionCount,
                                       const VkImageCopy *regions)
{
    size_t regionSize            = regionCount * sizeof(VkImageCopy);
    CopyImageParams *paramStruct = initCommand<CopyImageParams>(CommandID::CopyImage, regionSize);
    paramStruct->srcImage        = srcImage.getHandle();
    paramStruct->srcImageLayout  = srcImageLayout;
    paramStruct->dstImage        = dstImage.getHandle();
    paramStruct->dstImageLayout  = dstImageLayout;
    paramStruct->regionCount     = regionCount;
    // Copy variable sized data
    storePointerParameter(regions, &paramStruct->regions, regionSize);
}

void SecondaryCommandBuffer::copyImageToBuffer(const Image &srcImage,
                                               VkImageLayout srcImageLayout,
                                               VkBuffer dstBuffer,
                                               uint32_t regionCount,
                                               const VkBufferImageCopy *regions)
{
    size_t regionSize = regionCount * sizeof(VkBufferImageCopy);
    CopyImageToBufferParams *paramStruct =
        initCommand<CopyImageToBufferParams>(CommandID::CopyImageToBuffer, regionSize);
    paramStruct->srcImage       = srcImage.getHandle();
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

void SecondaryCommandBuffer::clearColorImage(const Image &image,
                                             VkImageLayout imageLayout,
                                             const VkClearColorValue &color,
                                             uint32_t rangeCount,
                                             const VkImageSubresourceRange *ranges)
{
    size_t rangeSize = rangeCount * sizeof(VkImageSubresourceRange);
    ClearColorImageParams *paramStruct =
        initCommand<ClearColorImageParams>(CommandID::ClearColorImage, rangeSize);
    paramStruct->image       = image.getHandle();
    paramStruct->imageLayout = imageLayout;
    paramStruct->color       = color;
    paramStruct->rangeCount  = rangeCount;
    // Copy variable sized data
    storePointerParameter(ranges, &paramStruct->ranges, rangeSize);
}

void SecondaryCommandBuffer::clearDepthStencilImage(const Image &image,
                                                    VkImageLayout imageLayout,
                                                    const VkClearDepthStencilValue &depthStencil,
                                                    uint32_t rangeCount,
                                                    const VkImageSubresourceRange *ranges)
{
    size_t rangeSize = rangeCount * sizeof(VkImageSubresourceRange);
    ClearDepthStencilImageParams *paramStruct =
        initCommand<ClearDepthStencilImageParams>(CommandID::ClearDepthStencilImage, rangeSize);
    paramStruct->image        = image.getHandle();
    paramStruct->imageLayout  = imageLayout;
    paramStruct->depthStencil = depthStencil;
    paramStruct->rangeCount   = rangeCount;
    // Copy variable sized data
    storePointerParameter(ranges, &paramStruct->ranges, rangeSize);
}

void SecondaryCommandBuffer::updateBuffer(const Buffer &buffer,
                                          VkDeviceSize dstOffset,
                                          VkDeviceSize dataSize,
                                          const void *data)
{
    ASSERT(dataSize == static_cast<size_t>(dataSize));
    UpdateBufferParams *paramStruct =
        initCommand<UpdateBufferParams>(CommandID::UpdateBuffer, static_cast<size_t>(dataSize));
    paramStruct->buffer    = buffer.getHandle();
    paramStruct->dstOffset = dstOffset;
    paramStruct->dataSize  = dataSize;
    // Copy variable sized data
    storePointerParameter(data, &paramStruct->data, static_cast<size_t>(dataSize));
}

void SecondaryCommandBuffer::pushConstants(const PipelineLayout &layout,
                                           VkShaderStageFlags flag,
                                           uint32_t offset,
                                           uint32_t size,
                                           const void *data)
{
    ASSERT(size == static_cast<size_t>(size));
    PushConstantsParams *paramStruct =
        initCommand<PushConstantsParams>(CommandID::PushConstants, static_cast<size_t>(size));
    paramStruct->layout = layout.getHandle();
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

void SecondaryCommandBuffer::dispatch(uint32_t groupCountX,
                                      uint32_t groupCountY,
                                      uint32_t groupCountZ)
{
    DispatchParams *paramStruct = initCommand<DispatchParams>(CommandID::Dispatch);
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
        CommandID::PipelineBarrier, memBarrierSize + buffBarrierSize + imgBarrierSize);
    paramStruct->srcStageMask             = srcStageMask;
    paramStruct->dstStageMask             = dstStageMask;
    paramStruct->dependencyFlags          = dependencyFlags;
    paramStruct->memoryBarrierCount       = memoryBarrierCount;
    paramStruct->bufferMemoryBarrierCount = bufferMemoryBarrierCount;
    paramStruct->imageMemoryBarrierCount  = imageMemoryBarrierCount;
    // Copy variable sized data
    storePointerParameter(memoryBarriers, &paramStruct->memoryBarriers, memBarrierSize);
    storePointerParameter(bufferMemoryBarriers, &paramStruct->bufferMemoryBarriers,
                          buffBarrierSize);
    storePointerParameter(imageMemoryBarriers, &paramStruct->imageMemoryBarriers, imgBarrierSize);
}

void SecondaryCommandBuffer::imageBarrier(VkPipelineStageFlags srcStageMask,
                                          VkPipelineStageFlags dstStageMask,
                                          VkImageMemoryBarrier *imageMemoryBarrier)
{
    ImageBarrierParams *paramStruct = initCommand<ImageBarrierParams>(CommandID::ImageBarrier);
    paramStruct->srcStageMask       = srcStageMask;
    paramStruct->dstStageMask       = dstStageMask;
    paramStruct->imageMemoryBarrier = *imageMemoryBarrier;
}

void SecondaryCommandBuffer::setEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    SetEventParams *paramStruct = initCommand<SetEventParams>(CommandID::SetEvent);
    paramStruct->event          = event;
    paramStruct->stageMask      = stageMask;
}

void SecondaryCommandBuffer::resetEvent(VkEvent event, VkPipelineStageFlags stageMask)
{
    ResetEventParams *paramStruct = initCommand<ResetEventParams>(CommandID::ResetEvent);
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
        initCommand<ResetQueryPoolParams>(CommandID::ResetQueryPool);
    paramStruct->queryPool  = queryPool;
    paramStruct->firstQuery = firstQuery;
    paramStruct->queryCount = queryCount;
}

void SecondaryCommandBuffer::beginQuery(VkQueryPool queryPool,
                                        uint32_t query,
                                        VkQueryControlFlags flags)
{
    BeginQueryParams *paramStruct = initCommand<BeginQueryParams>(CommandID::BeginQuery);
    paramStruct->queryPool        = queryPool;
    paramStruct->query            = query;
    paramStruct->flags            = flags;
}

void SecondaryCommandBuffer::endQuery(VkQueryPool queryPool, uint32_t query)
{
    EndQueryParams *paramStruct = initCommand<EndQueryParams>(CommandID::EndQuery);
    paramStruct->queryPool      = queryPool;
    paramStruct->query          = query;
}

void SecondaryCommandBuffer::writeTimestamp(VkPipelineStageFlagBits pipelineStage,
                                            VkQueryPool queryPool,
                                            uint32_t query)
{
    WriteTimestampParams *paramStruct =
        initCommand<WriteTimestampParams>(CommandID::WriteTimestamp);
    paramStruct->pipelineStage = pipelineStage;
    paramStruct->queryPool     = queryPool;
    paramStruct->query         = query;
}

ANGLE_INLINE const CommandHeader *NextCommand(const CommandHeader *command)
{
    return reinterpret_cast<const CommandHeader *>(reinterpret_cast<const uint8_t *>(command) +
                                                   command->size);
}

// Parse the cmds in this cmd buffer into given primary cmd buffer
void SecondaryCommandBuffer::executeCommands(VkCommandBuffer cmdBuffer)
{
    for (const CommandHeader *command : mCommands)
    {
        for (const CommandHeader *currentCommand                      = command;
             currentCommand->id != CommandID::Invalid; currentCommand = NextCommand(currentCommand))
        {
            switch (currentCommand->id)
            {
                case CommandID::BeginQuery:
                {
                    const BeginQueryParams *params = getParamPtr<BeginQueryParams>(currentCommand);
                    vkCmdBeginQuery(cmdBuffer, params->queryPool, params->query, params->flags);
                    break;
                }
                case CommandID::BindComputePipeline:
                {
                    const BindPipelineParams *params =
                        getParamPtr<BindPipelineParams>(currentCommand);
                    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, params->pipeline);
                    break;
                }
                case CommandID::BindDescriptorSets:
                {
                    const BindDescriptorSetParams *params =
                        getParamPtr<BindDescriptorSetParams>(currentCommand);
                    vkCmdBindDescriptorSets(cmdBuffer, params->bindPoint, params->layout,
                                            params->firstSet, params->descriptorSetCount,
                                            params->descriptorSets, params->dynamicOffsetCount,
                                            params->dynamicOffsets);
                    break;
                }
                case CommandID::BindGraphicsPipeline:
                {
                    const BindPipelineParams *params =
                        getParamPtr<BindPipelineParams>(currentCommand);
                    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, params->pipeline);
                    break;
                }
                case CommandID::BindIndexBuffer:
                {
                    const BindIndexBufferParams *params =
                        getParamPtr<BindIndexBufferParams>(currentCommand);
                    vkCmdBindIndexBuffer(cmdBuffer, params->buffer, params->offset,
                                         params->indexType);
                    break;
                }
                case CommandID::BindVertexBuffers:
                {
                    const BindVertexBuffersParams *params =
                        getParamPtr<BindVertexBuffersParams>(currentCommand);
                    const VkBuffer *buffers =
                        Offset<VkBuffer>(params, sizeof(BindVertexBuffersParams));
                    const VkDeviceSize *offsets =
                        Offset<VkDeviceSize>(buffers, sizeof(VkBuffer) * params->bindingCount);
                    vkCmdBindVertexBuffers(cmdBuffer, 0, params->bindingCount, buffers, offsets);
                    break;
                }
                case CommandID::BlitImage:
                {
                    const BlitImageParams *params = getParamPtr<BlitImageParams>(currentCommand);
                    vkCmdBlitImage(cmdBuffer, params->srcImage, params->srcImageLayout,
                                   params->dstImage, params->dstImageLayout, params->regionCount,
                                   params->pRegions, params->filter);
                    break;
                }
                case CommandID::ClearAttachments:
                {
                    const ClearAttachmentsParams *params =
                        getParamPtr<ClearAttachmentsParams>(currentCommand);
                    vkCmdClearAttachments(cmdBuffer, params->attachmentCount, params->attachments,
                                          params->rectCount, params->rects);
                    break;
                }
                case CommandID::ClearColorImage:
                {
                    const ClearColorImageParams *params =
                        getParamPtr<ClearColorImageParams>(currentCommand);
                    vkCmdClearColorImage(cmdBuffer, params->image, params->imageLayout,
                                         &params->color, params->rangeCount, params->ranges);
                    break;
                }
                case CommandID::ClearDepthStencilImage:
                {
                    const ClearDepthStencilImageParams *params =
                        getParamPtr<ClearDepthStencilImageParams>(currentCommand);
                    vkCmdClearDepthStencilImage(cmdBuffer, params->image, params->imageLayout,
                                                &params->depthStencil, params->rangeCount,
                                                params->ranges);
                    break;
                }
                case CommandID::CopyBuffer:
                {
                    const CopyBufferParams *params = getParamPtr<CopyBufferParams>(currentCommand);
                    vkCmdCopyBuffer(cmdBuffer, params->srcBuffer, params->destBuffer,
                                    params->regionCount, params->regions);
                    break;
                }
                case CommandID::CopyBufferToImage:
                {
                    const CopyBufferToImageParams *params =
                        getParamPtr<CopyBufferToImageParams>(currentCommand);
                    vkCmdCopyBufferToImage(cmdBuffer, params->srcBuffer, params->dstImage,
                                           params->dstImageLayout, params->regionCount,
                                           params->regions);
                    break;
                }
                case CommandID::CopyImage:
                {
                    const CopyImageParams *params = getParamPtr<CopyImageParams>(currentCommand);
                    vkCmdCopyImage(cmdBuffer, params->srcImage, params->srcImageLayout,
                                   params->dstImage, params->dstImageLayout, params->regionCount,
                                   params->regions);
                    break;
                }
                case CommandID::CopyImageToBuffer:
                {
                    const CopyImageToBufferParams *params =
                        getParamPtr<CopyImageToBufferParams>(currentCommand);
                    vkCmdCopyImageToBuffer(cmdBuffer, params->srcImage, params->srcImageLayout,
                                           params->dstBuffer, params->regionCount, params->regions);
                    break;
                }
                case CommandID::Dispatch:
                {
                    const DispatchParams *params = getParamPtr<DispatchParams>(currentCommand);
                    vkCmdDispatch(cmdBuffer, params->groupCountX, params->groupCountY,
                                  params->groupCountZ);
                    break;
                }
                case CommandID::Draw:
                {
                    const DrawParams *params = getParamPtr<DrawParams>(currentCommand);
                    vkCmdDraw(cmdBuffer, params->vertexCount, 1, params->firstVertex, 0);
                    break;
                }
                case CommandID::DrawIndexed:
                {
                    const DrawIndexedParams *params =
                        getParamPtr<DrawIndexedParams>(currentCommand);
                    vkCmdDrawIndexed(cmdBuffer, params->indexCount, 1, 0, 0, 0);
                    break;
                }
                case CommandID::DrawIndexedInstanced:
                {
                    const DrawIndexedInstancedParams *params =
                        getParamPtr<DrawIndexedInstancedParams>(currentCommand);
                    vkCmdDrawIndexed(cmdBuffer, params->indexCount, params->instanceCount, 0, 0, 0);
                    break;
                }
                case CommandID::DrawInstanced:
                {
                    const DrawInstancedParams *params =
                        getParamPtr<DrawInstancedParams>(currentCommand);
                    vkCmdDraw(cmdBuffer, params->vertexCount, params->instanceCount,
                              params->firstVertex, 0);
                    break;
                }
                case CommandID::EndQuery:
                {
                    const EndQueryParams *params = getParamPtr<EndQueryParams>(currentCommand);
                    vkCmdEndQuery(cmdBuffer, params->queryPool, params->query);
                    break;
                }
                case CommandID::ImageBarrier:
                {
                    const ImageBarrierParams *params =
                        getParamPtr<ImageBarrierParams>(currentCommand);
                    vkCmdPipelineBarrier(cmdBuffer, params->srcStageMask, params->dstStageMask, 0,
                                         0, nullptr, 0, nullptr, 1, &params->imageMemoryBarrier);
                    break;
                }
                case CommandID::PipelineBarrier:
                {
                    const PipelineBarrierParams *params =
                        getParamPtr<PipelineBarrierParams>(currentCommand);
                    vkCmdPipelineBarrier(
                        cmdBuffer, params->srcStageMask, params->dstStageMask,
                        params->dependencyFlags, params->memoryBarrierCount, params->memoryBarriers,
                        params->bufferMemoryBarrierCount, params->bufferMemoryBarriers,
                        params->imageMemoryBarrierCount, params->imageMemoryBarriers);
                    break;
                }
                case CommandID::PushConstants:
                {
                    const PushConstantsParams *params =
                        getParamPtr<PushConstantsParams>(currentCommand);
                    vkCmdPushConstants(cmdBuffer, params->layout, params->flag, params->offset,
                                       params->size, params->data);
                    break;
                }
                case CommandID::ResetEvent:
                {
                    const ResetEventParams *params = getParamPtr<ResetEventParams>(currentCommand);
                    vkCmdResetEvent(cmdBuffer, params->event, params->stageMask);
                    break;
                }
                case CommandID::ResetQueryPool:
                {
                    const ResetQueryPoolParams *params =
                        getParamPtr<ResetQueryPoolParams>(currentCommand);
                    vkCmdResetQueryPool(cmdBuffer, params->queryPool, params->firstQuery,
                                        params->queryCount);
                    break;
                }
                case CommandID::SetEvent:
                {
                    const SetEventParams *params = getParamPtr<SetEventParams>(currentCommand);
                    vkCmdSetEvent(cmdBuffer, params->event, params->stageMask);
                    break;
                }
                case CommandID::SetScissor:
                {
                    const SetScissorParams *params = getParamPtr<SetScissorParams>(currentCommand);
                    vkCmdSetScissor(cmdBuffer, params->firstScissor, params->scissorCount,
                                    params->scissors);
                    break;
                }
                case CommandID::SetViewport:
                {
                    const SetViewportParams *params =
                        getParamPtr<SetViewportParams>(currentCommand);
                    vkCmdSetViewport(cmdBuffer, params->firstViewport, params->viewportCount,
                                     params->viewports);
                    break;
                }
                case CommandID::UpdateBuffer:
                {
                    const UpdateBufferParams *params =
                        getParamPtr<UpdateBufferParams>(currentCommand);
                    vkCmdUpdateBuffer(cmdBuffer, params->buffer, params->dstOffset,
                                      params->dataSize, params->data);
                    break;
                }
                case CommandID::WaitEvents:
                {
                    const WaitEventsParams *params = getParamPtr<WaitEventsParams>(currentCommand);
                    vkCmdWaitEvents(cmdBuffer, params->eventCount, params->events,
                                    params->srcStageMask, params->dstStageMask,
                                    params->memoryBarrierCount, params->memoryBarriers,
                                    params->bufferMemoryBarrierCount, params->bufferMemoryBarriers,
                                    params->imageMemoryBarrierCount, params->imageMemoryBarriers);
                    break;
                }
                case CommandID::WriteTimestamp:
                {
                    const WriteTimestampParams *params =
                        getParamPtr<WriteTimestampParams>(currentCommand);
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
}

}  // namespace vk
}  // namespace rx
