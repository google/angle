//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BufferVk.cpp:
//    Implements the class methods for BufferVk.
//

#include "libANGLE/renderer/vulkan/BufferVk.h"

#include "common/FixedVector.h"
#include "common/debug.h"
#include "common/mathutil.h"
#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/trace.h"

namespace rx
{

namespace
{
// Vertex attribute buffers are used as storage buffers for conversion in compute, where access to
// the buffer is made in 4-byte chunks.  Assume the size of the buffer is 4k+n where n is in [0, 3).
// On some hardware, reading 4 bytes from address 4k returns 0, making it impossible to read the
// last n bytes.  By rounding up the buffer sizes to a multiple of 4, the problem is alleviated.
constexpr size_t kBufferSizeGranularity = 4;
static_assert(gl::isPow2(kBufferSizeGranularity), "use as alignment, must be power of two");

// Start with a fairly small buffer size. We can increase this dynamically as we convert more data.
constexpr size_t kConvertedArrayBufferInitialSize = 1024 * 8;

// Buffers that have a static usage pattern will be allocated in
// device local memory to speed up access to and from the GPU.
// Dynamic usage patterns or that are frequently mapped
// will now request host cached memory to speed up access from the CPU.
ANGLE_INLINE VkMemoryPropertyFlags GetPreferredMemoryType(gl::BufferBinding target,
                                                          gl::BufferUsage usage)
{
    constexpr VkMemoryPropertyFlags kDeviceLocalFlags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    constexpr VkMemoryPropertyFlags kHostCachedFlags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
         VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    constexpr VkMemoryPropertyFlags kHostUncachedFlags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (target == gl::BufferBinding::PixelUnpack)
    {
        return kHostCachedFlags;
    }

    switch (usage)
    {
        case gl::BufferUsage::StaticCopy:
        case gl::BufferUsage::StaticDraw:
        case gl::BufferUsage::StaticRead:
            // For static usage, request a device local memory
            return kDeviceLocalFlags;
        case gl::BufferUsage::DynamicDraw:
        case gl::BufferUsage::StreamDraw:
            // For non-static usage where the CPU performs a write-only access, request
            // a host uncached memory
            return kHostUncachedFlags;
        case gl::BufferUsage::DynamicCopy:
        case gl::BufferUsage::DynamicRead:
        case gl::BufferUsage::StreamCopy:
        case gl::BufferUsage::StreamRead:
            // For all other types of usage, request a host cached memory
            return kHostCachedFlags;
        default:
            UNREACHABLE();
            return kHostCachedFlags;
    }
}

ANGLE_INLINE VkMemoryPropertyFlags GetStorageMemoryType(GLbitfield storageFlags,
                                                        bool externalBuffer)
{
    constexpr VkMemoryPropertyFlags kDeviceLocalHostVisibleFlags =
        (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    constexpr VkMemoryPropertyFlags kDeviceLocalHostCoherentFlags =
        (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    const bool isCoherentMap   = (storageFlags & GL_MAP_COHERENT_BIT_EXT) != 0;
    const bool isPersistentMap = (storageFlags & GL_MAP_PERSISTENT_BIT_EXT) != 0;

    if (isCoherentMap || isPersistentMap || externalBuffer)
    {
        // We currently allocate coherent memory for persistently mapped buffers.
        // GL_EXT_buffer_storage allows non-coherent memory, but currently the implementation of
        // |glMemoryBarrier(CLIENT_MAPPED_BUFFER_BARRIER_BIT_EXT)| relies on the mapping being
        // coherent.
        //
        // If persistently mapped buffers ever use non-coherent memory, then said |glMemoryBarrier|
        // call must result in |vkInvalidateMappedMemoryRanges| for all persistently mapped buffers.
        return kDeviceLocalHostCoherentFlags;
    }

    return kDeviceLocalHostVisibleFlags;
}

size_t GetPreferredDynamicBufferInitialSize(RendererVk *renderer,
                                            size_t dataSize,
                                            gl::BufferUsage usage,
                                            size_t *alignmentOut)
{
    // The buffer may be used for a number of different operations, so its allocations should
    // have an alignment that satisifies all.
    const VkPhysicalDeviceLimits &limitsVk = renderer->getPhysicalDeviceProperties().limits;

    // All known vendors have power-of-2 alignment requirements, so std::max can work instead of
    // lcm.
    ASSERT(gl::isPow2(limitsVk.minUniformBufferOffsetAlignment));
    ASSERT(gl::isPow2(limitsVk.minStorageBufferOffsetAlignment));
    ASSERT(gl::isPow2(limitsVk.minTexelBufferOffsetAlignment));
    ASSERT(gl::isPow2(limitsVk.minMemoryMapAlignment));

    *alignmentOut = std::max({static_cast<size_t>(limitsVk.minUniformBufferOffsetAlignment),
                              static_cast<size_t>(limitsVk.minStorageBufferOffsetAlignment),
                              static_cast<size_t>(limitsVk.minTexelBufferOffsetAlignment),
                              limitsVk.minMemoryMapAlignment});

    // mBuffer will be allocated through a DynamicBuffer.  If hinted to be DYNAMIC, have
    // DynamicBuffer allocate bigger blocks to suballocate from.  Otherwise, let it adapt to the
    // buffer size automatically (which will allocate BufferHelpers with the same size as this
    // buffer).
    //
    // TODO: fix bugs with suballocation and uncomment the following.  http://anglebug.com/5719
    // const bool isDynamic = usage == gl::BufferUsage::DynamicDraw ||
    //                        usage == gl::BufferUsage::DynamicCopy ||
    //                        usage == gl::BufferUsage::DynamicRead;
    // Sub-allocate from a 4KB buffer.  If the buffer allocations are bigger, the dynamic buffer
    // will adapt to it automatically (and stop sub-allocating).
    //
    // constexpr size_t kDynamicBufferMaxSize = 4 * 1024;
    // const size_t alignedSize               = roundUp(dataSize, *alignmentOut);
    // const size_t suballocationCount        = kDynamicBufferMaxSize / alignedSize;
    // const size_t initialSize               = isDynamic ? alignedSize * suballocationCount : 0;
    const size_t initialSize = 0;

    return initialSize;
}

ANGLE_INLINE bool SubDataSizeMeetsThreshold(size_t subDataSize, size_t bufferSize)
{
    // A sub data update with size > 50% of buffer size meets the threshold
    // to acquire a new BufferHelper from the pool.
    return subDataSize > (bufferSize / 2);
}

ANGLE_INLINE bool IsUsageDynamic(gl::BufferUsage usage)
{
    return (usage == gl::BufferUsage::DynamicDraw || usage == gl::BufferUsage::DynamicCopy ||
            usage == gl::BufferUsage::DynamicRead);
}
}  // namespace

// ConversionBuffer implementation.
ConversionBuffer::ConversionBuffer(RendererVk *renderer,
                                   VkBufferUsageFlags usageFlags,
                                   size_t initialSize,
                                   size_t alignment,
                                   bool hostVisible)
    : dirty(true), lastAllocationOffset(0)
{
    data.init(renderer, usageFlags, alignment, initialSize, hostVisible,
              vk::DynamicBufferPolicy::OneShotUse);
}

ConversionBuffer::~ConversionBuffer() = default;

ConversionBuffer::ConversionBuffer(ConversionBuffer &&other) = default;

// BufferVk::VertexConversionBuffer implementation.
BufferVk::VertexConversionBuffer::VertexConversionBuffer(RendererVk *renderer,
                                                         angle::FormatID formatIDIn,
                                                         GLuint strideIn,
                                                         size_t offsetIn,
                                                         bool hostVisible)
    : ConversionBuffer(renderer,
                       vk::kVertexBufferUsageFlags,
                       kConvertedArrayBufferInitialSize,
                       vk::kVertexBufferAlignment,
                       hostVisible),
      formatID(formatIDIn),
      stride(strideIn),
      offset(offsetIn)
{}

BufferVk::VertexConversionBuffer::VertexConversionBuffer(VertexConversionBuffer &&other) = default;

BufferVk::VertexConversionBuffer::~VertexConversionBuffer() = default;

// BufferVk implementation.
BufferVk::BufferVk(const gl::BufferState &state)
    : BufferImpl(state), mBuffer(nullptr), mBufferOffset(0)
{}

BufferVk::~BufferVk() {}

void BufferVk::destroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    release(contextVk);
}

void BufferVk::release(ContextVk *contextVk)
{
    RendererVk *renderer = contextVk->getRenderer();
    // For external buffers, mBuffer is not a reference to a chunk in mBufferPool.
    // It was allocated explicitly and needs to be deallocated during release(...)
    if (mBuffer && mBuffer->isExternalBuffer())
    {
        mBuffer->release(renderer);
    }
    mShadowBuffer.release();
    mBufferPool.release(renderer);
    mHostVisibleBufferPool.release(renderer);
    mBuffer       = nullptr;
    mBufferOffset = 0;

    for (ConversionBuffer &buffer : mVertexConversionBuffers)
    {
        buffer.data.release(renderer);
    }
}

angle::Result BufferVk::initializeShadowBuffer(ContextVk *contextVk,
                                               gl::BufferBinding target,
                                               size_t size)
{
    if (!contextVk->getRenderer()->getFeatures().shadowBuffers.enabled)
    {
        return angle::Result::Continue;
    }

    // For now, enable shadow buffers only for pixel unpack buffers.
    // If usecases present themselves, we can enable them for other buffer types.
    // Note: If changed, update the waitForIdle message in BufferVk::copySubData to reflect it.
    if (target == gl::BufferBinding::PixelUnpack)
    {
        // Initialize the shadow buffer
        mShadowBuffer.init(size);

        // Allocate required memory. If allocation fails, treat it is a non-fatal error
        // since we do not need the shadow buffer for functionality
        ANGLE_TRY(mShadowBuffer.allocate(size));
    }

    return angle::Result::Continue;
}

void BufferVk::initializeHostVisibleBufferPool(ContextVk *contextVk)
{
    // These buffers will only be used as transfer sources or transfer targets.
    constexpr VkImageUsageFlags kUsageFlags =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // These buffers need to be host visible.
    constexpr VkMemoryPropertyFlags kDeviceLocalHostCoherentFlags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    constexpr size_t kBufferHelperAlignment       = 1;
    constexpr size_t kBufferHelperPoolInitialSize = 0;

    mHostVisibleBufferPool.initWithFlags(
        contextVk->getRenderer(), kUsageFlags, kBufferHelperAlignment, kBufferHelperPoolInitialSize,
        kDeviceLocalHostCoherentFlags, vk::DynamicBufferPolicy::SporadicTextureUpload);
}

void BufferVk::updateShadowBuffer(const uint8_t *data, size_t size, size_t offset)
{
    if (mShadowBuffer.valid())
    {
        mShadowBuffer.updateData(data, size, offset);
    }
}

angle::Result BufferVk::setExternalBufferData(const gl::Context *context,
                                              gl::BufferBinding target,
                                              GLeglClientBufferEXT clientBuffer,
                                              size_t size,
                                              VkMemoryPropertyFlags memoryPropertyFlags)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // Release and re-create the memory and buffer.
    release(contextVk);

