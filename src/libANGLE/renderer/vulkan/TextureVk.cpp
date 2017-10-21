//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TextureVk.cpp:
//    Implements the class methods for TextureVk.
//

#include "libANGLE/renderer/vulkan/TextureVk.h"

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/formatutilsvk.h"

namespace rx
{

TextureVk::TextureVk(const gl::TextureState &state) : TextureImpl(state)
{
}

TextureVk::~TextureVk()
{
}

gl::Error TextureVk::onDestroy(const gl::Context *context)
{
    ContextVk *contextVk = GetImplAs<ContextVk>(context);
    RendererVk *renderer = contextVk->getRenderer();

    renderer->enqueueGarbageOrDeleteNow(*this, std::move(mImage));
    renderer->enqueueGarbageOrDeleteNow(*this, std::move(mDeviceMemory));
    renderer->enqueueGarbageOrDeleteNow(*this, std::move(mImageView));

    return gl::NoError();
}

gl::Error TextureVk::setImage(const gl::Context *context,
                              GLenum target,
                              size_t level,
                              GLenum internalFormat,
                              const gl::Extents &size,
                              GLenum format,
                              GLenum type,
                              const gl::PixelUnpackState &unpack,
                              const uint8_t *pixels)
{
    // TODO(jmadill): support multi-level textures.
    ASSERT(level == 0);

    // TODO(jmadill): support texture re-definition.
    ASSERT(!mImage.valid());

    // TODO(jmadill): support other types of textures.
    ASSERT(target == GL_TEXTURE_2D);

    // Convert internalFormat to sized internal format.
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat, type);
    const vk::Format &vkFormat           = vk::Format::Get(formatInfo.sizedInternalFormat);

    VkImageCreateInfo imageInfo;
    imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext         = nullptr;
    imageInfo.flags         = 0;
    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.format        = vkFormat.native;
    imageInfo.extent.width  = size.width;
    imageInfo.extent.height = size.height;
    imageInfo.extent.depth  = size.depth;
    imageInfo.mipLevels     = 1;
    imageInfo.arrayLayers   = 1;
    imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;

