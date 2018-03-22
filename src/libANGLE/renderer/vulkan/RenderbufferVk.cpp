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
    : RenderbufferImpl(state), mAllocatedMemorySize(0)
{
    mRenderTarget.image     = &mImage;
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

    renderer->releaseResource(*this, &mImage);
    renderer->releaseResource(*this, &mDeviceMemory);
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
            ASSERT(mImageView.valid());
            renderer->releaseResource(*this, &mImage);
            renderer->releaseResource(*this, &mDeviceMemory);
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

        VkImageCreateInfo imageInfo;
        imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext                 = nullptr;
        imageInfo.flags                 = 0;
        imageInfo.imageType             = VK_IMAGE_TYPE_2D;
        imageInfo.format                = vkFormat.vkTextureFormat;
        imageInfo.extent.width          = static_cast<uint32_t>(width);
        imageInfo.extent.height         = static_cast<uint32_t>(height);
        imageInfo.extent.depth          = 1;
        imageInfo.mipLevels             = 1;
        imageInfo.arrayLayers           = 1;
        imageInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage                 = usage;
        imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.queueFamilyIndexCount = 0;
        imageInfo.pQueueFamilyIndices   = nullptr;
        imageInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

        ANGLE_TRY(mImage.init(device, imageInfo));

        VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        ANGLE_TRY(vk::AllocateImageMemory(renderer, flags, &mImage, &mDeviceMemory,
                                          &mAllocatedMemorySize));

        VkImageAspectFlags aspect =
            (textureFormat.depthBits > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            (textureFormat.stencilBits > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0) |
            (textureFormat.redBits > 0 ? VK_IMAGE_ASPECT_COLOR_BIT : 0);

        // Allocate ImageView.
        VkImageViewCreateInfo viewInfo;
        viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext                           = nullptr;
        viewInfo.flags                           = 0;
        viewInfo.image                           = mImage.getHandle();
        viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format                          = vkFormat.vkTextureFormat;
        viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
        viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
        viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
        viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
        viewInfo.subresourceRange.aspectMask     = aspect;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        ANGLE_TRY(mImageView.init(device, viewInfo));

        // TODO(jmadill): Fold this into the RenderPass load/store ops. http://anglebug.com/2361
        vk::CommandBuffer *commandBuffer = nullptr;
        ANGLE_TRY(beginWriteResource(renderer, &commandBuffer));
        mImage.changeLayoutWithStages(aspect, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT, commandBuffer);

        if (isDepthOrStencilFormat)
        {
            commandBuffer->clearSingleDepthStencilImage(mImage, aspect,
                                                        kDefaultClearDepthStencilValue);
        }
        else
        {
            commandBuffer->clearSingleColorImage(mImage, kBlackClearColorValue);
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
