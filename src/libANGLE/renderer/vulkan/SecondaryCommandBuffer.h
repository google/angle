//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SecondaryCommandBuffer:
//    Lightweight, CPU-Side command buffers used to hold command state until
//    it has to be submitted to GPU.
//

#ifndef LIBANGLE_RENDERER_VULKAN_SECONDARYCOMMANDBUFFERVK_H_
#define LIBANGLE_RENDERER_VULKAN_SECONDARYCOMMANDBUFFERVK_H_

#include <vulkan/vulkan.h>

#include "common/PoolAlloc.h"
#include "libANGLE/renderer/vulkan/vk_wrapper.h"

namespace rx
{

namespace vk
{

enum class CommandID : uint16_t
{
    // Invalid cmd used to mark end of sequence of commands
    Invalid = 0,
    BeginQuery,
    BindComputePipeline,
    BindDescriptorSets,
    BindGraphicsPipeline,
    BindIndexBuffer,
    BindVertexBuffers,
    BlitImage,
    ClearAttachments,
    ClearColorImage,
    ClearDepthStencilImage,
    CopyBuffer,
    CopyBufferToImage,
    CopyImage,
    CopyImageToBuffer,
    Dispatch,
    Draw,
    DrawIndexed,
    DrawIndexedInstanced,
    DrawInstanced,
    EndQuery,
    ImageBarrier,
    PipelineBarrier,
    PushConstants,
    ResetEvent,
    ResetQueryPool,
    SetEvent,
    SetScissor,
    SetViewport,
    UpdateBuffer,
    WaitEvents,
    WriteTimestamp,
};

#define VERIFY_4_BYTE_ALIGNMENT(StructName) \
    static_assert((sizeof(StructName) % 4) == 0, "Check StructName alignment");

// Structs to encapsulate parameters for different commands
// This makes it easy to know the size of params & to copy params
// TODO: Could optimize the size of some of these structs through bit-packing
//  and customizing sizing based on limited parameter sets used by ANGLE
struct BindDescriptorSetParams
{
    VkPipelineBindPoint bindPoint;
    VkPipelineLayout layout;
    uint32_t firstSet;
    uint32_t descriptorSetCount;
    const VkDescriptorSet *descriptorSets;
    uint32_t dynamicOffsetCount;
    const uint32_t *dynamicOffsets;
};
VERIFY_4_BYTE_ALIGNMENT(BindDescriptorSetParams)

struct BindIndexBufferParams
{
    VkBuffer buffer;
    VkDeviceSize offset;
    VkIndexType indexType;
};
VERIFY_4_BYTE_ALIGNMENT(BindIndexBufferParams)

struct BindPipelineParams
{
    VkPipeline pipeline;
};
VERIFY_4_BYTE_ALIGNMENT(BindPipelineParams)

struct BindVertexBuffersParams
{
    // ANGLE always has firstBinding of 0 so not storing that currently
    uint32_t bindingCount;
};
VERIFY_4_BYTE_ALIGNMENT(BindVertexBuffersParams)

struct BlitImageParams
{
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageBlit *pRegions;
    VkFilter filter;
};
VERIFY_4_BYTE_ALIGNMENT(BlitImageParams)

struct CopyBufferParams
{
    VkBuffer srcBuffer;
    VkBuffer destBuffer;
    uint32_t regionCount;
    const VkBufferCopy *regions;
};
VERIFY_4_BYTE_ALIGNMENT(CopyBufferParams)

struct CopyBufferToImageParams
{
    VkBuffer srcBuffer;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkBufferImageCopy *regions;
};
VERIFY_4_BYTE_ALIGNMENT(CopyBufferToImageParams)

struct CopyImageParams
{
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    VkImageLayout dstImageLayout;
    uint32_t regionCount;
    const VkImageCopy *regions;
};
VERIFY_4_BYTE_ALIGNMENT(CopyImageParams)

struct CopyImageToBufferParams
{
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkBuffer dstBuffer;
    uint32_t regionCount;
    const VkBufferImageCopy *regions;
};
VERIFY_4_BYTE_ALIGNMENT(CopyImageToBufferParams)

struct ClearAttachmentsParams
{
    uint32_t attachmentCount;
    const VkClearAttachment *attachments;
    uint32_t rectCount;
    const VkClearRect *rects;
};
VERIFY_4_BYTE_ALIGNMENT(ClearAttachmentsParams)

struct ClearColorImageParams
{
    VkImage image;
    VkImageLayout imageLayout;
    VkClearColorValue color;
    uint32_t rangeCount;
    const VkImageSubresourceRange *ranges;
};
VERIFY_4_BYTE_ALIGNMENT(ClearColorImageParams)

struct ClearDepthStencilImageParams
{
    VkImage image;
    VkImageLayout imageLayout;
    VkClearDepthStencilValue depthStencil;
    uint32_t rangeCount;
    const VkImageSubresourceRange *ranges;
};
VERIFY_4_BYTE_ALIGNMENT(ClearDepthStencilImageParams)

struct UpdateBufferParams
{
    VkBuffer buffer;
    VkDeviceSize dstOffset;
    VkDeviceSize dataSize;
    const void *data;
};
VERIFY_4_BYTE_ALIGNMENT(UpdateBufferParams)

struct PushConstantsParams
{
    VkPipelineLayout layout;
    VkShaderStageFlags flag;
    uint32_t offset;
    uint32_t size;
    const void *data;
};
VERIFY_4_BYTE_ALIGNMENT(PushConstantsParams)

struct SetViewportParams
{
    uint32_t firstViewport;
    uint32_t viewportCount;
    const VkViewport *viewports;
};
VERIFY_4_BYTE_ALIGNMENT(SetViewportParams)

struct SetScissorParams
{
    uint32_t firstScissor;
    uint32_t scissorCount;
    const VkRect2D *scissors;
};
VERIFY_4_BYTE_ALIGNMENT(SetScissorParams)

struct DrawParams
{
    uint32_t vertexCount;
    uint32_t firstVertex;
};
VERIFY_4_BYTE_ALIGNMENT(DrawParams)

struct DrawInstancedParams
{
    uint32_t vertexCount;
    uint32_t instanceCount;
    uint32_t firstVertex;
};
VERIFY_4_BYTE_ALIGNMENT(DrawInstancedParams)

struct DrawIndexedParams
{
    uint32_t indexCount;
};
VERIFY_4_BYTE_ALIGNMENT(DrawIndexedParams)

struct DrawIndexedInstancedParams
{
    uint32_t indexCount;
    uint32_t instanceCount;
};
VERIFY_4_BYTE_ALIGNMENT(DrawIndexedInstancedParams)

struct DispatchParams
{
    uint32_t groupCountX;
    uint32_t groupCountY;
    uint32_t groupCountZ;
};
VERIFY_4_BYTE_ALIGNMENT(DispatchParams)

struct PipelineBarrierParams
{
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkDependencyFlags dependencyFlags;
    uint32_t memoryBarrierCount;
    const VkMemoryBarrier *memoryBarriers;
    uint32_t bufferMemoryBarrierCount;
    const VkBufferMemoryBarrier *bufferMemoryBarriers;
    uint32_t imageMemoryBarrierCount;
    const VkImageMemoryBarrier *imageMemoryBarriers;
};
VERIFY_4_BYTE_ALIGNMENT(PipelineBarrierParams)

struct ImageBarrierParams
{
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    VkImageMemoryBarrier imageMemoryBarrier;
};
VERIFY_4_BYTE_ALIGNMENT(ImageBarrierParams)

struct SetEventParams
{
    VkEvent event;
    VkPipelineStageFlags stageMask;
};
VERIFY_4_BYTE_ALIGNMENT(SetEventParams)

struct ResetEventParams
{
    VkEvent event;
    VkPipelineStageFlags stageMask;
};
VERIFY_4_BYTE_ALIGNMENT(ResetEventParams)

struct WaitEventsParams
{
    uint32_t eventCount;
    const VkEvent *events;
    VkPipelineStageFlags srcStageMask;
    VkPipelineStageFlags dstStageMask;
    uint32_t memoryBarrierCount;
    const VkMemoryBarrier *memoryBarriers;
    uint32_t bufferMemoryBarrierCount;
    const VkBufferMemoryBarrier *bufferMemoryBarriers;
    uint32_t imageMemoryBarrierCount;
    const VkImageMemoryBarrier *imageMemoryBarriers;
};
VERIFY_4_BYTE_ALIGNMENT(WaitEventsParams)

struct ResetQueryPoolParams
{
    VkQueryPool queryPool;
    uint32_t firstQuery;
    uint32_t queryCount;
};
VERIFY_4_BYTE_ALIGNMENT(ResetQueryPoolParams)

struct BeginQueryParams
{
    VkQueryPool queryPool;
    uint32_t query;
    VkQueryControlFlags flags;
};
VERIFY_4_BYTE_ALIGNMENT(BeginQueryParams)

struct EndQueryParams
{
    VkQueryPool queryPool;
    uint32_t query;
};
VERIFY_4_BYTE_ALIGNMENT(EndQueryParams)

struct WriteTimestampParams
{
    VkPipelineStageFlagBits pipelineStage;
    VkQueryPool queryPool;
    uint32_t query;
};
VERIFY_4_BYTE_ALIGNMENT(WriteTimestampParams)

// Header for every cmd in custom cmd buffer
struct CommandHeader
{
    CommandID id;
    uint16_t size;
};

static_assert(sizeof(CommandHeader) == 4, "Check CommandHeader size");

template <typename DestT, typename T>
ANGLE_INLINE DestT *Offset(T *ptr, size_t bytes)
{
    return reinterpret_cast<DestT *>((reinterpret_cast<uint8_t *>(ptr) + bytes));
}

template <typename DestT, typename T>
ANGLE_INLINE const DestT *Offset(const T *ptr, size_t bytes)
{
    return reinterpret_cast<const DestT *>((reinterpret_cast<const uint8_t *>(ptr) + bytes));
}

class SecondaryCommandBuffer final : angle::NonCopyable
{
  public:
    SecondaryCommandBuffer();
    ~SecondaryCommandBuffer();

