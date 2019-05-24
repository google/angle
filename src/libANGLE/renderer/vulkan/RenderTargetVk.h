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
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
namespace vk
{
struct Format;
class FramebufferHelper;
class ImageHelper;
class ImageView;
class CommandGraphResource;
class RenderPassDesc;
}  // namespace vk

class ContextVk;
class TextureVk;

// This is a very light-weight class that does not own to the resources it points to.
// It's meant only to copy across some information from a FramebufferAttachment to the
// business rendering logic. It stores Images and ImageViews by pointer for performance.
class RenderTargetVk final : public FramebufferAttachmentRenderTarget
{
  public:
    RenderTargetVk();
    ~RenderTargetVk() override;

    // Used in std::vector initialization.
    RenderTargetVk(RenderTargetVk &&other);

    void init(vk::ImageHelper *image,
              vk::ImageView *imageView,
              vk::ImageView *cubeImageFetchView,
              size_t levelIndex,
              size_t layerIndex);
    void reset();

    // Note: RenderTargets should be called in order, with the depth/stencil onRender last.
    angle::Result onColorDraw(ContextVk *contextVk,
                              vk::FramebufferHelper *framebufferVk,
                              vk::CommandBuffer *commandBuffer);
    angle::Result onDepthStencilDraw(ContextVk *contextVk,
                                     vk::FramebufferHelper *framebufferVk,
                                     vk::CommandBuffer *commandBuffer);

    vk::ImageHelper &getImage();
    const vk::ImageHelper &getImage() const;

    // getImageForRead will also transition the resource to the given layout.
    vk::ImageHelper *getImageForRead(vk::CommandGraphResource *readingResource,
                                     vk::ImageLayout layout,
                                     vk::CommandBuffer *commandBuffer);
    vk::ImageHelper *getImageForWrite(vk::CommandGraphResource *writingResource) const;

    vk::ImageView *getDrawImageView() const;
    vk::ImageView *getReadImageView() const;
    // GLSL's texelFetch() needs a 2D array view to read from cube maps.  This function returns the
    // same view as `getReadImageView()`, except for cubemaps, in which case it returns a 2D array
    // view of it.
    vk::ImageView *getFetchImageView() const;

    const vk::Format &getImageFormat() const;
    gl::Extents getExtents() const;
    size_t getLevelIndex() const { return mLevelIndex; }
    size_t getLayerIndex() const { return mLayerIndex; }

    // Special mutator for Surface RenderTargets. Allows the Framebuffer to keep a single
    // RenderTargetVk pointer.
    void updateSwapchainImage(vk::ImageHelper *image, vk::ImageView *imageView);

    angle::Result flushStagedUpdates(ContextVk *contextVk);

  private:
    vk::ImageHelper *mImage;
    // Note that the draw and read image views are the same, given the requirements of a render
    // target.
    vk::ImageView *mImageView;
    // For cubemaps, a 2D-array view is also created to be used with shaders that use texelFetch().
    vk::ImageView *mCubeImageFetchView;
    size_t mLevelIndex;
    size_t mLayerIndex;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERTARGETVK_H_
