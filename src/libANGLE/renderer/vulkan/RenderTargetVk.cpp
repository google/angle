//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RenderTargetVk:
//   Wrapper around a Vulkan renderable resource, using an ImageView.
//

#include "libANGLE/renderer/vulkan/RenderTargetVk.h"

#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/TextureVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
RenderTargetVk::RenderTargetVk()
    : mImage(nullptr), mImageView(nullptr), mLayerIndex(0), mOwner(nullptr)
{}

RenderTargetVk::~RenderTargetVk() {}

RenderTargetVk::RenderTargetVk(RenderTargetVk &&other)
    : mImage(other.mImage),
      mImageView(other.mImageView),
      mLayerIndex(other.mLayerIndex),
      mOwner(other.mOwner)
{}

void RenderTargetVk::init(vk::ImageHelper *image,
                          vk::ImageView *imageView,
                          size_t layerIndex,
                          TextureVk *owner)
{
    mImage      = image;
    mImageView  = imageView;
    mLayerIndex = layerIndex;
    mOwner      = owner;
}

void RenderTargetVk::reset()
{
    mImage      = nullptr;
    mImageView  = nullptr;
    mLayerIndex = 0;
    mOwner      = nullptr;
}

void RenderTargetVk::onColorDraw(vk::FramebufferHelper *framebufferVk,
                                 vk::CommandBuffer *commandBuffer,
                                 vk::RenderPassDesc *renderPassDesc)
{
    ASSERT(commandBuffer->valid());
    ASSERT(!mImage->getFormat().textureFormat().hasDepthOrStencilBits());

    // Store the attachment info in the renderPassDesc.
    renderPassDesc->packAttachment(mImage->getFormat());

    // TODO(jmadill): Use automatic layout transition. http://anglebug.com/2361
    mImage->changeLayout(VK_IMAGE_ASPECT_COLOR_BIT, vk::ImageLayout::ColorAttachment,
                         commandBuffer);

    // Set up dependencies between the RT resource and the Framebuffer.
    mImage->addWriteDependency(framebufferVk);
}

void RenderTargetVk::onDepthStencilDraw(vk::FramebufferHelper *framebufferVk,
                                        vk::CommandBuffer *commandBuffer,
                                        vk::RenderPassDesc *renderPassDesc)
{
    ASSERT(commandBuffer->valid());
    ASSERT(mImage->getFormat().textureFormat().hasDepthOrStencilBits());

    // Store the attachment info in the renderPassDesc.
    renderPassDesc->packAttachment(mImage->getFormat());

    // TODO(jmadill): Use automatic layout transition. http://anglebug.com/2361
    const angle::Format &format    = mImage->getFormat().textureFormat();
    VkImageAspectFlags aspectFlags = vk::GetDepthStencilAspectFlags(format);

    mImage->changeLayout(aspectFlags, vk::ImageLayout::DepthStencilAttachment, commandBuffer);

    // Set up dependencies between the RT resource and the Framebuffer.
    mImage->addWriteDependency(framebufferVk);
}

vk::ImageHelper &RenderTargetVk::getImage()
{
    ASSERT(mImage && mImage->valid());
    return *mImage;
}

const vk::ImageHelper &RenderTargetVk::getImage() const
{
    ASSERT(mImage && mImage->valid());
    return *mImage;
}

vk::ImageView *RenderTargetVk::getDrawImageView() const
{
    ASSERT(mImageView && mImageView->valid());
    return mImageView;
}

vk::ImageView *RenderTargetVk::getReadImageView() const
{
    return getDrawImageView();
}

const vk::Format &RenderTargetVk::getImageFormat() const
{
    ASSERT(mImage && mImage->valid());
    return mImage->getFormat();
}

const gl::Extents &RenderTargetVk::getImageExtents() const
{
    ASSERT(mImage && mImage->valid());
    return mImage->getExtents();
}

void RenderTargetVk::updateSwapchainImage(vk::ImageHelper *image, vk::ImageView *imageView)
{
    ASSERT(image && image->valid() && imageView && imageView->valid());
    mImage     = image;
    mImageView = imageView;
    mOwner     = nullptr;
}

vk::ImageHelper *RenderTargetVk::getImageForRead(vk::CommandGraphResource *readingResource,
                                                 vk::ImageLayout layout,
                                                 vk::CommandBuffer *commandBuffer)
{
    ASSERT(mImage && mImage->valid());

    // TODO(jmadill): Better simultaneous resource access. http://anglebug.com/2679
    mImage->addWriteDependency(readingResource);

    mImage->changeLayout(mImage->getAspectFlags(), layout, commandBuffer);

    return mImage;
}

vk::ImageHelper *RenderTargetVk::getImageForWrite(vk::CommandGraphResource *writingResource) const
{
    ASSERT(mImage && mImage->valid());
    mImage->addWriteDependency(writingResource);
    return mImage;
}

angle::Result RenderTargetVk::ensureImageInitialized(ContextVk *contextVk)
{
    if (mOwner)
    {
        return mOwner->ensureImageInitialized(contextVk);
    }
    return angle::Result::Continue;
}

}  // namespace rx