    // Add commands
    void bindDescriptorSets(VkPipelineBindPoint bindPoint,
                            const PipelineLayout &layout,
                            uint32_t firstSet,
                            uint32_t descriptorSetCount,
                            const VkDescriptorSet *descriptorSets,
                            uint32_t dynamicOffsetCount,
                            const uint32_t *dynamicOffsets);

    void bindIndexBuffer(const Buffer &buffer, VkDeviceSize offset, VkIndexType indexType);

    void bindGraphicsPipeline(const Pipeline &pipeline);

    void bindComputePipeline(const Pipeline &pipeline);

    void bindVertexBuffers(uint32_t firstBinding,
                           uint32_t bindingCount,
                           const VkBuffer *buffers,
                           const VkDeviceSize *offsets);

    void blitImage(const Image &srcImage,
                   VkImageLayout srcImageLayout,
                   const Image &dstImage,
                   VkImageLayout dstImageLayout,
                   uint32_t regionCount,
                   const VkImageBlit *pRegions,
                   VkFilter filter);

    void copyBuffer(const Buffer &srcBuffer,
                    const Buffer &destBuffer,
                    uint32_t regionCount,
                    const VkBufferCopy *regions);

    void copyBufferToImage(VkBuffer srcBuffer,
                           const Image &dstImage,
                           VkImageLayout dstImageLayout,
                           uint32_t regionCount,
                           const VkBufferImageCopy *regions);

