//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RenderTargetVk:
//   Wrapper around a Vulkan renderable resource, using an ImageView.
//

#ifndef LIBANGLE_RENDERER_VULKAN_RENDERTARGETVK_H_
#define LIBANGLE_RENDERER_VULKAN_RENDERTARGETVK_H_

#include <vulkan/vulkan.h>

#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/renderer/renderer_utils.h"

namespace rx
{
namespace vk
{
class CommandBuffer;
struct Format;
class FramebufferHelper;
class ImageHelper;
class ImageView;
class RecordableGraphResource;
class RenderPassDesc;
}  // namespace vk

// This is a very light-weight class that does not own to the resources it points to.
// It's meant only to copy across some information from a FramebufferAttachment to the
// business rendering logic. It stores Images and ImageView by pointer for performance.
class RenderTargetVk final : public FramebufferAttachmentRenderTarget
{
  public:
    RenderTargetVk(vk::ImageHelper *image,
                   vk::ImageView *imageView,
                   size_t layerIndex);
    ~RenderTargetVk();

    // Used in std::vector initialization.
    RenderTargetVk(RenderTargetVk &&other);

    // Note: RenderTargets should be called in order, with the depth/stencil onRender last.
    void onColorDraw(vk::FramebufferHelper *framebufferVk,
                     vk::CommandBuffer *commandBuffer,
                     vk::RenderPassDesc *renderPassDesc);
    void onDepthStencilDraw(vk::FramebufferHelper *framebufferVk,
                            vk::CommandBuffer *commandBuffer,
                            vk::RenderPassDesc *renderPassDesc);

    const vk::ImageHelper &getImage() const;

    // getImageForRead will also transition the resource to the given layout.
    vk::ImageHelper *getImageForRead(vk::RecordableGraphResource *readingResource,
                                     VkImageLayout layout,
                                     vk::CommandBuffer *commandBuffer);
    vk::ImageHelper *getImageForWrite(vk::RecordableGraphResource *writingResource) const;
    vk::ImageView *getImageView() const;

    const vk::Format &getImageFormat() const;
    const gl::Extents &getImageExtents() const;
    size_t getLayerIndex() const { return mLayerIndex; }

    // Special mutator for Surface RenderTargets. Allows the Framebuffer to keep a single
    // RenderTargetVk pointer.
    void updateSwapchainImage(vk::ImageHelper *image, vk::ImageView *imageView);

  private:
    vk::ImageHelper *mImage;
    vk::ImageView *mImageView;
    size_t mLayerIndex;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERTARGETVK_H_
