//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DynamicBuffer:
//    Create, map and flush buffers as needed to hold data, returning a handle and offset for each
//    chunk.
//

#include "libANGLE/renderer/vulkan/DynamicBuffer.h"

#include "anglebase/numerics/safe_math.h"

#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{
DynamicBuffer::DynamicBuffer(VkBufferUsageFlags usage, size_t minSize)
    : mUsage(usage),
      mMinSize(minSize),
      mNextWriteOffset(0),
      mLastFlushOffset(0),
      mSize(0),
      mAlignment(0),
      mMappedMemory(nullptr)
{
}

void DynamicBuffer::init(size_t alignment)
{
    ASSERT(alignment > 0);
    mAlignment = alignment;
}

DynamicBuffer::~DynamicBuffer()
{
    ASSERT(mAlignment == 0);
}

bool DynamicBuffer::valid()
{
    return mAlignment > 0;
}

vk::Error DynamicBuffer::allocate(RendererVk *renderer,
                                  size_t sizeInBytes,
                                  uint8_t **ptrOut,
                                  VkBuffer *handleOut,
                                  uint32_t *offsetOut,
                                  bool *outNewBufferAllocated)
{
    ASSERT(valid());
    VkDevice device = renderer->getDevice();

    size_t sizeToAllocate = roundUp(sizeInBytes, mAlignment);

    angle::base::CheckedNumeric<size_t> checkedNextWriteOffset = mNextWriteOffset;
    checkedNextWriteOffset += sizeToAllocate;

    if (!checkedNextWriteOffset.IsValid() || checkedNextWriteOffset.ValueOrDie() > mSize)
    {
        if (mMappedMemory)
        {
            ANGLE_TRY(flush(device));
            mMemory.unmap(device);
            mMappedMemory = nullptr;
        }
        Serial currentSerial = renderer->getCurrentQueueSerial();
        renderer->releaseObject(currentSerial, &mBuffer);
        renderer->releaseObject(currentSerial, &mMemory);

        VkBufferCreateInfo createInfo;
        createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.pNext                 = nullptr;
        createInfo.flags                 = 0;
        createInfo.size                  = std::max(sizeToAllocate, mMinSize);
        createInfo.usage                 = mUsage;
        createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
        ANGLE_TRY(mBuffer.init(device, createInfo));

        ANGLE_TRY(vk::AllocateBufferMemory(renderer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &mBuffer,
                                           &mMemory, &mSize));
        ANGLE_TRY(mMemory.map(device, 0, mSize, 0, &mMappedMemory));
        mNextWriteOffset = 0;
        mLastFlushOffset = 0;

        if (outNewBufferAllocated != nullptr)
        {
            *outNewBufferAllocated = true;
        }
    }
    else
    {
        if (outNewBufferAllocated != nullptr)
        {
            *outNewBufferAllocated = false;
        }
    }

    ASSERT(mBuffer.valid());

    if (handleOut != nullptr)
    {
        *handleOut = mBuffer.getHandle();
    }

    ASSERT(mMappedMemory);
    *ptrOut    = mMappedMemory + mNextWriteOffset;
    *offsetOut = mNextWriteOffset;
    mNextWriteOffset += static_cast<uint32_t>(sizeToAllocate);

    return vk::NoError();
}

vk::Error DynamicBuffer::flush(VkDevice device)
{
    if (mNextWriteOffset > mLastFlushOffset)
    {
        VkMappedMemoryRange range;
        range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.pNext  = nullptr;
        range.memory = mMemory.getHandle();
        range.offset = mLastFlushOffset;
        range.size   = mNextWriteOffset - mLastFlushOffset;
        ANGLE_VK_TRY(vkFlushMappedMemoryRanges(device, 1, &range));

        mLastFlushOffset = mNextWriteOffset;
    }
    return vk::NoError();
}

void DynamicBuffer::destroy(VkDevice device)
{
    mAlignment = 0;
    mBuffer.destroy(device);
    mMemory.destroy(device);
}

VkBuffer DynamicBuffer::getCurrentBufferHandle() const
{
    return mBuffer.getHandle();
}

void DynamicBuffer::setMinimumSize(size_t minSize)
{
    // This will really only have an effect next time we call allocate.
    mMinSize = minSize;

    // Forces a new allocation on the next allocate.
    mSize = 0;
}
}  // namespace rx