    // We could potentially use multiple backing buffers for different usages.
    // For now keep a single buffer with all relevant usage flags.
    VkImageUsageFlags usageFlags =
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

    if (contextVk->getFeatures().supportsTransformFeedbackExtension.enabled)
    {
        usageFlags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    }

    std::unique_ptr<vk::BufferHelper> buffer = std::make_unique<vk::BufferHelper>();

    VkBufferCreateInfo createInfo    = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.flags                 = 0;
    createInfo.size                  = size;
    createInfo.usage                 = usageFlags;
    createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;

    ANGLE_TRY(buffer->initExternal(contextVk, memoryPropertyFlags, createInfo, clientBuffer));

    ASSERT(!mBuffer);
    mBuffer       = buffer.release();
    mBufferOffset = 0;

    return angle::Result::Continue;
}

angle::Result BufferVk::setDataWithUsageFlags(const gl::Context *context,
                                              gl::BufferBinding target,
                                              GLeglClientBufferEXT clientBuffer,
                                              const void *data,
                                              size_t size,
                                              gl::BufferUsage usage,
                                              GLbitfield flags)
{
    VkMemoryPropertyFlags memoryPropertyFlags = 0;
    bool persistentMapRequired                = false;
    const bool isExternalBuffer               = clientBuffer != nullptr;

    switch (usage)
    {
        case gl::BufferUsage::InvalidEnum:
        {
            // glBufferStorage API call
            memoryPropertyFlags   = GetStorageMemoryType(flags, isExternalBuffer);
            persistentMapRequired = (flags & GL_MAP_PERSISTENT_BIT_EXT) != 0;
            break;
        }
        default:
        {
            // glBufferData API call
            memoryPropertyFlags = GetPreferredMemoryType(target, usage);
            break;
        }
    }

    if (isExternalBuffer)
    {
        ANGLE_TRY(setExternalBufferData(context, target, clientBuffer, size, memoryPropertyFlags));
        if (!mBuffer->isHostVisible())
        {
            // If external buffer's memory does not support host visible memory property, we cannot
            // support a persistent map request.
            ANGLE_VK_CHECK(vk::GetImpl(context), !persistentMapRequired,
                           VK_ERROR_MEMORY_MAP_FAILED);

            // Since external buffer is not host visible, allocate a host visible buffer pool
            // to handle map/unmap operations.
            initializeHostVisibleBufferPool(vk::GetImpl(context));
        }

        return angle::Result::Continue;
    }
    return setDataWithMemoryType(context, target, data, size, memoryPropertyFlags,
                                 persistentMapRequired, usage);
}

angle::Result BufferVk::setData(const gl::Context *context,
                                gl::BufferBinding target,
                                const void *data,
                                size_t size,
                                gl::BufferUsage usage)
{
    // Assume host visible/coherent memory available.
    VkMemoryPropertyFlags memoryPropertyFlags = GetPreferredMemoryType(target, usage);
    return setDataWithMemoryType(context, target, data, size, memoryPropertyFlags, false, usage);
}

angle::Result BufferVk::setDataWithMemoryType(const gl::Context *context,
                                              gl::BufferBinding target,
                                              const void *data,
                                              size_t size,
                                              VkMemoryPropertyFlags memoryPropertyFlags,
                                              bool persistentMapRequired,
                                              gl::BufferUsage usage)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    if (size == 0)
    {
        // Nothing to do.
        return angle::Result::Continue;
    }

