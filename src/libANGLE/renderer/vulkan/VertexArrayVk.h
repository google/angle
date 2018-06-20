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
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace gl
{
class DrawCallParams;
}  // namespace gl

namespace rx
{
class BufferVk;

namespace vk
{
class CommandGraphResource;
class DynamicBuffer;
}  // namespace vk

class VertexArrayVk : public VertexArrayImpl
{
  public:
    VertexArrayVk(const gl::VertexArrayState &state, RendererVk *renderer);
    ~VertexArrayVk() override;

    void destroy(const gl::Context *context) override;

    gl::Error syncState(const gl::Context *context,
                        const gl::VertexArray::DirtyBits &dirtyBits,
                        const gl::VertexArray::DirtyAttribBitsArray &attribBits,
                        const gl::VertexArray::DirtyBindingBitsArray &bindingBits) override;

    const gl::AttribArray<VkBuffer> &getCurrentArrayBufferHandles() const;
    const gl::AttribArray<VkDeviceSize> &getCurrentArrayBufferOffsets() const;

    void updateDrawDependencies(vk::CommandGraphResource *drawFramebuffer,
                                const gl::AttributesMask &activeAttribsMask,
                                vk::CommandGraphResource *elementArrayBufferOverride,
                                Serial serial,
                                bool isDrawElements);

    void getPackedInputDescriptions(const RendererVk *rendererVk, vk::PipelineDesc *pipelineDesc);

    // Draw call handling.
    gl::Error drawArrays(const gl::Context *context,
                         RendererVk *renderer,
                         const gl::DrawCallParams &drawCallParams,
                         vk::CommandBuffer *commandBuffer,
                         bool shouldApplyVertexArray);
    gl::Error drawElements(const gl::Context *context,
                           RendererVk *renderer,
                           const gl::DrawCallParams &drawCallParams,
                           vk::CommandBuffer *commandBuffer,
                           bool shouldApplyVertexArray);

  private:
    // This will update any dirty packed input descriptions, regardless if they're used by the
    // active program. This could lead to slight inefficiencies when the app would repeatedly
    // update vertex info for attributes the program doesn't use, (very silly edge case). The
    // advantage is the cached state then doesn't depend on the Program, so doesn't have to be
    // updated when the active Program changes.
    void updatePackedInputDescriptions(const RendererVk *rendererVk);
    void updatePackedInputInfo(const RendererVk *rendererVk,
                               uint32_t attribIndex,
                               const gl::VertexBinding &binding,
                               const gl::VertexAttribute &attrib);

    void updateArrayBufferReadDependencies(vk::CommandGraphResource *drawFramebuffer,
                                           const gl::AttributesMask &activeAttribsMask,
                                           Serial serial);

    void updateElementArrayBufferReadDependency(vk::CommandGraphResource *drawFramebuffer,
                                                Serial serial);

    gl::Error streamVertexData(RendererVk *renderer,
                               const gl::AttributesMask &attribsToStream,
                               const gl::DrawCallParams &drawCallParams);

    gl::Error streamIndexData(RendererVk *renderer, const gl::DrawCallParams &drawCallParams);

    gl::Error onDraw(const gl::Context *context,
                     RendererVk *renderer,
                     const gl::DrawCallParams &drawCallParams,
                     vk::CommandBuffer *commandBuffer,
                     bool newCommandBuffer);
    gl::Error onIndexedDraw(const gl::Context *context,
                            RendererVk *renderer,
                            const gl::DrawCallParams &drawCallParams,
                            vk::CommandBuffer *commandBuffer,
                            bool newCommandBuffer);

    void syncDirtyAttrib(const gl::VertexAttribute &attrib,
                         const gl::VertexBinding &binding,
                         size_t attribIndex);

    gl::AttribArray<VkBuffer> mCurrentArrayBufferHandles;
    gl::AttribArray<VkDeviceSize> mCurrentArrayBufferOffsets;
    gl::AttribArray<vk::CommandGraphResource *> mCurrentArrayBufferResources;
    VkBuffer mCurrentElementArrayBufferHandle;
    VkDeviceSize mCurrentElementArrayBufferOffset;
    vk::CommandGraphResource *mCurrentElementArrayBufferResource;

    // Keep a cache of binding and attribute descriptions for easy pipeline updates.
    // This is copied out of here into the pipeline description on a Context state change.
    gl::AttributesMask mDirtyPackedInputs;
    vk::VertexInputBindings mPackedInputBindings;
    vk::VertexInputAttributes mPackedInputAttributes;

    vk::DynamicBuffer mDynamicVertexData;
    vk::DynamicBuffer mDynamicIndexData;
    vk::DynamicBuffer mTranslatedByteIndexData;

    vk::LineLoopHelper mLineLoopHelper;
    Optional<GLint> mLineLoopBufferFirstIndex;
    Optional<size_t> mLineLoopBufferLastIndex;
    bool mDirtyLineLoopTranslation;

    // Cache variable for determining whether or not to store new dependencies in the node.
    bool mVertexBuffersDirty;
    bool mIndexBufferDirty;

    // The offset we had the last time we bound the index buffer.
    uintptr_t mLastIndexBufferOffset;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VERTEXARRAYVK_H_