    void copyImage(const Image &srcImage,
                   VkImageLayout srcImageLayout,
                   const Image &dstImage,
                   VkImageLayout dstImageLayout,
                   uint32_t regionCount,
                   const VkImageCopy *regions);

    void copyImageToBuffer(const Image &srcImage,
                           VkImageLayout srcImageLayout,
                           VkBuffer dstBuffer,
                           uint32_t regionCount,
                           const VkBufferImageCopy *regions);

    void clearAttachments(uint32_t attachmentCount,
                          const VkClearAttachment *attachments,
                          uint32_t rectCount,
                          const VkClearRect *rects);

    void clearColorImage(const Image &image,
                         VkImageLayout imageLayout,
                         const VkClearColorValue &color,
                         uint32_t rangeCount,
                         const VkImageSubresourceRange *ranges);

    void clearDepthStencilImage(const Image &image,
                                VkImageLayout imageLayout,
                                const VkClearDepthStencilValue &depthStencil,
                                uint32_t rangeCount,
                                const VkImageSubresourceRange *ranges);

    void updateBuffer(const Buffer &buffer,
                      VkDeviceSize dstOffset,
                      VkDeviceSize dataSize,
                      const void *data);

    void pushConstants(const PipelineLayout &layout,
                       VkShaderStageFlags flag,
                       uint32_t offset,
                       uint32_t size,
                       const void *data);