    // BufferData call is re-specifying the entire buffer
    // Release and init a new mBuffer with this new size
    if (size != static_cast<size_t>(mState.getSize()))
    {
        // Release and re-create the memory and buffer.
        release(contextVk);

        // We could potentially use multiple backing buffers for different usages.
        // For now keep a single buffer with all relevant usage flags.
        VkImageUsageFlags usageFlags =
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

        if (contextVk->getFeatures().supportsTransformFeedbackExtension.enabled)
        {
            usageFlags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
        }

        size_t bufferHelperAlignment = 0;
        const size_t bufferHelperPoolInitialSize =
            GetPreferredDynamicBufferInitialSize(renderer, size, usage, &bufferHelperAlignment);

        mBufferPool.initWithFlags(renderer, usageFlags, bufferHelperAlignment,
                                  bufferHelperPoolInitialSize, memoryPropertyFlags,
                                  vk::DynamicBufferPolicy::FrequentSmallAllocations);

        ANGLE_TRY(acquireBufferHelper(contextVk, size));

        // persistentMapRequired may request that the server read from or write to the buffer while
        // it is mapped. The client's pointer to the data store remains valid so long as the data
        // store is mapped. So it cannot have shadow buffer
        if (!persistentMapRequired)
        {
            // Initialize the shadow buffer
            ANGLE_TRY(initializeShadowBuffer(contextVk, target, size));
        }
    }

