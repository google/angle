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
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
RenderTargetVk::RenderTargetVk(vk::ImageHelper *image,
                               vk::ImageView *imageView,
                               vk::CommandGraphResource *resource)
    : mImage(image), mImageView(imageView), mResource(resource)
{
}

RenderTargetVk::~RenderTargetVk()
{
}

void RenderTargetVk::onColorDraw(Serial currentSerial,
                                 vk::CommandGraphNode *writingNode,
                                 vk::RenderPassDesc *renderPassDesc)
{
    ASSERT(writingNode->getOutsideRenderPassCommands()->valid());

    // Store the attachment info in the renderPassDesc.
    renderPassDesc->packColorAttachment(*mImage);

    // TODO(jmadill): Use automatic layout transition. http://anglebug.com/2361
    mImage->changeLayoutWithStages(
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        writingNode->getOutsideRenderPassCommands());

    // Set up dependencies between the new graph node and other current nodes in the resource.
    mResource->onWriteResource(writingNode, currentSerial);
}

void RenderTargetVk::onDepthStencilDraw(Serial currentSerial,
                                        vk::CommandGraphNode *writingNode,
                                        vk::RenderPassDesc *renderPassDesc)
{
    ASSERT(writingNode->getOutsideRenderPassCommands()->valid());
    ASSERT(mImage->getFormat().textureFormat().hasDepthOrStencilBits());

    // Store the attachment info in the renderPassDesc.
    renderPassDesc->packDepthStencilAttachment(*mImage);

    // TODO(jmadill): Use automatic layout transition. http://anglebug.com/2361
    const angle::Format &format    = mImage->getFormat().textureFormat();
    VkImageAspectFlags aspectFlags = (format.depthBits > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
                                     (format.stencilBits > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

    mImage->changeLayoutWithStages(aspectFlags, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                   VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                   VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                   writingNode->getOutsideRenderPassCommands());

    // Set up dependencies between the new graph node and other current nodes in the resource.
    mResource->onWriteResource(writingNode, currentSerial);
}

const vk::ImageHelper &RenderTargetVk::getImage() const
{
    ASSERT(mImage && mImage->valid());
    return *mImage;
}

vk::ImageView *RenderTargetVk::getImageView() const
{
    ASSERT(mImageView && mImageView->valid());
    return mImageView;
}

vk::CommandGraphResource *RenderTargetVk::getResource() const
{
    return mResource;
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
}

vk::ImageHelper *RenderTargetVk::getImageForWrite(Serial currentSerial,
                                                  vk::CommandGraphNode *writingNode) const
{
    ASSERT(mImage && mImage->valid());
    mResource->onWriteResource(writingNode, currentSerial);
    return mImage;
}

}  // namespace rx