    // TODO(jmadill): Are all these image transfer bits necessary?
    imageInfo.usage = (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                       VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices   = nullptr;
    imageInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    ContextVk *contextVk = GetImplAs<ContextVk>(context);
    VkDevice device      = contextVk->getDevice();
    ANGLE_TRY(mImage.init(device, imageInfo));

    // Allocate the device memory for the image.
    // TODO(jmadill): Use more intelligent device memory allocation.
    VkMemoryRequirements memoryRequirements;
    mImage.getMemoryRequirements(device, &memoryRequirements);

    RendererVk *renderer = contextVk->getRenderer();

    uint32_t memoryIndex = renderer->getMemoryProperties().findCompatibleMemoryIndex(
        memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocateInfo;
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext           = nullptr;
    allocateInfo.allocationSize  = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryIndex;

    ANGLE_TRY(mDeviceMemory.allocate(device, allocateInfo));
    ANGLE_TRY(mImage.bindMemory(device, mDeviceMemory));

    VkImageViewCreateInfo viewInfo;
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext                           = nullptr;
    viewInfo.flags                           = 0;
    viewInfo.image                           = mImage.getHandle();
    viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format                          = vkFormat.native;
    viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = 1;

    ANGLE_TRY(mImageView.init(device, viewInfo));

    // Handle initial data.
    // TODO(jmadill): Consider re-using staging texture.
    if (pixels)
    {
        vk::StagingImage stagingImage;
        ANGLE_TRY(renderer->createStagingImage(TextureDimension::TEX_2D, vkFormat, size,
                                               vk::StagingUsage::Write, &stagingImage));

        GLuint inputRowPitch = 0;
        ANGLE_TRY_RESULT(
            formatInfo.computeRowPitch(type, size.width, unpack.alignment, unpack.rowLength),
            inputRowPitch);

        GLuint inputDepthPitch = 0;
        ANGLE_TRY_RESULT(
            formatInfo.computeDepthPitch(size.height, unpack.imageHeight, inputRowPitch),
            inputDepthPitch);

        // TODO(jmadill): skip images for 3D Textures.
        bool applySkipImages = false;

        GLuint inputSkipBytes = 0;
        ANGLE_TRY_RESULT(
            formatInfo.computeSkipBytes(inputRowPitch, inputDepthPitch, unpack, applySkipImages),
            inputSkipBytes);

        auto loadFunction = vkFormat.getLoadFunctions()(type);

        uint8_t *mapPointer = nullptr;
        ANGLE_TRY(stagingImage.getDeviceMemory().map(device, 0, VK_WHOLE_SIZE, 0, &mapPointer));

        const uint8_t *source = pixels + inputSkipBytes;

        // Get the subresource layout. This has important parameters like row pitch.
        // TODO(jmadill): Fill out this structure based on input parameters.
        VkImageSubresource subresource;
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.mipLevel   = 0;
        subresource.arrayLayer = 0;

        VkSubresourceLayout subresourceLayout;
        vkGetImageSubresourceLayout(device, stagingImage.getImage().getHandle(), &subresource,
                                    &subresourceLayout);

        loadFunction.loadFunction(size.width, size.height, size.depth, source, inputRowPitch,
                                  inputDepthPitch, mapPointer,
                                  static_cast<size_t>(subresourceLayout.rowPitch),
                                  static_cast<size_t>(subresourceLayout.depthPitch));

        stagingImage.getDeviceMemory().unmap(device);

        vk::CommandBuffer *commandBuffer = nullptr;
        ANGLE_TRY(contextVk->getStartedCommandBuffer(&commandBuffer));
        setQueueSerial(renderer->getCurrentQueueSerial());

        stagingImage.getImage().changeLayoutTop(
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);
        mImage.changeLayoutTop(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               commandBuffer);

        gl::Box wholeRegion(0, 0, 0, size.width, size.height, size.depth);
        commandBuffer->copySingleImage(stagingImage.getImage(), mImage, wholeRegion,
                                       VK_IMAGE_ASPECT_COLOR_BIT);

        // TODO(jmadill): Re-use staging images.
        renderer->enqueueGarbage(renderer->getCurrentQueueSerial(), std::move(stagingImage));
    }

    return gl::NoError();
}

gl::Error TextureVk::setSubImage(const gl::Context *context,
                                 GLenum target,
                                 size_t level,
                                 const gl::Box &area,
                                 GLenum format,
                                 GLenum type,
                                 const gl::PixelUnpackState &unpack,
                                 const uint8_t *pixels)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::setCompressedImage(const gl::Context *context,
                                        GLenum target,
                                        size_t level,
                                        GLenum internalFormat,
                                        const gl::Extents &size,
                                        const gl::PixelUnpackState &unpack,
                                        size_t imageSize,
                                        const uint8_t *pixels)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::setCompressedSubImage(const gl::Context *context,
                                           GLenum target,
                                           size_t level,
                                           const gl::Box &area,
                                           GLenum format,
                                           const gl::PixelUnpackState &unpack,
                                           size_t imageSize,
                                           const uint8_t *pixels)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::copyImage(const gl::Context *context,
                               GLenum target,
                               size_t level,
                               const gl::Rectangle &sourceArea,
                               GLenum internalFormat,
                               const gl::Framebuffer *source)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::copySubImage(const gl::Context *context,
                                  GLenum target,
                                  size_t level,
                                  const gl::Offset &destOffset,
                                  const gl::Rectangle &sourceArea,
                                  const gl::Framebuffer *source)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::setStorage(const gl::Context *context,
                                GLenum target,
                                size_t levels,
                                GLenum internalFormat,
                                const gl::Extents &size)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::setEGLImageTarget(const gl::Context *context, GLenum target, egl::Image *image)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::setImageExternal(const gl::Context *context,
                                      GLenum target,
                                      egl::Stream *stream,
                                      const egl::Stream::GLTextureDescription &desc)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::generateMipmap(const gl::Context *context)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::setBaseLevel(const gl::Context *context, GLuint baseLevel)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::bindTexImage(const gl::Context *context, egl::Surface *surface)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::releaseTexImage(const gl::Context *context)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::getAttachmentRenderTarget(const gl::Context *context,
                                               GLenum binding,
                                               const gl::ImageIndex &imageIndex,
                                               FramebufferAttachmentRenderTarget **rtOut)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

void TextureVk::syncState(const gl::Texture::DirtyBits &dirtyBits)
{
    UNIMPLEMENTED();
}

gl::Error TextureVk::setStorageMultisample(const gl::Context *context,
                                           GLenum target,
                                           GLsizei samples,
                                           GLint internalformat,
                                           const gl::Extents &size,
                                           GLboolean fixedSampleLocations)
{
    UNIMPLEMENTED();
    return gl::InternalError() << "setStorageMultisample is unimplemented.";
}

gl::Error TextureVk::initializeContents(const gl::Context *context,
                                        const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return gl::NoError();
}

}  // namespace rx