    if (data)
    {
        ANGLE_TRY(setDataImpl(contextVk, static_cast<const uint8_t *>(data), size, 0));
    }

    return angle::Result::Continue;
}

angle::Result BufferVk::setSubData(const gl::Context *context,
                                   gl::BufferBinding target,
                                   const void *data,
                                   size_t size,
                                   size_t offset)
{
    ASSERT(mBuffer && mBuffer->valid());

    ContextVk *contextVk = vk::GetImpl(context);
    ANGLE_TRY(setDataImpl(contextVk, static_cast<const uint8_t *>(data), size, offset));

    return angle::Result::Continue;
}

angle::Result BufferVk::copySubData(const gl::Context *context,
                                    BufferImpl *source,
                                    GLintptr sourceOffset,
                                    GLintptr destOffset,
                                    GLsizeiptr size)
{
    ASSERT(mBuffer && mBuffer->valid());

    ContextVk *contextVk            = vk::GetImpl(context);
    BufferVk *sourceVk              = GetAs<BufferVk>(source);
    VkDeviceSize sourceBufferOffset = 0;
    vk::BufferHelper &sourceBuffer  = sourceVk->getBufferAndOffset(&sourceBufferOffset);
    ASSERT(sourceBuffer.valid());

    // If the shadow buffer is enabled for the destination buffer then
    // we need to update that as well. This will require us to complete
    // all recorded and in-flight commands involving the source buffer.
    if (mShadowBuffer.valid())
    {
        // Map the source buffer.
        void *mapPtr;
        ANGLE_TRY(sourceVk->mapRangeImpl(contextVk, sourceOffset, size, 0, &mapPtr));

        // Update the shadow buffer with data from source buffer
        updateShadowBuffer(static_cast<uint8_t *>(mapPtr), size, destOffset);

        // Unmap the source buffer
        ANGLE_TRY(sourceVk->unmapImpl(contextVk));
    }

    // Check for self-dependency.
    vk::CommandBufferAccess access;
    if (sourceBuffer.getBufferSerial() == mBuffer->getBufferSerial())
    {
        access.onBufferSelfCopy(mBuffer);
    }
    else
    {
        access.onBufferTransferRead(&sourceBuffer);
        access.onBufferTransferWrite(mBuffer);
    }

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

    // Enqueue a copy command on the GPU.
    const VkBufferCopy copyRegion = {static_cast<VkDeviceSize>(sourceOffset) + sourceBufferOffset,
                                     static_cast<VkDeviceSize>(destOffset) + mBufferOffset,
                                     static_cast<VkDeviceSize>(size)};

    commandBuffer->copyBuffer(sourceBuffer.getBuffer(), mBuffer->getBuffer(), 1, &copyRegion);

    // The new destination buffer data may require a conversion for the next draw, so mark it dirty.
    onDataChanged();

    return angle::Result::Continue;
}

