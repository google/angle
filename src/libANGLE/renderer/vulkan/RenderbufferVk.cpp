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

RenderbufferVk::RenderbufferVk(const gl::RenderbufferState &state)
    : RenderbufferImpl(state), mRenderTarget(&mImage, &mImageView, 0)
{
}

RenderbufferVk::~RenderbufferVk()
{
}

void RenderbufferVk::onDestroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    mImage.release(renderer);
    renderer->releaseObject(renderer->getCurrentQueueSerial(), &mImageView);
}

angle::Result RenderbufferVk::setStorage(const gl::Context *context,
                                         GLenum internalformat,
                                         size_t width,
                                         size_t height)
{
    ContextVk *contextVk       = vk::GetImpl(context);
    RendererVk *renderer       = contextVk->getRenderer();
    const vk::Format &vkFormat = renderer->getFormat(internalformat);

    if (mImage.valid())
    {
        // Check against the state if we need to recreate the storage.
        if (internalformat != mState.getFormat().info->internalFormat ||
            static_cast<GLsizei>(width) != mState.getWidth() ||
            static_cast<GLsizei>(height) != mState.getHeight())
        {
            mImage.release(renderer);
            renderer->releaseObject(renderer->getCurrentQueueSerial(), &mImageView);
        }
    }

    if (!mImage.valid() && (width != 0 && height != 0))
    {
        const angle::Format &textureFormat = vkFormat.textureFormat();
        bool isDepthOrStencilFormat = textureFormat.depthBits > 0 || textureFormat.stencilBits > 0;
        const VkImageUsageFlags usage =
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            (textureFormat.redBits > 0 ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0) |
            (isDepthOrStencilFormat ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0);

        gl::Extents extents(static_cast<int>(width), static_cast<int>(height), 1);
        ANGLE_TRY(mImage.init(contextVk, gl::TextureType::_2D, extents, vkFormat, 1, usage, 1));

        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        ANGLE_TRY(mImage.initMemory(contextVk, renderer->getMemoryProperties(), flags));

        VkImageAspectFlags aspect = vk::GetFormatAspectFlags(textureFormat);

        ANGLE_TRY(mImage.initImageView(contextVk, gl::TextureType::_2D, aspect, gl::SwizzleState(),
                                       &mImageView, 1));

        // TODO(jmadill): Fold this into the RenderPass load/store ops. http://anglebug.com/2361
        vk::CommandBuffer *commandBuffer = nullptr;
        ANGLE_TRY(mImage.recordCommands(contextVk, &commandBuffer));

        if (isDepthOrStencilFormat)
        {
            mImage.clearDepthStencil(aspect, aspect, kDefaultClearDepthStencilValue, commandBuffer);
        }
        else
        {
            mImage.clearColor(kBlackClearColorValue, 0, 1, commandBuffer);
        }
    }

    return angle::Result::Continue();
}

angle::Result RenderbufferVk::setStorageMultisample(const gl::Context *context,
                                                    size_t samples,
                                                    GLenum internalformat,
                                                    size_t width,
                                                    size_t height)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result RenderbufferVk::setStorageEGLImageTarget(const gl::Context *context,
                                                       egl::Image *image)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result RenderbufferVk::getAttachmentRenderTarget(const gl::Context *context,
                                                        GLenum binding,
                                                        const gl::ImageIndex &imageIndex,
                                                        FramebufferAttachmentRenderTarget **rtOut)
{
    ASSERT(mImage.valid());
    *rtOut = &mRenderTarget;
    return angle::Result::Continue();
}

angle::Result RenderbufferVk::initializeContents(const gl::Context *context,
                                                 const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return angle::Result::Continue();
}

}  // namespace rx
