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
#include "image_util/generatemip.inl"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"

namespace rx
{
namespace
{
void MapSwizzleState(GLenum internalFormat,
                     const gl::SwizzleState &swizzleState,
                     gl::SwizzleState *swizzleStateOut)
{
    switch (internalFormat)
    {
        case GL_LUMINANCE8_OES:
            swizzleStateOut->swizzleRed   = swizzleState.swizzleRed;
            swizzleStateOut->swizzleGreen = swizzleState.swizzleRed;
            swizzleStateOut->swizzleBlue  = swizzleState.swizzleRed;
            swizzleStateOut->swizzleAlpha = GL_ONE;
            break;
        case GL_LUMINANCE8_ALPHA8_OES:
            swizzleStateOut->swizzleRed   = swizzleState.swizzleRed;
            swizzleStateOut->swizzleGreen = swizzleState.swizzleRed;
            swizzleStateOut->swizzleBlue  = swizzleState.swizzleRed;
            swizzleStateOut->swizzleAlpha = swizzleState.swizzleGreen;
            break;
        case GL_ALPHA8_OES:
            swizzleStateOut->swizzleRed   = GL_ZERO;
            swizzleStateOut->swizzleGreen = GL_ZERO;
            swizzleStateOut->swizzleBlue  = GL_ZERO;
            swizzleStateOut->swizzleAlpha = swizzleState.swizzleRed;
            break;
        case GL_RGB8:
            swizzleStateOut->swizzleRed   = swizzleState.swizzleRed;
            swizzleStateOut->swizzleGreen = swizzleState.swizzleGreen;
            swizzleStateOut->swizzleBlue  = swizzleState.swizzleBlue;
            swizzleStateOut->swizzleAlpha = GL_ONE;
            break;
        default:
            *swizzleStateOut = swizzleState;
            break;
    }
}

constexpr VkBufferUsageFlags kStagingBufferFlags =
    (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
constexpr size_t kStagingBufferSize = 1024 * 16;

constexpr VkFormatFeatureFlags kBlitFeatureFlags =
    VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;
}  // anonymous namespace

// StagingStorage implementation.
PixelBuffer::PixelBuffer(RendererVk *renderer)
    : mStagingBuffer(kStagingBufferFlags, kStagingBufferSize)
{
    // vkCmdCopyBufferToImage must have an offset that is a multiple of 4.
    // https://www.khronos.org/registry/vulkan/specs/1.0/man/html/VkBufferImageCopy.html
    mStagingBuffer.init(4, renderer);
}

PixelBuffer::~PixelBuffer()
{
}

void PixelBuffer::release(RendererVk *renderer)
{
    mStagingBuffer.release(renderer);
}

void PixelBuffer::removeStagedUpdates(const gl::ImageIndex &index)
{
    // Find any staged updates for this index and removes them from the pending list.
    uint32_t levelIndex    = static_cast<uint32_t>(index.getLevelIndex());
    uint32_t layerIndex    = static_cast<uint32_t>(index.getLayerIndex());
    auto removeIfStatement = [levelIndex, layerIndex](SubresourceUpdate &update) {
        return update.copyRegion.imageSubresource.mipLevel == levelIndex &&
               update.copyRegion.imageSubresource.baseArrayLayer == layerIndex;
    };
    mSubresourceUpdates.erase(
        std::remove_if(mSubresourceUpdates.begin(), mSubresourceUpdates.end(), removeIfStatement),
        mSubresourceUpdates.end());
}

gl::Error PixelBuffer::stageSubresourceUpdate(ContextVk *contextVk,
                                              const gl::ImageIndex &index,
                                              const gl::Extents &extents,
                                              const gl::Offset &offset,
                                              const gl::InternalFormat &formatInfo,
                                              const gl::PixelUnpackState &unpack,
                                              GLenum type,
                                              const uint8_t *pixels)
{
    GLuint inputRowPitch = 0;
    ANGLE_TRY_RESULT(
        formatInfo.computeRowPitch(type, extents.width, unpack.alignment, unpack.rowLength),
        inputRowPitch);

    GLuint inputDepthPitch = 0;
    ANGLE_TRY_RESULT(
        formatInfo.computeDepthPitch(extents.height, unpack.imageHeight, inputRowPitch),
        inputDepthPitch);

    // TODO(jmadill): skip images for 3D Textures.
    bool applySkipImages = false;

    GLuint inputSkipBytes = 0;
    ANGLE_TRY_RESULT(
        formatInfo.computeSkipBytes(type, inputRowPitch, inputDepthPitch, unpack, applySkipImages),
        inputSkipBytes);

    RendererVk *renderer = contextVk->getRenderer();

    const vk::Format &vkFormat         = renderer->getFormat(formatInfo.sizedInternalFormat);
    const angle::Format &storageFormat = vkFormat.textureFormat();

    size_t outputRowPitch   = storageFormat.pixelBytes * extents.width;
    size_t outputDepthPitch = outputRowPitch * extents.height;

    VkBuffer bufferHandle = VK_NULL_HANDLE;

    uint8_t *stagingPointer = nullptr;
    bool newBufferAllocated = false;
    uint32_t stagingOffset  = 0;
    size_t allocationSize   = outputDepthPitch * extents.depth;
    mStagingBuffer.allocate(renderer, allocationSize, &stagingPointer, &bufferHandle,
                            &stagingOffset, &newBufferAllocated);

    const uint8_t *source = pixels + inputSkipBytes;

    LoadImageFunctionInfo loadFunction = vkFormat.loadFunctions(type);

    loadFunction.loadFunction(extents.width, extents.height, extents.depth, source, inputRowPitch,
                              inputDepthPitch, stagingPointer, outputRowPitch, outputDepthPitch);

    VkBufferImageCopy copy;

    copy.bufferOffset                    = static_cast<VkDeviceSize>(stagingOffset);
    copy.bufferRowLength                 = extents.width;
    copy.bufferImageHeight               = extents.height;
    copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel       = index.getLevelIndex();
    copy.imageSubresource.baseArrayLayer = index.hasLayer() ? index.getLayerIndex() : 0;
    copy.imageSubresource.layerCount     = index.getLayerCount();

    gl_vk::GetOffset(offset, &copy.imageOffset);
    gl_vk::GetExtent(extents, &copy.imageExtent);

    mSubresourceUpdates.emplace_back(bufferHandle, copy);

    return gl::NoError();
}

gl::Error PixelBuffer::stageSubresourceUpdateFromFramebuffer(const gl::Context *context,
                                                             const gl::ImageIndex &index,
                                                             const gl::Rectangle &sourceArea,
                                                             const gl::Offset &dstOffset,
                                                             const gl::Extents &dstExtent,
                                                             const gl::InternalFormat &formatInfo,
                                                             FramebufferVk *framebufferVk)
{
    // If the extents and offset is outside the source image, we need to clip.
    gl::Rectangle clippedRectangle;
    const gl::Extents readExtents = framebufferVk->getReadImageExtents();
    if (!ClipRectangle(sourceArea, gl::Rectangle(0, 0, readExtents.width, readExtents.height),
                       &clippedRectangle))
    {
        // Empty source area, nothing to do.
        return gl::NoError();
    }

    // 1- obtain a buffer handle to copy to
    RendererVk *renderer = GetImplAs<ContextVk>(context)->getRenderer();

    const vk::Format &vkFormat         = renderer->getFormat(formatInfo.sizedInternalFormat);
    const angle::Format &storageFormat = vkFormat.textureFormat();
    LoadImageFunctionInfo loadFunction = vkFormat.loadFunctions(formatInfo.type);

    size_t outputRowPitch   = storageFormat.pixelBytes * clippedRectangle.width;
    size_t outputDepthPitch = outputRowPitch * clippedRectangle.height;

    VkBuffer bufferHandle = VK_NULL_HANDLE;

    uint8_t *stagingPointer = nullptr;
    bool newBufferAllocated = false;
    uint32_t stagingOffset  = 0;

    // The destination is only one layer deep.
    size_t allocationSize = outputDepthPitch;
    mStagingBuffer.allocate(renderer, allocationSize, &stagingPointer, &bufferHandle,
                            &stagingOffset, &newBufferAllocated);

    PackPixelsParams params;
    params.area        = sourceArea;
    params.format      = formatInfo.internalFormat;
    params.type        = formatInfo.type;
    params.outputPitch = static_cast<GLuint>(outputRowPitch);
    params.packBuffer  = nullptr;
    params.pack        = gl::PixelPackState();

    // 2- copy the source image region to the pixel buffer using a cpu readback
    if (loadFunction.requiresConversion)
    {
        // When a conversion is required, we need to use the loadFunction to read from a temporary
        // buffer instead so its an even slower path.
        size_t bufferSize = storageFormat.pixelBytes * sourceArea.width * sourceArea.height;
        angle::MemoryBuffer *memoryBuffer = nullptr;
        ANGLE_TRY(context->getScratchBuffer(bufferSize, &memoryBuffer));

        // Read into the scratch buffer
        ANGLE_TRY(framebufferVk->readPixelsImpl(context, sourceArea, params, memoryBuffer->data()));

        // Load from scratch buffer to our pixel buffer
        loadFunction.loadFunction(sourceArea.width, sourceArea.height, 1, memoryBuffer->data(),
                                  outputRowPitch, 0, stagingPointer, outputRowPitch, 0);
    }
    else
    {
        // We read directly from the framebuffer into our pixel buffer.
        ANGLE_TRY(framebufferVk->readPixelsImpl(context, sourceArea, params, stagingPointer));
    }

    // 3- enqueue the destination image subresource update
    VkBufferImageCopy copyToImage;
    copyToImage.bufferOffset                    = static_cast<VkDeviceSize>(stagingOffset);
    copyToImage.bufferRowLength                 = 0;  // Tightly packed data can be specified as 0.
    copyToImage.bufferImageHeight               = clippedRectangle.height;
    copyToImage.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyToImage.imageSubresource.mipLevel       = index.getLevelIndex();
    copyToImage.imageSubresource.baseArrayLayer = index.hasLayer() ? index.getLayerIndex() : 0;
    copyToImage.imageSubresource.layerCount     = index.getLayerCount();
    gl_vk::GetOffset(dstOffset, &copyToImage.imageOffset);
    gl_vk::GetExtent(dstExtent, &copyToImage.imageExtent);

    // 3- enqueue the destination image subresource update
    mSubresourceUpdates.emplace_back(bufferHandle, copyToImage);
    return gl::NoError();
}

gl::Error PixelBuffer::allocate(RendererVk *renderer,
                                size_t sizeInBytes,
                                uint8_t **ptrOut,
                                VkBuffer *handleOut,
                                uint32_t *offsetOut,
                                bool *newBufferAllocatedOut)
{
    return mStagingBuffer.allocate(renderer, sizeInBytes, ptrOut, handleOut, offsetOut,
                                   newBufferAllocatedOut);
}

vk::Error PixelBuffer::flushUpdatesToImage(RendererVk *renderer,
                                           uint32_t levelCount,
                                           vk::ImageHelper *image,
                                           vk::CommandBuffer *commandBuffer)
{
    if (mSubresourceUpdates.empty())
    {
        return vk::NoError();
    }

    ANGLE_TRY(mStagingBuffer.flush(renderer->getDevice()));

    std::vector<SubresourceUpdate> updatesToKeep;

    for (const SubresourceUpdate &update : mSubresourceUpdates)
    {
        ASSERT(update.bufferHandle != VK_NULL_HANDLE);

        const uint32_t updateMipLevel = update.copyRegion.imageSubresource.mipLevel;
        // It's possible we've accumulated updates that are no longer applicable if the image has
        // never been flushed but the image description has changed. Check if this level exist for
        // this image.
        if (updateMipLevel >= levelCount)
        {
            updatesToKeep.emplace_back(update);
            continue;
        }

        // Conservatively flush all writes to the image. We could use a more restricted barrier.
        // Do not move this above the for loop, otherwise multiple updates can have race conditions
        // and not be applied correctly as seen i:
        // dEQP-gles2.functional_texture_specification_texsubimage2d_align_2d* tests on Windows AMD
        image->changeLayoutWithStages(
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandBuffer);

        commandBuffer->copyBufferToImage(update.bufferHandle, image->getImage(),
                                         image->getCurrentLayout(), 1, &update.copyRegion);
    }

    // Only remove the updates that were actually applied to the image.
    mSubresourceUpdates = std::move(updatesToKeep);

    if (mSubresourceUpdates.empty())
    {
        mStagingBuffer.releaseRetainedBuffers(renderer);
    }
    else
    {
        WARN() << "Internal Vulkan bufffer could not be released. This is likely due to having "
                  "extra images defined in the Texture.";
    }

    return vk::NoError();
}

bool PixelBuffer::empty() const
{
    return mSubresourceUpdates.empty();
}

gl::Error PixelBuffer::stageSubresourceUpdateAndGetData(RendererVk *renderer,
                                                        size_t allocationSize,
                                                        const gl::ImageIndex &imageIndex,
                                                        const gl::Extents &extents,
                                                        const gl::Offset &offset,
                                                        uint8_t **destData)
{
    VkBuffer bufferHandle;
    uint32_t stagingOffset  = 0;
    bool newBufferAllocated = false;
    ANGLE_TRY(mStagingBuffer.allocate(renderer, allocationSize, destData, &bufferHandle,
                                      &stagingOffset, &newBufferAllocated));

    VkBufferImageCopy copy;
    copy.bufferOffset                    = static_cast<VkDeviceSize>(stagingOffset);
    copy.bufferRowLength                 = extents.width;
    copy.bufferImageHeight               = extents.height;
    copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel       = imageIndex.getLevelIndex();
    copy.imageSubresource.baseArrayLayer = imageIndex.hasLayer() ? imageIndex.getLayerIndex() : 0;
    copy.imageSubresource.layerCount     = imageIndex.getLayerCount();

    gl_vk::GetOffset(offset, &copy.imageOffset);
    gl_vk::GetExtent(extents, &copy.imageExtent);

    mSubresourceUpdates.emplace_back(bufferHandle, copy);

    return gl::NoError();
}

gl::Error TextureVk::generateMipmapLevelsWithCPU(ContextVk *contextVk,
                                                 const angle::Format &sourceFormat,
                                                 GLuint layer,
                                                 GLuint firstMipLevel,
                                                 GLuint maxMipLevel,
                                                 const size_t sourceWidth,
                                                 const size_t sourceHeight,
                                                 const size_t sourceRowPitch,
                                                 uint8_t *sourceData)
{
    RendererVk *renderer = contextVk->getRenderer();

    size_t previousLevelWidth    = sourceWidth;
    size_t previousLevelHeight   = sourceHeight;
    uint8_t *previousLevelData   = sourceData;
    size_t previousLevelRowPitch = sourceRowPitch;

    for (GLuint currentMipLevel = firstMipLevel; currentMipLevel <= maxMipLevel; currentMipLevel++)
    {
        // Compute next level width and height.
        size_t mipWidth  = std::max<size_t>(1, previousLevelWidth >> 1);
        size_t mipHeight = std::max<size_t>(1, previousLevelHeight >> 1);

        // With the width and height of the next mip, we can allocate the next buffer we need.
        uint8_t *destData   = nullptr;
        size_t destRowPitch = mipWidth * sourceFormat.pixelBytes;

        size_t mipAllocationSize = destRowPitch * mipHeight;
        gl::Extents mipLevelExtents(static_cast<int>(mipWidth), static_cast<int>(mipHeight), 1);

        ANGLE_TRY(mPixelBuffer.stageSubresourceUpdateAndGetData(
            renderer, mipAllocationSize,
            gl::ImageIndex::MakeFromType(mState.getType(), currentMipLevel, layer), mipLevelExtents,
            gl::Offset(), &destData));

        // Generate the mipmap into that new buffer
        sourceFormat.mipGenerationFunction(previousLevelWidth, previousLevelHeight, 1,
                                           previousLevelData, previousLevelRowPitch, 0, destData,
                                           destRowPitch, 0);

        // Swap for the next iteration
        previousLevelWidth    = mipWidth;
        previousLevelHeight   = mipHeight;
        previousLevelData     = destData;
        previousLevelRowPitch = destRowPitch;
    }

    return gl::NoError();
}

PixelBuffer::SubresourceUpdate::SubresourceUpdate() : bufferHandle(VK_NULL_HANDLE)
{
}

PixelBuffer::SubresourceUpdate::SubresourceUpdate(VkBuffer bufferHandleIn,
                                                  const VkBufferImageCopy &copyRegionIn)
    : bufferHandle(bufferHandleIn), copyRegion(copyRegionIn)
{
}

PixelBuffer::SubresourceUpdate::SubresourceUpdate(const SubresourceUpdate &other) = default;

// TextureVk implementation.
TextureVk::TextureVk(const gl::TextureState &state, RendererVk *renderer)
    : TextureImpl(state), mRenderTarget(&mImage, &mBaseLevelImageView, this), mPixelBuffer(renderer)
{
}

TextureVk::~TextureVk()
{
}

gl::Error TextureVk::onDestroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    releaseImage(context, renderer);
    renderer->releaseObject(getStoredQueueSerial(), &mSampler);