angle::Result BufferVk::handleDeviceLocalBufferMap(ContextVk *contextVk,
                                                   VkDeviceSize offset,
                                                   VkDeviceSize size,
                                                   void **mapPtr)
{
    // The buffer is device local, create a copy of the buffer and return its CPU pointer.
    bool needToReleasePreviousBuffers = false;
    ANGLE_TRY(mHostVisibleBufferPool.allocate(
        contextVk, static_cast<size_t>(size), reinterpret_cast<uint8_t **>(mapPtr), nullptr,
        &mHostVisibleBufferOffset, &needToReleasePreviousBuffers));
    if (needToReleasePreviousBuffers)
    {
        // Release previous buffers
        mHostVisibleBufferPool.releaseInFlightBuffers(contextVk);
    }

    // Copy data from device local buffer to host visible staging buffer.
    vk::BufferHelper *hostVisibleBuffer = mHostVisibleBufferPool.getCurrentBuffer();
    ASSERT(hostVisibleBuffer && hostVisibleBuffer->valid());

    VkBufferCopy copyRegion = {mBufferOffset + offset, mHostVisibleBufferOffset, size};
    ANGLE_TRY(hostVisibleBuffer->copyFromBuffer(contextVk, mBuffer, 1, &copyRegion));
    ANGLE_TRY(
        hostVisibleBuffer->waitForIdle(contextVk, "GPU stall due to mapping device local buffer"));

    return angle::Result::Continue;
}

angle::Result BufferVk::handleDeviceLocalBufferUnmap(ContextVk *contextVk,
                                                     VkDeviceSize offset,
                                                     VkDeviceSize size)
{
    // Copy data from the host visible buffer into the device local buffer.
    vk::BufferHelper *hostVisibleBuffer = mHostVisibleBufferPool.getCurrentBuffer();
    ASSERT(hostVisibleBuffer && hostVisibleBuffer->valid());

    VkBufferCopy copyRegion = {mHostVisibleBufferOffset, mBufferOffset + offset, size};
    ANGLE_TRY(mBuffer->copyFromBuffer(contextVk, hostVisibleBuffer, 1, &copyRegion));

    return angle::Result::Continue;
}

angle::Result BufferVk::map(const gl::Context *context, GLenum access, void **mapPtr)
{
    ASSERT(mBuffer && mBuffer->valid());

    return mapImpl(vk::GetImpl(context), mapPtr);
}

angle::Result BufferVk::mapRange(const gl::Context *context,
                                 size_t offset,
                                 size_t length,
                                 GLbitfield access,
                                 void **mapPtr)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "BufferVk::mapRange");
    return mapRangeImpl(vk::GetImpl(context), offset, length, access, mapPtr);
}

