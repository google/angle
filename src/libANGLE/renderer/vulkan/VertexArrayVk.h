//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VertexArrayVk.h:
//    Defines the class interface for VertexArrayVk, implementing VertexArrayImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_VERTEXARRAYVK_H_
#define LIBANGLE_RENDERER_VULKAN_VERTEXARRAYVK_H_

#include "libANGLE/renderer/VertexArrayImpl.h"
#include "libANGLE/renderer/vulkan/vk_cache_utils.h"

namespace rx
{
class BufferVk;
class StreamingBuffer;

class VertexArrayVk : public VertexArrayImpl
{
  public:
    VertexArrayVk(const gl::VertexArrayState &state);
    ~VertexArrayVk() override;

    void destroy(const gl::Context *context) override;

    gl::Error streamVertexData(ContextVk *context,
                               StreamingBuffer *stream,
                               int firstVertex,
                               int lastVertex);
    void syncState(const gl::Context *context,
                   const gl::VertexArray::DirtyBits &dirtyBits) override;

    const gl::AttribArray<VkBuffer> &getCurrentArrayBufferHandles() const;
    const gl::AttribArray<VkDeviceSize> &getCurrentArrayBufferOffsets() const;

    void updateDrawDependencies(vk::CommandBufferNode *readNode,
                                const gl::AttributesMask &activeAttribsMask,
                                Serial serial,
                                DrawType drawType);

    void getPackedInputDescriptions(vk::PipelineDesc *pipelineDesc);

  private:
    // This will update any dirty packed input descriptions, regardless if they're used by the
    // active program. This could lead to slight inefficiencies when the app would repeatedly
    // update vertex info for attributes the program doesn't use, (very silly edge case). The
    // advantage is the cached state then doesn't depend on the Program, so doesn't have to be
    // updated when the active Program changes.
    void updatePackedInputDescriptions();
    void updatePackedInputInfo(uint32_t attribIndex,
                               const gl::VertexBinding &binding,
                               const gl::VertexAttribute &attrib);

    gl::AttribArray<VkBuffer> mCurrentArrayBufferHandles;
    gl::AttribArray<VkDeviceSize> mCurrentArrayBufferOffsets;
    gl::AttribArray<ResourceVk *> mCurrentArrayBufferResources;
    ResourceVk *mCurrentElementArrayBufferResource;

    // Keep a cache of binding and attribute descriptions for easy pipeline updates.
    // This is copied out of here into the pipeline description on a Context state change.
    gl::AttributesMask mDirtyPackedInputs;
    vk::VertexInputBindings mPackedInputBindings;
    vk::VertexInputAttributes mPackedInputAttributes;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VERTEXARRAYVK_H_