    mPixelBuffer.release(renderer);
    return gl::NoError();
}

gl::Error TextureVk::setImage(const gl::Context *context,
                              const gl::ImageIndex &index,
                              GLenum internalFormat,
                              const gl::Extents &size,
                              GLenum format,
                              GLenum type,
                              const gl::PixelUnpackState &unpack,
                              const uint8_t *pixels)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    // If there is any staged changes for this index, we can remove them since we're going to
    // override them with this call.
    mPixelBuffer.removeStagedUpdates(index);

    // Convert internalFormat to sized internal format.
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat, type);

    if (mImage.valid())
    {
        const vk::Format &vkFormat = renderer->getFormat(formatInfo.sizedInternalFormat);

        // Calculate the expected size for the index we are defining. If the size is different from
        // the given size, or the format is different, we are redefining the image so we must
        // release it.
        if (mImage.getFormat() != vkFormat || size != mImage.getSize(index))
        {
            releaseImage(context, renderer);
        }
    }

    // Early-out on empty textures, don't create a zero-sized storage.
    if (size.empty())
    {
        return gl::NoError();
    }

    // Create a new graph node to store image initialization commands.
    onResourceChanged(renderer);

    // Handle initial data.
    if (pixels)
    {
        ANGLE_TRY(mPixelBuffer.stageSubresourceUpdate(contextVk, index, size, gl::Offset(),
                                                      formatInfo, unpack, type, pixels));
    }

    return gl::NoError();
}