angle::Result BufferVk::mapImpl(ContextVk *contextVk, void **mapPtr)
{
    return mapRangeImpl(contextVk, 0, static_cast<VkDeviceSize>(mState.getSize()), 0, mapPtr);
}

angle::Result BufferVk::mapRangeImpl(ContextVk *contextVk,
                                     VkDeviceSize offset,
                                     VkDeviceSize length,
                                     GLbitfield access,
                                     void **mapPtr)
{
    if (!mShadowBuffer.valid())
    {
        ASSERT(mBuffer && mBuffer->valid());

        if ((access & GL_MAP_INVALIDATE_BUFFER_BIT) != 0 &&
            mBuffer->isCurrentlyInUse(contextVk->getLastCompletedQueueSerial()) &&
            !mBuffer->isExternalBuffer())
        {
            // We try to map buffer, but buffer is busy. Caller has told us it doesn't care about
            // previous content. Instead of wait for GPU to finish, we just allocate a new buffer.
            RendererVk *renderer = contextVk->getRenderer();
            mBuffer->release(renderer);
            ANGLE_TRY(acquireBufferHelper(contextVk, static_cast<size_t>(mState.getSize())));
        }
        else if ((access & GL_MAP_UNSYNCHRONIZED_BIT) == 0)
        {
            ANGLE_TRY(mBuffer->waitForIdle(contextVk,
                                           "GPU stall due to mapping buffer in use by the GPU"));
        }

        if (mBuffer->isHostVisible())
        {
            // If the buffer is host visible, map and return.
            ANGLE_TRY(mBuffer->mapWithOffset(contextVk, reinterpret_cast<uint8_t **>(mapPtr),
                                             static_cast<size_t>(mBufferOffset + offset)));
        }
        else
        {
            // Handle device local buffers.
            ANGLE_TRY(handleDeviceLocalBufferMap(contextVk, offset, length, mapPtr));
        }
    }
    else
    {
        // If the app requested a GL_MAP_UNSYNCHRONIZED_BIT access, the spec states -
        //      No GL error is generated if pending operations which source or modify the
        //      buffer overlap the mapped region, but the result of such previous and any
        //      subsequent operations is undefined
        // To keep the code simple, irrespective of whether the access was GL_MAP_UNSYNCHRONIZED_BIT
        // or not, just return the shadow buffer.
        mShadowBuffer.map(static_cast<size_t>(offset), mapPtr);
    }

    return angle::Result::Continue;
}

angle::Result BufferVk::unmap(const gl::Context *context, GLboolean *result)
{
    ANGLE_TRY(unmapImpl(vk::GetImpl(context)));

    // This should be false if the contents have been corrupted through external means.  Vulkan
    // doesn't provide such information.
    *result = true;

    return angle::Result::Continue;
}

angle::Result BufferVk::unmapImpl(ContextVk *contextVk)
{
    ASSERT(mBuffer && mBuffer->valid());

    bool writeOperation = ((mState.getAccessFlags() & GL_MAP_WRITE_BIT) != 0);

    if (!mShadowBuffer.valid() && mBuffer->isHostVisible())
    {
        mBuffer->unmap(contextVk->getRenderer());
    }
    else
    {
        size_t offset = static_cast<size_t>(mState.getMapOffset());
        size_t size   = static_cast<size_t>(mState.getMapLength());

        // If it was a write operation we need to update the buffer with new data.
        if (writeOperation)
        {
            if (mShadowBuffer.valid())
            {
                // We do not yet know if this data will ever be used. Perform a staged
                // update which will get flushed if and when necessary.
                const uint8_t *data = getShadowBuffer(offset);
                ANGLE_TRY(stagedUpdate(contextVk, data, size, offset));
                mShadowBuffer.unmap();
            }
            else
            {
                // The buffer is device local.
                ASSERT(!mBuffer->isHostVisible());
                ANGLE_TRY(handleDeviceLocalBufferUnmap(contextVk, offset, size));
            }
        }
    }

    if (writeOperation)
    {
        markConversionBuffersDirty();
    }

    return angle::Result::Continue;
}