    void setViewport(uint32_t firstViewport, uint32_t viewportCount, const VkViewport *viewports);
    void setScissor(uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *scissors);

    void draw(uint32_t vertexCount, uint32_t firstVertex);

    void drawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex);

    void drawIndexed(uint32_t indexCount);

    void drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount);

    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    void pipelineBarrier(VkPipelineStageFlags srcStageMask,
                         VkPipelineStageFlags dstStageMask,
                         VkDependencyFlags dependencyFlags,
                         uint32_t memoryBarrierCount,
                         const VkMemoryBarrier *memoryBarriers,
                         uint32_t bufferMemoryBarrierCount,
                         const VkBufferMemoryBarrier *bufferMemoryBarriers,
                         uint32_t imageMemoryBarrierCount,
                         const VkImageMemoryBarrier *imageMemoryBarriers);

    void imageBarrier(VkPipelineStageFlags srcStageMask,
                      VkPipelineStageFlags dstStageMask,
                      VkImageMemoryBarrier *imageMemoryBarrier);

    void setEvent(VkEvent event, VkPipelineStageFlags stageMask);
    void resetEvent(VkEvent event, VkPipelineStageFlags stageMask);
    void waitEvents(uint32_t eventCount,
                    const VkEvent *events,
                    VkPipelineStageFlags srcStageMask,
                    VkPipelineStageFlags dstStageMask,
                    uint32_t memoryBarrierCount,
                    const VkMemoryBarrier *memoryBarriers,
                    uint32_t bufferMemoryBarrierCount,
                    const VkBufferMemoryBarrier *bufferMemoryBarriers,
                    uint32_t imageMemoryBarrierCount,
                    const VkImageMemoryBarrier *imageMemoryBarriers);