gl::Error TextureVk::setSubImage(const gl::Context *context,
                                 const gl::ImageIndex &index,
                                 const gl::Box &area,
                                 GLenum format,
                                 GLenum type,
                                 const gl::PixelUnpackState &unpack,
                                 const uint8_t *pixels)
{
    ContextVk *contextVk                 = vk::GetImpl(context);
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(format, type);
    ANGLE_TRY(mPixelBuffer.stageSubresourceUpdate(
        contextVk, index, gl::Extents(area.width, area.height, area.depth),
        gl::Offset(area.x, area.y, area.z), formatInfo, unpack, type, pixels));

    // Create a new graph node to store image initialization commands.
    onResourceChanged(contextVk->getRenderer());

    return gl::NoError();
}

gl::Error TextureVk::setCompressedImage(const gl::Context *context,
                                        const gl::ImageIndex &index,
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
                                           const gl::ImageIndex &index,
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
                               const gl::ImageIndex &index,
                               const gl::Rectangle &sourceArea,
                               GLenum internalFormat,
                               gl::Framebuffer *source)
{
    gl::Extents newImageSize(sourceArea.width, sourceArea.height, 1);
    const gl::InternalFormat &internalFormatInfo =
        gl::GetInternalFormatInfo(internalFormat, GL_UNSIGNED_BYTE);
    ANGLE_TRY(setImage(context, index, internalFormat, newImageSize, internalFormatInfo.format,
                       internalFormatInfo.type, gl::PixelUnpackState(), nullptr));
    return copySubImageImpl(context, index, gl::Offset(0, 0, 0), sourceArea, internalFormatInfo,
                            source);
}