angle::Result BufferVk::getSubData(const gl::Context *context,
                                   GLintptr offset,
                                   GLsizeiptr size,
                                   void *outData)
{
    ASSERT(offset + size <= getSize());
    if (!mShadowBuffer.valid())
    {
        ASSERT(mBuffer && mBuffer->valid());
        ContextVk *contextVk = vk::GetImpl(context);
        void *mapPtr;
        ANGLE_TRY(mapRangeImpl(contextVk, offset, size, 0, &mapPtr));
        memcpy(outData, mapPtr, size);
        ANGLE_TRY(unmapImpl(contextVk));
    }
    else
    {
        memcpy(outData, mShadowBuffer.getCurrentBuffer() + offset, size);
    }
    return angle::Result::Continue;
}

angle::Result BufferVk::getIndexRange(const gl::Context *context,
                                      gl::DrawElementsType type,
                                      size_t offset,
                                      size_t count,
                                      bool primitiveRestartEnabled,
                                      gl::IndexRange *outRange)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    // This is a workaround for the mock ICD not implementing buffer memory state.
    // Could be removed if https://github.com/KhronosGroup/Vulkan-Tools/issues/84 is fixed.
    if (renderer->isMockICDEnabled())
    {
        outRange->start = 0;
        outRange->end   = 0;
        return angle::Result::Continue;
    }

    ANGLE_TRACE_EVENT0("gpu.angle", "BufferVk::getIndexRange");

    void *mapPtr;
    ANGLE_TRY(mapRangeImpl(contextVk, offset, getSize(), 0, &mapPtr));
    *outRange = gl::ComputeIndexRange(type, mapPtr, count, primitiveRestartEnabled);
    ANGLE_TRY(unmapImpl(contextVk));

    return angle::Result::Continue;
}

angle::Result BufferVk::updateBuffer(ContextVk *contextVk,
                                     const uint8_t *data,
                                     size_t size,
                                     size_t offset)
{
    if (mBuffer->isHostVisible())
    {
        ANGLE_TRY(directUpdate(contextVk, data, size, offset));
    }
    else
    {
        ANGLE_TRY(stagedUpdate(contextVk, data, size, offset));
    }
    return angle::Result::Continue;
}
angle::Result BufferVk::directUpdate(ContextVk *contextVk,
                                     const uint8_t *data,
                                     size_t size,
                                     size_t offset)
{
    uint8_t *mapPointer = nullptr;

    ANGLE_TRY(mBuffer->mapWithOffset(contextVk, &mapPointer,
                                     static_cast<size_t>(mBufferOffset) + offset));
    ASSERT(mapPointer);

    memcpy(mapPointer, data, size);

    // If the buffer has dynamic usage then the intent is frequent client side updates to the
    // buffer. Don't CPU unmap the buffer, we will take care of unmapping when releasing the buffer
    // to either the renderer or mBufferFreeList.
    if (!IsUsageDynamic(mState.getUsage()))
    {
        mBuffer->unmap(contextVk->getRenderer());
    }
    ASSERT(mBuffer->isCoherent());

    return angle::Result::Continue;
}

angle::Result BufferVk::stagedUpdate(ContextVk *contextVk,
                                     const uint8_t *data,
                                     size_t size,
                                     size_t offset)
{
    // Acquire a "new" staging buffer
    uint8_t *mapPointer              = nullptr;
    VkDeviceSize stagingBufferOffset = 0;

    vk::DynamicBuffer *stagingBuffer = contextVk->getStagingBuffer();
    ANGLE_TRY(stagingBuffer->allocate(contextVk, size, &mapPointer, nullptr, &stagingBufferOffset,
                                      nullptr));
    ASSERT(mapPointer);

    memcpy(mapPointer, data, size);
    ASSERT(!stagingBuffer->isCoherent());
    ANGLE_TRY(stagingBuffer->flush(contextVk));

    // Enqueue a copy command on the GPU.
    VkBufferCopy copyRegion = {stagingBufferOffset, mBufferOffset + offset, size};
    ANGLE_TRY(
        mBuffer->copyFromBuffer(contextVk, stagingBuffer->getCurrentBuffer(), 1, &copyRegion));

    return angle::Result::Continue;
}

