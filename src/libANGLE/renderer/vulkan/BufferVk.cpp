//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BufferVk.cpp:
//    Implements the class methods for BufferVk.
//

#include "libANGLE/renderer/vulkan/BufferVk.h"

#include "common/debug.h"
#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{

BufferVk::BufferVk(const gl::BufferState &state) : BufferImpl(state), mCurrentRequiredSize(0)
{
}

BufferVk::~BufferVk()
{
}

void BufferVk::destroy(const gl::Context *context)
{
    ContextVk *contextVk = GetImplAs<ContextVk>(context);
    RendererVk *renderer = contextVk->getRenderer();

    renderer->releaseResource(*this, &mBuffer);
    renderer->releaseResource(*this, &mBufferMemory);
}

gl::Error BufferVk::setData(const gl::Context *context,
                            GLenum target,
                            const void *data,
                            size_t size,
                            gl::BufferUsage usage)
{
    ContextVk *contextVk = GetImplAs<ContextVk>(context);
    auto device          = contextVk->getDevice();

    if (size > mCurrentRequiredSize)
    {
        // TODO(jmadill): Proper usage bit implementation. Likely will involve multiple backing
        // buffers like in D3D11.
        VkBufferCreateInfo createInfo;
        createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.pNext                 = nullptr;
        createInfo.flags                 = 0;
        createInfo.size                  = size;
        createInfo.usage                 = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;

        vk::Buffer newBuffer;
        ANGLE_TRY(newBuffer.init(device, createInfo));
        mBuffer.retain(device, std::move(newBuffer));

        ANGLE_TRY(vk::AllocateBufferMemory(contextVk, size, &mBuffer, &mBufferMemory,
                                           &mCurrentRequiredSize));
    }

    if (data)
    {
        ANGLE_TRY(setDataImpl(device, static_cast<const uint8_t *>(data), size, 0));
    }

    return gl::NoError();
}

gl::Error BufferVk::setSubData(const gl::Context *context,
                               GLenum target,
                               const void *data,
                               size_t size,
                               size_t offset)
{
    ASSERT(mBuffer.getHandle() != VK_NULL_HANDLE);
    ASSERT(mBufferMemory.getHandle() != VK_NULL_HANDLE);

    VkDevice device = GetImplAs<ContextVk>(context)->getDevice();

    ANGLE_TRY(setDataImpl(device, static_cast<const uint8_t *>(data), size, offset));

    return gl::NoError();
}

gl::Error BufferVk::copySubData(const gl::Context *context,
                                BufferImpl *source,
                                GLintptr sourceOffset,
                                GLintptr destOffset,
                                GLsizeiptr size)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error BufferVk::map(const gl::Context *context, GLenum access, void **mapPtr)
{
    ASSERT(mBuffer.getHandle() != VK_NULL_HANDLE);
    ASSERT(mBufferMemory.getHandle() != VK_NULL_HANDLE);

    VkDevice device = GetImplAs<ContextVk>(context)->getDevice();

    ANGLE_TRY(
        mBufferMemory.map(device, 0, mState.getSize(), 0, reinterpret_cast<uint8_t **>(mapPtr)));

    return gl::NoError();
}

gl::Error BufferVk::mapRange(const gl::Context *context,
                             size_t offset,
                             size_t length,
                             GLbitfield access,
                             void **mapPtr)
{
    ASSERT(mBuffer.getHandle() != VK_NULL_HANDLE);
    ASSERT(mBufferMemory.getHandle() != VK_NULL_HANDLE);

    VkDevice device = GetImplAs<ContextVk>(context)->getDevice();

    ANGLE_TRY(mBufferMemory.map(device, offset, length, 0, reinterpret_cast<uint8_t **>(mapPtr)));

    return gl::NoError();
}

gl::Error BufferVk::unmap(const gl::Context *context, GLboolean *result)
{
    ASSERT(mBuffer.getHandle() != VK_NULL_HANDLE);
    ASSERT(mBufferMemory.getHandle() != VK_NULL_HANDLE);

    VkDevice device = GetImplAs<ContextVk>(context)->getDevice();

    mBufferMemory.unmap(device);

    return gl::NoError();
}

gl::Error BufferVk::getIndexRange(const gl::Context *context,
                                  GLenum type,
                                  size_t offset,
                                  size_t count,
                                  bool primitiveRestartEnabled,
                                  gl::IndexRange *outRange)
{
    VkDevice device = GetImplAs<ContextVk>(context)->getDevice();

    // TODO(jmadill): Consider keeping a shadow system memory copy in some cases.
    ASSERT(mBuffer.valid());

    const gl::Type &typeInfo = gl::GetTypeInfo(type);

    uint8_t *mapPointer = nullptr;
    ANGLE_TRY(mBufferMemory.map(device, offset, typeInfo.bytes * count, 0, &mapPointer));

    *outRange = gl::ComputeIndexRange(type, mapPointer, count, primitiveRestartEnabled);

    return gl::NoError();
}

vk::Error BufferVk::setDataImpl(VkDevice device, const uint8_t *data, size_t size, size_t offset)
{
    uint8_t *mapPointer = nullptr;
    ANGLE_TRY(mBufferMemory.map(device, offset, size, 0, &mapPointer));
    ASSERT(mapPointer);

    memcpy(mapPointer, data, size);

    mBufferMemory.unmap(device);

    return vk::NoError();
}

const vk::Buffer &BufferVk::getVkBuffer() const
{
    return mBuffer;
}

}  // namespace rx