gl::Error TextureVk::copySubImage(const gl::Context *context,
                                  const gl::ImageIndex &index,
                                  const gl::Offset &destOffset,
                                  const gl::Rectangle &sourceArea,
                                  gl::Framebuffer *source)
{
    const gl::InternalFormat &currentFormat = *mState.getBaseLevelDesc().format.info;
    return copySubImageImpl(context, index, destOffset, sourceArea, currentFormat, source);
}

gl::Error TextureVk::copySubImageImpl(const gl::Context *context,
                                      const gl::ImageIndex &index,
                                      const gl::Offset &destOffset,
                                      const gl::Rectangle &sourceArea,
                                      const gl::InternalFormat &internalFormat,
                                      gl::Framebuffer *source)
{
    gl::Extents fbSize = source->getReadColorbuffer()->getSize();
    gl::Rectangle clippedSourceArea;
    if (!ClipRectangle(sourceArea, gl::Rectangle(0, 0, fbSize.width, fbSize.height),
                       &clippedSourceArea))
    {
        return gl::NoError();
    }

    const gl::Offset modifiedDestOffset(destOffset.x + sourceArea.x - sourceArea.x,
                                        destOffset.y + sourceArea.y - sourceArea.y, 0);

    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer         = contextVk->getRenderer();
    FramebufferVk *framebufferVk = vk::GetImpl(source);

    // For now, favor conformance. We do a CPU readback that does the conversion, and then stage the
    // change to the pixel buffer.
    // Eventually we can improve this easily by implementing vkCmdBlitImage to do the conversion
    // when its supported.
    ANGLE_TRY(mPixelBuffer.stageSubresourceUpdateFromFramebuffer(
        context, index, clippedSourceArea, modifiedDestOffset,
        gl::Extents(clippedSourceArea.width, clippedSourceArea.height, 1), internalFormat,
        framebufferVk));

    onResourceChanged(renderer);
    framebufferVk->addReadDependency(this);
    return gl::NoError();
}