angle::Result BufferVk::acquireAndUpdate(ContextVk *contextVk,
                                         const uint8_t *data,
                                         size_t updateSize,
                                         size_t offset)
{
    // Here we acquire a new BufferHelper and directUpdate() the new buffer.
    // If the subData size was less than the buffer's size we additionally enqueue
    // a GPU copy of the remaining regions from the old mBuffer to the new one.
    vk::BufferHelper *src          = mBuffer;
    size_t bufferSize              = static_cast<size_t>(mState.getSize());
    size_t offsetAfterSubdata      = (offset + updateSize);
    bool updateRegionBeforeSubData = (offset > 0);
    bool updateRegionAfterSubData  = (offsetAfterSubdata < bufferSize);

    if (updateRegionBeforeSubData || updateRegionAfterSubData)
    {
        src->retain(&contextVk->getResourceUseList());
    }

    ANGLE_TRY(acquireBufferHelper(contextVk, bufferSize));
    ANGLE_TRY(updateBuffer(contextVk, data, updateSize, offset));

    constexpr int kMaxCopyRegions = 2;
    angle::FixedVector<VkBufferCopy, kMaxCopyRegions> copyRegions;

    if (updateRegionBeforeSubData)
    {
        copyRegions.push_back({0, mBufferOffset, offset});
    }
    if (updateRegionAfterSubData)
    {
        copyRegions.push_back({offsetAfterSubdata, mBufferOffset + offsetAfterSubdata,
                               (bufferSize - offsetAfterSubdata)});
    }

    if (!copyRegions.empty())
    {
        ANGLE_TRY(mBuffer->copyFromBuffer(contextVk, src, static_cast<uint32_t>(copyRegions.size()),
                                          copyRegions.data()));
    }

    return angle::Result::Continue;
}

angle::Result BufferVk::setDataImpl(ContextVk *contextVk,
                                    const uint8_t *data,
                                    size_t size,
                                    size_t offset)
{
    // Update shadow buffer
    updateShadowBuffer(data, size, offset);

    // if the buffer is currently in use
    //     if it isn't an external buffer and sub data size meets threshold
    //          acquire a new BufferHelper from the pool
    //     else stage the update
    // else update the buffer directly
    if (mBuffer->isCurrentlyInUse(contextVk->getLastCompletedQueueSerial()))
    {
        if (!mBuffer->isExternalBuffer() &&
            SubDataSizeMeetsThreshold(size, static_cast<size_t>(mState.getSize())))
        {
            ANGLE_TRY(acquireAndUpdate(contextVk, data, size, offset));
        }
        else
        {
            ANGLE_TRY(stagedUpdate(contextVk, data, size, offset));
        }
    }
    else
    {
        ANGLE_TRY(updateBuffer(contextVk, data, size, offset));
    }

    // Update conversions
    markConversionBuffersDirty();

    return angle::Result::Continue;
}

ConversionBuffer *BufferVk::getVertexConversionBuffer(RendererVk *renderer,
                                                      angle::FormatID formatID,
                                                      GLuint stride,
                                                      size_t offset,
                                                      bool hostVisible)
{
    for (VertexConversionBuffer &buffer : mVertexConversionBuffers)
    {
        if (buffer.formatID == formatID && buffer.stride == stride && buffer.offset == offset)
        {
            return &buffer;
        }
    }

    mVertexConversionBuffers.emplace_back(renderer, formatID, stride, offset, hostVisible);
    return &mVertexConversionBuffers.back();
}

void BufferVk::markConversionBuffersDirty()
{
    for (VertexConversionBuffer &buffer : mVertexConversionBuffers)
    {
        buffer.dirty = true;
    }
}

void BufferVk::onDataChanged()
{
    markConversionBuffersDirty();
}

angle::Result BufferVk::acquireBufferHelper(ContextVk *contextVk, size_t sizeInBytes)
{
    // This method should not be called if it is an ExternalBuffer
    ASSERT(mBuffer == nullptr || mBuffer->isExternalBuffer() == false);

    bool needToReleasePreviousBuffers = false;
    size_t size                       = roundUpPow2(sizeInBytes, kBufferSizeGranularity);

    ANGLE_TRY(mBufferPool.allocate(contextVk, size, nullptr, nullptr, &mBufferOffset,
                                   &needToReleasePreviousBuffers));

    if (needToReleasePreviousBuffers)
    {
        // Release previous buffers
        mBufferPool.releaseInFlightBuffers(contextVk);
    }

    mBuffer = mBufferPool.getCurrentBuffer();
    ASSERT(mBuffer);

    return angle::Result::Continue;
}

}  // namespace rx