    void resetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);
    void beginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags);
    void endQuery(VkQueryPool queryPool, uint32_t query);
    void writeTimestamp(VkPipelineStageFlagBits pipelineStage,
                        VkQueryPool queryPool,
                        uint32_t query);
    // No-op for compatibility
    VkResult end() { return VK_SUCCESS; }

    // Parse the cmds in this cmd buffer into given primary cmd buffer for execution
    void executeCommands(VkCommandBuffer cmdBuffer);
    // Pool Alloc uses 16kB pages w/ 16byte header = 16368bytes. To minimize waste
    //  using a 16368/12 = 1364. Also better perf than 1024 due to fewer block allocations
    static constexpr size_t kBlockSize = 1364;
    // Make sure block size is 4-byte aligned to avoid Android errors
    static_assert((kBlockSize % 4) == 0, "Check kBlockSize alignment");

    // Initialize the SecondaryCommandBuffer by setting the allocator it will use
    void initialize(angle::PoolAllocator *allocator)
    {
        ASSERT(allocator);
        mAllocator = allocator;
        allocateNewBlock();
        // Set first command to Invalid to start
        reinterpret_cast<CommandHeader *>(mCurrentWritePointer)->id = CommandID::Invalid;
    }

    // This will cause the SecondaryCommandBuffer to become invalid by clearing its allocator
    void releaseHandle() { mAllocator = nullptr; }
    // The SecondaryCommandBuffer is valid if it's been initialized
    bool valid() { return mAllocator != nullptr; }

  private:
    template <class StructType>
    ANGLE_INLINE StructType *commonInit(CommandID cmdID, size_t allocationSize)
    {
        mCurrentBytesRemaining -= allocationSize;

        CommandHeader *header = reinterpret_cast<CommandHeader *>(mCurrentWritePointer);
        header->id            = cmdID;
        header->size          = static_cast<uint16_t>(allocationSize);
        ASSERT(allocationSize <= std::numeric_limits<uint16_t>::max());

        mCurrentWritePointer += allocationSize;
        // Set next cmd header to Invalid (0) so cmd sequence will be terminated
        reinterpret_cast<CommandHeader *>(mCurrentWritePointer)->id = CommandID::Invalid;
        return Offset<StructType>(header, sizeof(CommandHeader));
    }
    ANGLE_INLINE void allocateNewBlock()
    {
        ASSERT(mAllocator);
        mCurrentWritePointer   = mAllocator->fastAllocate(kBlockSize);
        mCurrentBytesRemaining = kBlockSize;
        mCommands.push_back(reinterpret_cast<CommandHeader *>(mCurrentWritePointer));
    }

    // Allocate and initialize memory for given commandID & variable param size
    //  returning a pointer to the start of the commands parameter data and updating
    //  mPtrCmdData to just past the fixed parameter data.
    template <class StructType>
    ANGLE_INLINE StructType *initCommand(CommandID cmdID, size_t variableSize)
    {
        constexpr size_t fixedAllocationSize = sizeof(StructType) + sizeof(CommandHeader);
        const size_t allocationSize          = fixedAllocationSize + variableSize;
        // Make sure we have enough room to mark follow-on header "Invalid"
        if (mCurrentBytesRemaining <= (allocationSize + sizeof(CommandHeader)))
        {
            allocateNewBlock();
        }
        mPtrCmdData = mCurrentWritePointer + fixedAllocationSize;
        return commonInit<StructType>(cmdID, allocationSize);
    }

    // Initialize a command that doesn't have variable-sized ptr data
    template <class StructType>
    ANGLE_INLINE StructType *initCommand(CommandID cmdID)
    {
        constexpr size_t allocationSize = sizeof(StructType) + sizeof(CommandHeader);
        // Make sure we have enough room to mark follow-on header "Invalid"
        if (mCurrentBytesRemaining <= (allocationSize + sizeof(CommandHeader)))
        {
            allocateNewBlock();
        }
        return commonInit<StructType>(cmdID, allocationSize);
    }

    // Return a ptr to the parameter type
    template <class StructType>
    const StructType *getParamPtr(const CommandHeader *header) const
    {
        return reinterpret_cast<const StructType *>(reinterpret_cast<const uint8_t *>(header) +
                                                    sizeof(CommandHeader));
    }
    // Copy sizeInBytes data from paramData to mPtrCmdData and assign *writePtr
    //  to mPtrCmdData. Then increment mPtrCmdData by sizeInBytes.
    // Precondition: initCommand() must have already been called on the given cmd
    template <class PtrType>
    void storePointerParameter(const PtrType *paramData,
                               const PtrType **writePtr,
                               size_t sizeInBytes)
    {
        if (sizeInBytes == 0)
            return;
        *writePtr = reinterpret_cast<const PtrType *>(mPtrCmdData);
        memcpy(mPtrCmdData, paramData, sizeInBytes);
        mPtrCmdData += sizeInBytes;
    }

    std::vector<CommandHeader *> mCommands;

    // Allocator used by this class. If non-null then the class is valid.
    angle::PoolAllocator *mAllocator;

    uint8_t *mCurrentWritePointer;
    size_t mCurrentBytesRemaining;

    // Ptr to write variable ptr data section of cmd into.
    //  This is set to just past fixed parameter data when initCommand() is called
    uint8_t *mPtrCmdData;
};

ANGLE_INLINE SecondaryCommandBuffer::SecondaryCommandBuffer()
    : mAllocator(nullptr), mCurrentWritePointer(nullptr), mCurrentBytesRemaining(0)
{}
ANGLE_INLINE SecondaryCommandBuffer::~SecondaryCommandBuffer() {}