vk::Error TextureVk::getCommandBufferForWrite(RendererVk *renderer,
                                              vk::CommandBuffer **commandBufferOut)
{
    ANGLE_TRY(appendWriteResource(renderer, commandBufferOut));
    return vk::NoError();
}

gl::Error TextureVk::setStorage(const gl::Context *context,
                                gl::TextureType type,
                                size_t levels,
                                GLenum internalFormat,
                                const gl::Extents &size)
{
    ContextVk *contextVk             = GetAs<ContextVk>(context->getImplementation());
    RendererVk *renderer             = contextVk->getRenderer();
    const vk::Format &format         = renderer->getFormat(internalFormat);
    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(getCommandBufferForWrite(renderer, &commandBuffer));
    ANGLE_TRY(initImage(renderer, format, size, static_cast<uint32_t>(levels), commandBuffer));
    return gl::NoError();
}

gl::Error TextureVk::setEGLImageTarget(const gl::Context *context,
                                       gl::TextureType type,
                                       egl::Image *image)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error TextureVk::setImageExternal(const gl::Context *context,
                                      gl::TextureType type,
                                      egl::Stream *stream,
                                      const egl::Stream::GLTextureDescription &desc)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

void TextureVk::generateMipmapWithBlit(RendererVk *renderer)
{
    uint32_t imageLayerCount           = GetImageLayerCount(mState.getType());
    const gl::Extents baseLevelExtents = mImage.getExtents();
    vk::CommandBuffer *commandBuffer   = nullptr;
    getCommandBufferForWrite(renderer, &commandBuffer);

    // We are able to use blitImage since the image format we are using supports it. This
    // is a faster way we can generate the mips.
    int32_t mipWidth  = baseLevelExtents.width;
    int32_t mipHeight = baseLevelExtents.height;

    // Manually manage the image memory barrier because it uses a lot more parameters than our
    // usual one.
    VkImageMemoryBarrier barrier;
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = mImage.getImage().getHandle();
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.pNext                           = nullptr;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = imageLayerCount;
    barrier.subresourceRange.levelCount     = 1;

    for (uint32_t mipLevel = 1; mipLevel <= mState.getMipmapMaxLevel(); mipLevel++)
    {
        int32_t nextMipWidth  = std::max<int32_t>(1, mipWidth >> 1);
        int32_t nextMipHeight = std::max<int32_t>(1, mipHeight >> 1);

        barrier.subresourceRange.baseMipLevel = mipLevel - 1;
        barrier.oldLayout                     = mImage.getCurrentLayout();
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        // We can do it for all layers at once.
        commandBuffer->singleImageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                          VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

        VkImageBlit blit                   = {};
        blit.srcOffsets[0]                 = {0, 0, 0};
        blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = mipLevel - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = imageLayerCount;
        blit.dstOffsets[0]                 = {0, 0, 0};
        blit.dstOffsets[1]                 = {nextMipWidth, nextMipHeight, 1};
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = mipLevel;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = imageLayerCount;

        mipWidth  = nextMipWidth;
        mipHeight = nextMipHeight;

        commandBuffer->blitImage(mImage.getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 mImage.getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                                 VK_FILTER_LINEAR);
    }

    // Transition the last mip level to the same layout as all the other ones, so we can declare
    // our whole image layout to be SRC_OPTIMAL.
    barrier.subresourceRange.baseMipLevel = mState.getMipmapMaxLevel();
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    // We can do it for all layers at once.
    commandBuffer->singleImageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, barrier);

    // This is just changing the internal state of the image helper so that the next call
    // to changeLayoutWithStages will use this layout as the "oldLayout" argument.
    mImage.updateLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
}

