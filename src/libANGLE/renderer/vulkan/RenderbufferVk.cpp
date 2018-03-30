//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RenderbufferVk.cpp:
//    Implements the class methods for RenderbufferVk.
//

#include "libANGLE/renderer/vulkan/RenderbufferVk.h"

#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{

namespace
{
constexpr VkClearDepthStencilValue kDefaultClearDepthStencilValue = {0.0f, 1};
constexpr VkClearColorValue kBlackClearColorValue                 = {{0}};

}  // anonymous namespace

RenderbufferVk::RenderbufferVk(const gl::RenderbufferState &state) : RenderbufferImpl(state)
{
    mRenderTarget.image     = &mImage.getImage();
    mRenderTarget.imageView = &mImageView;
    mRenderTarget.resource  = this;
}

RenderbufferVk::~RenderbufferVk()
{
}

gl::Error RenderbufferVk::onDestroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    mImage.release(renderer->getCurrentQueueSerial(), renderer);
    renderer->releaseResource(*this, &mImageView);

    onStateChange(context, angle::SubjectMessage::DEPENDENT_DIRTY_BITS);

    return gl::NoError();
}

gl::Error RenderbufferVk::setStorage(const gl::Context *context,
                                     GLenum internalformat,
                                     size_t width,
                                     size_t height)
{
    ContextVk *contextVk       = vk::GetImpl(context);
    RendererVk *renderer       = contextVk->getRenderer();
    const vk::Format &vkFormat = renderer->getFormat(internalformat);
    VkDevice device            = renderer->getDevice();

    if (mImage.valid())
    {
        // Check against the state if we need to recreate the storage.
        if (internalformat != mState.getFormat().info->internalFormat ||
            static_cast<GLsizei>(width) != mState.getWidth() ||
            static_cast<GLsizei>(height) != mState.getHeight())
        {
            mImage.release(renderer->getCurrentQueueSerial(), renderer);
            renderer->releaseResource(*this, &mImageView);
            onStateChange(context, angle::SubjectMessage::DEPENDENT_DIRTY_BITS);
        }
    }

    // Init RenderTarget.
    mRenderTarget.extents.width  = static_cast<int>(width);
    mRenderTarget.extents.height = static_cast<int>(height);
    mRenderTarget.extents.depth  = 1;
    mRenderTarget.format         = &vkFormat;
    mRenderTarget.samples        = VK_SAMPLE_COUNT_1_BIT;  // TODO(jmadill): Multisample bits.

    if (!mImage.valid() && (width != 0 || height != 0))
    {
        const angle::Format &textureFormat = vkFormat.textureFormat();
        bool isDepthOrStencilFormat = textureFormat.depthBits > 0 || textureFormat.stencilBits > 0;
        const VkImageUsageFlags usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            (textureFormat.redBits > 0 ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0) |
            (isDepthOrStencilFormat ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0);

        ANGLE_TRY(mImage.init2D(device, mRenderTarget.extents, vkFormat, 1, usage));

        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        ANGLE_TRY(mImage.initMemory(device, renderer->getMemoryProperties(), flags));

        VkImageAspectFlags aspect =
            (textureFormat.depthBits > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            (textureFormat.stencilBits > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0) |
            (textureFormat.redBits > 0 ? VK_IMAGE_ASPECT_COLOR_BIT : 0);

        ANGLE_TRY(mImage.initImageView(device, aspect, gl::SwizzleState(), &mImageView));

        // TODO(jmadill): Fold this into the RenderPass load/store ops. http://anglebug.com/2361
        vk::CommandBuffer *commandBuffer = nullptr;
        ANGLE_TRY(beginWriteResource(renderer, &commandBuffer));

        mImage.getImage().changeLayoutWithStages(
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandBuffer);

        if (isDepthOrStencilFormat)
        {
            commandBuffer->clearSingleDepthStencilImage(mImage.getImage(), aspect,
                                                        kDefaultClearDepthStencilValue);
        }
        else
        {
            commandBuffer->clearSingleColorImage(mImage.getImage(), kBlackClearColorValue);
        }
    }

    return gl::NoError();
}

gl::Error RenderbufferVk::setStorageMultisample(const gl::Context *context,
                                                size_t samples,
                                                GLenum internalformat,
                                                size_t width,
                                                size_t height)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error RenderbufferVk::setStorageEGLImageTarget(const gl::Context *context, egl::Image *image)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error RenderbufferVk::getAttachmentRenderTarget(const gl::Context * /*context*/,
                                                    GLenum /*binding*/,
                                                    const gl::ImageIndex & /*imageIndex*/,
                                                    FramebufferAttachmentRenderTarget **rtOut)
{
    ASSERT(mImage.valid());
    *rtOut = &mRenderTarget;
    return gl::NoError();
}

gl::Error RenderbufferVk::initializeContents(const gl::Context *context,
                                             const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return gl::NoError();
}

}  // namespace rx