ANGLE_INLINE void SecondaryCommandBuffer::bindDescriptorSets(VkPipelineBindPoint bindPoint,
                                                             const PipelineLayout &layout,
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
    paramStruct->layout             = layout.getHandle();
    paramStruct->firstSet           = firstSet;
    paramStruct->descriptorSetCount = descriptorSetCount;
    paramStruct->dynamicOffsetCount = dynamicOffsetCount;
    // Copy variable sized data
    storePointerParameter(descriptorSets, &paramStruct->descriptorSets, descSize);
    storePointerParameter(dynamicOffsets, &paramStruct->dynamicOffsets, offsetSize);
}

ANGLE_INLINE void SecondaryCommandBuffer::bindIndexBuffer(const Buffer &buffer,
                                                          VkDeviceSize offset,
                                                          VkIndexType indexType)
{
    BindIndexBufferParams *paramStruct =
        initCommand<BindIndexBufferParams>(CommandID::BindIndexBuffer);
    paramStruct->buffer    = buffer.getHandle();
    paramStruct->offset    = offset;
    paramStruct->indexType = indexType;
}

ANGLE_INLINE void SecondaryCommandBuffer::bindGraphicsPipeline(const Pipeline &pipeline)
{
    BindPipelineParams *paramStruct =
        initCommand<BindPipelineParams>(CommandID::BindGraphicsPipeline);
    paramStruct->pipeline = pipeline.getHandle();
}

ANGLE_INLINE void SecondaryCommandBuffer::bindComputePipeline(const Pipeline &pipeline)
{
    BindPipelineParams *paramStruct =
        initCommand<BindPipelineParams>(CommandID::BindComputePipeline);
    paramStruct->pipeline = pipeline.getHandle();
}

ANGLE_INLINE void SecondaryCommandBuffer::bindVertexBuffers(uint32_t firstBinding,
                                                            uint32_t bindingCount,
                                                            const VkBuffer *buffers,
                                                            const VkDeviceSize *offsets)
{
    ASSERT(firstBinding == 0);
    size_t buffersSize                   = bindingCount * sizeof(VkBuffer);
    size_t offsetsSize                   = bindingCount * sizeof(VkDeviceSize);
    BindVertexBuffersParams *paramStruct = initCommand<BindVertexBuffersParams>(
        CommandID::BindVertexBuffers, buffersSize + offsetsSize);
    // Copy params
    paramStruct->bindingCount = bindingCount;
    uint8_t *writePointer     = Offset<uint8_t>(paramStruct, sizeof(BindVertexBuffersParams));
    memcpy(writePointer, buffers, buffersSize);
    writePointer += buffersSize;
    memcpy(writePointer, offsets, offsetsSize);
}

ANGLE_INLINE void SecondaryCommandBuffer::draw(uint32_t vertexCount, uint32_t firstVertex)
{
    DrawParams *paramStruct  = initCommand<DrawParams>(CommandID::Draw);
    paramStruct->vertexCount = vertexCount;
    paramStruct->firstVertex = firstVertex;
}

ANGLE_INLINE void SecondaryCommandBuffer::drawInstanced(uint32_t vertexCount,
                                                        uint32_t instanceCount,
                                                        uint32_t firstVertex)
{
    DrawInstancedParams *paramStruct = initCommand<DrawInstancedParams>(CommandID::DrawInstanced);
    paramStruct->vertexCount         = vertexCount;
    paramStruct->instanceCount       = instanceCount;
    paramStruct->firstVertex         = firstVertex;
}

ANGLE_INLINE void SecondaryCommandBuffer::drawIndexed(uint32_t indexCount)
{
    DrawIndexedParams *paramStruct = initCommand<DrawIndexedParams>(CommandID::DrawIndexed);
    paramStruct->indexCount        = indexCount;
}

ANGLE_INLINE void SecondaryCommandBuffer::drawIndexedInstanced(uint32_t indexCount,
                                                               uint32_t instanceCount)
{
    DrawIndexedInstancedParams *paramStruct =
        initCommand<DrawIndexedInstancedParams>(CommandID::DrawIndexedInstanced);
    paramStruct->indexCount    = indexCount;
    paramStruct->instanceCount = instanceCount;
}

}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_SECONDARYCOMMANDBUFFERVK_H_