gl::Error TextureVk::generateMipmapWithCPU(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    bool newBufferAllocated            = false;
    const gl::Extents baseLevelExtents = mImage.getExtents();
    uint32_t imageLayerCount           = GetImageLayerCount(mState.getType());
    const angle::Format &angleFormat   = mImage.getFormat().textureFormat();
    GLuint sourceRowPitch              = baseLevelExtents.width * angleFormat.pixelBytes;
    size_t baseLevelAllocationSize     = sourceRowPitch * baseLevelExtents.height;

    vk::CommandBuffer *commandBuffer = nullptr;
    getCommandBufferForWrite(renderer, &commandBuffer);

    // Requirement of the copyImageToBuffer, the source image must be in SRC_OPTIMAL layout.
    mImage.changeLayoutWithStages(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    size_t totalAllocationSize = baseLevelAllocationSize * imageLayerCount;

    VkBuffer copyBufferHandle;
    uint8_t *baseLevelBuffers;
    uint32_t copyBaseOffset;

    // Allocate enough memory to copy every level 0 image (one for each layer of the texture).
    ANGLE_TRY(mPixelBuffer.allocate(renderer, totalAllocationSize, &baseLevelBuffers,
                                    &copyBufferHandle, &copyBaseOffset, &newBufferAllocated));

    // Do only one copy for all layers at once.
    VkBufferImageCopy region;
    region.bufferImageHeight               = baseLevelExtents.height;
    region.bufferOffset                    = static_cast<VkDeviceSize>(copyBaseOffset);
    region.bufferRowLength                 = baseLevelExtents.width;
    region.imageExtent.width               = baseLevelExtents.width;
    region.imageExtent.height              = baseLevelExtents.height;
    region.imageExtent.depth               = 1;
    region.imageOffset.x                   = 0;
    region.imageOffset.y                   = 0;
    region.imageOffset.z                   = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = imageLayerCount;
    region.imageSubresource.mipLevel       = mState.getEffectiveBaseLevel();

    commandBuffer->copyImageToBuffer(mImage.getImage(), mImage.getCurrentLayout(), copyBufferHandle,
                                     1, &region);

    ANGLE_TRY(renderer->finish(context));

    const uint32_t levelCount = getLevelCount();

    // We now have the base level available to be manipulated in the baseLevelBuffer pointer.
    // Generate all the missing mipmaps with the slow path. We can optimize with vkCmdBlitImage
    // later.
    // For each layer, use the copied data to generate all the mips.
    for (GLuint layer = 0; layer < imageLayerCount; layer++)
    {
        size_t bufferOffset = layer * baseLevelAllocationSize;

        ANGLE_TRY(generateMipmapLevelsWithCPU(
            contextVk, angleFormat, layer, mState.getEffectiveBaseLevel() + 1,
            mState.getMipmapMaxLevel(), baseLevelExtents.width, baseLevelExtents.height,
            sourceRowPitch, baseLevelBuffers + bufferOffset));
    }

    mPixelBuffer.flushUpdatesToImage(renderer, levelCount, &mImage, commandBuffer);
    return gl::NoError();
}

gl::Error TextureVk::generateMipmap(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    // Some data is pending, or the image has not been defined at all yet
    if (!mImage.valid())
    {
        // lets initialize the image so we can generate the next levels.
        if (!mPixelBuffer.empty())
        {
            ANGLE_TRY(ensureImageInitialized(renderer));
            ASSERT(mImage.valid());
        }
        else
        {
            // There is nothing to generate if there is nothing uploaded so far.
            return gl::NoError();
        }
    }

    VkFormatProperties imageProperties;
    vk::GetFormatProperties(renderer->getPhysicalDevice(), mImage.getFormat().vkTextureFormat,
                            &imageProperties);

    // Check if the image supports blit. If it does, we can do the mipmap generation on the gpu
    // only.
    if (IsMaskFlagSet(kBlitFeatureFlags, imageProperties.linearTilingFeatures))
    {
        generateMipmapWithBlit(renderer);
    }
    else
    {
        ANGLE_TRY(generateMipmapWithCPU(context));
    }

    // We're changing this textureVk content, make sure we let the graph know.
    onResourceChanged(renderer);

    return gl::NoError();
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
    // TODO(jmadill): Handle cube textures. http://anglebug.com/2470
    ASSERT(imageIndex.getType() == gl::TextureType::_2D);

    // Non-zero mip level attachments are an ES 3.0 feature.
    ASSERT(imageIndex.getLevelIndex() == 0 && !imageIndex.hasLayer());

    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    ANGLE_TRY(ensureImageInitialized(renderer));

    *rtOut = &mRenderTarget;
    return gl::NoError();
}

vk::Error TextureVk::ensureImageInitialized(RendererVk *renderer)
{
    if (mImage.valid() && mPixelBuffer.empty())
    {
        return vk::NoError();
    }

    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(getCommandBufferForWrite(renderer, &commandBuffer));

    const gl::ImageDesc &baseLevelDesc  = mState.getBaseLevelDesc();
    const gl::Extents &baseLevelExtents = baseLevelDesc.size;
    const uint32_t levelCount           = getLevelCount();

    if (!mImage.valid())
    {
        const vk::Format &format =
            renderer->getFormat(baseLevelDesc.format.info->sizedInternalFormat);

        ANGLE_TRY(initImage(renderer, format, baseLevelExtents, levelCount, commandBuffer));
    }

    ANGLE_TRY(mPixelBuffer.flushUpdatesToImage(renderer, levelCount, &mImage, commandBuffer));
    return vk::NoError();
}

gl::Error TextureVk::syncState(const gl::Context *context, const gl::Texture::DirtyBits &dirtyBits)
{
    if (dirtyBits.none() && mSampler.valid())
    {
        return gl::NoError();
    }

    ContextVk *contextVk = vk::GetImpl(context);
    if (mSampler.valid())
    {
        RendererVk *renderer = contextVk->getRenderer();
        renderer->releaseObject(getStoredQueueSerial(), &mSampler);
    }

    const gl::SamplerState &samplerState = mState.getSamplerState();

    // Create a simple sampler. Force basic parameter settings.
    VkSamplerCreateInfo samplerInfo;
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.pNext                   = nullptr;
    samplerInfo.flags                   = 0;
    samplerInfo.magFilter               = gl_vk::GetFilter(samplerState.magFilter);
    samplerInfo.minFilter               = gl_vk::GetFilter(samplerState.minFilter);
    samplerInfo.mipmapMode              = gl_vk::GetSamplerMipmapMode(samplerState.minFilter);
    samplerInfo.addressModeU            = gl_vk::GetSamplerAddressMode(samplerState.wrapS);
    samplerInfo.addressModeV            = gl_vk::GetSamplerAddressMode(samplerState.wrapT);
    samplerInfo.addressModeW            = gl_vk::GetSamplerAddressMode(samplerState.wrapR);
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.anisotropyEnable        = VK_FALSE;
    samplerInfo.maxAnisotropy           = 1.0f;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod                  = samplerState.minLod;
    samplerInfo.maxLod                  = samplerState.maxLod;
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    ANGLE_TRY(mSampler.init(contextVk->getDevice(), samplerInfo));
    return gl::NoError();
}

gl::Error TextureVk::setStorageMultisample(const gl::Context *context,
                                           gl::TextureType type,
                                           GLsizei samples,
                                           GLint internalformat,
                                           const gl::Extents &size,
                                           bool fixedSampleLocations)
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

const vk::ImageHelper &TextureVk::getImage() const
{
    ASSERT(mImage.valid());
    return mImage;
}

const vk::ImageView &TextureVk::getImageView() const
{
    ASSERT(mImage.valid());

    const GLenum minFilter = mState.getSamplerState().minFilter;
    if (minFilter == GL_LINEAR || minFilter == GL_NEAREST)
    {
        return mBaseLevelImageView;
    }

    return mMipmapImageView;
}

const vk::Sampler &TextureVk::getSampler() const
{
    ASSERT(mSampler.valid());
    return mSampler;
}

vk::Error TextureVk::initImage(RendererVk *renderer,
                               const vk::Format &format,
                               const gl::Extents &extents,
                               const uint32_t levelCount,
                               vk::CommandBuffer *commandBuffer)
{
    const VkDevice device = renderer->getDevice();

    const VkImageUsageFlags usage =
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    ANGLE_TRY(mImage.init(device, mState.getType(), extents, format, 1, usage, levelCount));

    const VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ANGLE_TRY(mImage.initMemory(device, renderer->getMemoryProperties(), flags));

    gl::SwizzleState mappedSwizzle;
    MapSwizzleState(format.internalFormat, mState.getSwizzleState(), &mappedSwizzle);

    // TODO(jmadill): Separate imageviews for RenderTargets and Sampling.
    ANGLE_TRY(mImage.initImageView(device, mState.getType(), VK_IMAGE_ASPECT_COLOR_BIT,
                                   mappedSwizzle, &mMipmapImageView, levelCount));
    ANGLE_TRY(mImage.initImageView(device, mState.getType(), VK_IMAGE_ASPECT_COLOR_BIT,
                                   mappedSwizzle, &mBaseLevelImageView, 1));

    // TODO(jmadill): Fold this into the RenderPass load/store ops. http://anglebug.com/2361
    VkClearColorValue black = {{0, 0, 0, 1.0f}};
    mImage.clearColor(black, 0, levelCount, commandBuffer);
    return vk::NoError();
}

void TextureVk::releaseImage(const gl::Context *context, RendererVk *renderer)
{
    mImage.release(renderer->getCurrentQueueSerial(), renderer);
    renderer->releaseObject(getStoredQueueSerial(), &mBaseLevelImageView);
    renderer->releaseObject(getStoredQueueSerial(), &mMipmapImageView);
    onStateChange(context, angle::SubjectMessage::DEPENDENT_DIRTY_BITS);
}

uint32_t TextureVk::getLevelCount() const
{
    ASSERT(mState.getEffectiveBaseLevel() == 0);

    // getMipmapMaxLevel will be 0 here if mipmaps are not used, so the levelCount is always +1.
    return mState.getMipmapMaxLevel() + 1;
}
}  // namespace rx
