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

RenderbufferVk::RenderbufferVk() : RenderbufferImpl(), mRequiredSize(0)
{
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

    VkImageUsageFlags usage =
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

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

    ANGLE_TRY(mImage.init(contextVk->getDevice(), imageInfo));

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ANGLE_TRY(vk::AllocateImageMemory(renderer, flags, &mImage, &mDeviceMemory, &mRequiredSize));
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

gl::Error RenderbufferVk::getAttachmentRenderTarget(const gl::Context *context,
                                                    GLenum binding,
                                                    const gl::ImageIndex &imageIndex,
                                                    FramebufferAttachmentRenderTarget **rtOut)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error RenderbufferVk::initializeContents(const gl::Context *context,
                                             const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return gl::NoError();
}

}  // namespace rx
