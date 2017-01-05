//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// StreamingBuffer:
//    Create, map and flush buffers as needed to hold data, returning a handle and offset for each
//    chunk.
//

#ifndef LIBANGLE_RENDERER_VULKAN_STREAMING_BUFFER_H_
#define LIBANGLE_RENDERER_VULKAN_STREAMING_BUFFER_H_

#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace rx
{

class StreamingBuffer : public ResourceVk
{
  public:
    StreamingBuffer(VkBufferUsageFlags usage, size_t minSize);
    ~StreamingBuffer();
    gl::Error allocate(ContextVk *context,
                       size_t amount,
                       uint8_t **ptrOut,
                       VkBuffer *handleOut,
                       VkDeviceSize *offsetOut);
    gl::Error flush(ContextVk *context);
    void destroy(VkDevice device);

  private:
    VkBufferUsageFlags mUsage;
    size_t mMinSize;
    vk::Buffer mBuffer;
    vk::DeviceMemory mMemory;
    size_t mNextWriteOffset;
    size_t mLastFlushOffset;
    size_t mSize;
    uint8_t *mMappedMemory;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_STREAMING_BUFFER_H_
