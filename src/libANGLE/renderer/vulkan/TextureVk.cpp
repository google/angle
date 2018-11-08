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

angle::Result PixelBuffer::stageSubresourceUpdate(ContextVk *contextVk,
                                                  const gl::ImageIndex &index,
                                                  const gl::Extents &extents,
                                                  const gl::Offset &offset,
                                                  const gl::InternalFormat &formatInfo,
                                                  const gl::PixelUnpackState &unpack,
                                                  GLenum type,
                                                  const uint8_t *pixels)
{
    GLuint inputRowPitch = 0;
    ANGLE_VK_CHECK_MATH(contextVk, formatInfo.computeRowPitch(type, extents.width, unpack.alignment,
                                                              unpack.rowLength, &inputRowPitch));

    GLuint inputDepthPitch = 0;
    ANGLE_VK_CHECK_MATH(contextVk, formatInfo.computeDepthPitch(extents.height, unpack.imageHeight,
                                                                inputRowPitch, &inputDepthPitch));

    // TODO(jmadill): skip images for 3D Textures.
    bool applySkipImages = false;

    GLuint inputSkipBytes = 0;
    ANGLE_VK_CHECK_MATH(contextVk,
                        formatInfo.computeSkipBytes(type, inputRowPitch, inputDepthPitch, unpack,
                                                    applySkipImages, &inputSkipBytes));

    RendererVk *renderer = contextVk->getRenderer();

    const vk::Format &vkFormat         = renderer->getFormat(formatInfo.sizedInternalFormat);
    const angle::Format &storageFormat = vkFormat.textureFormat();

    size_t outputRowPitch   = storageFormat.pixelBytes * extents.width;
    size_t outputDepthPitch = outputRowPitch * extents.height;

    VkBuffer bufferHandle = VK_NULL_HANDLE;

    uint8_t *stagingPointer    = nullptr;
    bool newBufferAllocated    = false;
    VkDeviceSize stagingOffset = 0;
    size_t allocationSize      = outputDepthPitch * extents.depth;
    ANGLE_TRY(mStagingBuffer.allocate(contextVk, allocationSize, &stagingPointer, &bufferHandle,
                                      &stagingOffset, &newBufferAllocated));

    const uint8_t *source = pixels + inputSkipBytes;

    LoadImageFunctionInfo loadFunction = vkFormat.textureLoadFunctions(type);

    loadFunction.loadFunction(extents.width, extents.height, extents.depth, source, inputRowPitch,
                              inputDepthPitch, stagingPointer, outputRowPitch, outputDepthPitch);

    VkBufferImageCopy copy = {};

    copy.bufferOffset                    = stagingOffset;
    copy.bufferRowLength                 = extents.width;
    copy.bufferImageHeight               = extents.height;
    copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel       = index.getLevelIndex();
    copy.imageSubresource.baseArrayLayer = index.hasLayer() ? index.getLayerIndex() : 0;
    copy.imageSubresource.layerCount     = index.getLayerCount();

    gl_vk::GetOffset(offset, &copy.imageOffset);
    gl_vk::GetExtent(extents, &copy.imageExtent);

    mSubresourceUpdates.emplace_back(bufferHandle, copy);

    return angle::Result::Continue();
}

angle::Result PixelBuffer::stageSubresourceUpdateFromFramebuffer(
    const gl::Context *context,
    const gl::ImageIndex &index,
    const gl::Rectangle &sourceArea,
    const gl::Offset &dstOffset,
    const gl::Extents &dstExtent,
    const gl::InternalFormat &formatInfo,
    FramebufferVk *framebufferVk)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // If the extents and offset is outside the source image, we need to clip.
    gl::Rectangle clippedRectangle;
    const gl::Extents readExtents = framebufferVk->getReadImageExtents();
    if (!ClipRectangle(sourceArea, gl::Rectangle(0, 0, readExtents.width, readExtents.height),
                       &clippedRectangle))
    {
        // Empty source area, nothing to do.
        return angle::Result::Continue();
    }

    bool isViewportFlipEnabled = contextVk->isViewportFlipEnabledForDrawFBO();
    if (isViewportFlipEnabled)
    {
        clippedRectangle.y = readExtents.height - clippedRectangle.y - clippedRectangle.height;
    }

    // 1- obtain a buffer handle to copy to
    RendererVk *renderer = contextVk->getRenderer();

    const vk::Format &vkFormat         = renderer->getFormat(formatInfo.sizedInternalFormat);
    const angle::Format &storageFormat = vkFormat.textureFormat();
    LoadImageFunctionInfo loadFunction = vkFormat.textureLoadFunctions(formatInfo.type);

    size_t outputRowPitch   = storageFormat.pixelBytes * clippedRectangle.width;
    size_t outputDepthPitch = outputRowPitch * clippedRectangle.height;

    VkBuffer bufferHandle = VK_NULL_HANDLE;

    uint8_t *stagingPointer    = nullptr;
    bool newBufferAllocated    = false;
    VkDeviceSize stagingOffset = 0;

    // The destination is only one layer deep.
    size_t allocationSize = outputDepthPitch;
    ANGLE_TRY(mStagingBuffer.allocate(contextVk, allocationSize, &stagingPointer, &bufferHandle,
                                      &stagingOffset, &newBufferAllocated));

    const angle::Format &copyFormat =
        GetFormatFromFormatType(formatInfo.internalFormat, formatInfo.type);
    PackPixelsParams params(clippedRectangle, copyFormat, static_cast<GLuint>(outputRowPitch),
                            isViewportFlipEnabled, nullptr, 0);

    // 2- copy the source image region to the pixel buffer using a cpu readback
    if (loadFunction.requiresConversion)
    {
        // When a conversion is required, we need to use the loadFunction to read from a temporary
        // buffer instead so its an even slower path.
        size_t bufferSize =
            storageFormat.pixelBytes * clippedRectangle.width * clippedRectangle.height;
        angle::MemoryBuffer *memoryBuffer = nullptr;
        ANGLE_VK_CHECK_ALLOC(contextVk, context->getScratchBuffer(bufferSize, &memoryBuffer));

        // Read into the scratch buffer
        ANGLE_TRY(framebufferVk->readPixelsImpl(
            contextVk, clippedRectangle, params, VK_IMAGE_ASPECT_COLOR_BIT,
            framebufferVk->getColorReadRenderTarget(), memoryBuffer->data()));

        // Load from scratch buffer to our pixel buffer
        loadFunction.loadFunction(clippedRectangle.width, clippedRectangle.height, 1,
                                  memoryBuffer->data(), outputRowPitch, 0, stagingPointer,
                                  outputRowPitch, 0);
    }
    else
    {
        // We read directly from the framebuffer into our pixel buffer.
        ANGLE_TRY(framebufferVk->readPixelsImpl(
            contextVk, clippedRectangle, params, VK_IMAGE_ASPECT_COLOR_BIT,
            framebufferVk->getColorReadRenderTarget(), stagingPointer));
    }

    // 3- enqueue the destination image subresource update
    VkBufferImageCopy copyToImage               = {};
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
    return angle::Result::Continue();
}

angle::Result PixelBuffer::allocate(ContextVk *contextVk,
                                    size_t sizeInBytes,
                                    uint8_t **ptrOut,
                                    VkBuffer *handleOut,
                                    VkDeviceSize *offsetOut,
                                    bool *newBufferAllocatedOut)
{
    return mStagingBuffer.allocate(contextVk, sizeInBytes, ptrOut, handleOut, offsetOut,
                                   newBufferAllocatedOut);
}

angle::Result PixelBuffer::flushUpdatesToImage(ContextVk *contextVk,
                                               uint32_t levelCount,
                                               vk::ImageHelper *image,
                                               vk::CommandBuffer *commandBuffer)
{
    if (mSubresourceUpdates.empty())
    {
        return angle::Result::Continue();
    }

    ANGLE_TRY(mStagingBuffer.flush(contextVk));

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
        mStagingBuffer.releaseRetainedBuffers(contextVk->getRenderer());
    }
    else
    {
        WARN() << "Internal Vulkan bufffer could not be released. This is likely due to having "
                  "extra images defined in the Texture.";
    }

    return angle::Result::Continue();
}

bool PixelBuffer::empty() const
{
    return mSubresourceUpdates.empty();
}

angle::Result PixelBuffer::stageSubresourceUpdateAndGetData(ContextVk *contextVk,
                                                            size_t allocationSize,
                                                            const gl::ImageIndex &imageIndex,
                                                            const gl::Extents &extents,
                                                            const gl::Offset &offset,
                                                            uint8_t **destData)
{
    VkBuffer bufferHandle;
    VkDeviceSize stagingOffset = 0;
    bool newBufferAllocated    = false;
    ANGLE_TRY(mStagingBuffer.allocate(contextVk, allocationSize, destData, &bufferHandle,
                                      &stagingOffset, &newBufferAllocated));

    VkBufferImageCopy copy               = {};
    copy.bufferOffset                    = stagingOffset;
    copy.bufferRowLength                 = extents.width;
    copy.bufferImageHeight               = extents.height;
    copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel       = imageIndex.getLevelIndex();
    copy.imageSubresource.baseArrayLayer = imageIndex.hasLayer() ? imageIndex.getLayerIndex() : 0;
    copy.imageSubresource.layerCount     = imageIndex.getLayerCount();

    gl_vk::GetOffset(offset, &copy.imageOffset);
    gl_vk::GetExtent(extents, &copy.imageExtent);

    mSubresourceUpdates.emplace_back(bufferHandle, copy);

    return angle::Result::Continue();
}

angle::Result TextureVk::generateMipmapLevelsWithCPU(ContextVk *contextVk,
                                                     const angle::Format &sourceFormat,
                                                     GLuint layer,
                                                     GLuint firstMipLevel,
                                                     GLuint maxMipLevel,
                                                     const size_t sourceWidth,
                                                     const size_t sourceHeight,
                                                     const size_t sourceRowPitch,
                                                     uint8_t *sourceData)
{
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
            contextVk, mipAllocationSize,
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

    return angle::Result::Continue();
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
    : TextureImpl(state), mRenderTarget(&mImage, &mBaseLevelImageView, 0), mPixelBuffer(renderer)
{
}

TextureVk::~TextureVk() = default;

gl::Error TextureVk::onDestroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    releaseImage(context, renderer);
    renderer->releaseObject(renderer->getCurrentQueueSerial(), &mSampler);

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

    // Convert internalFormat to sized internal format.
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat, type);

    ANGLE_TRY(redefineImage(context, index, formatInfo, size));

    // Early-out on empty textures, don't create a zero-sized storage.
    if (size.empty())
    {
        return gl::NoError();
    }

    // Create a new graph node to store image initialization commands.
    mImage.finishCurrentCommands(renderer);

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
                                 gl::Buffer *unpackBuffer,
                                 const uint8_t *pixels)
{
    ContextVk *contextVk                 = vk::GetImpl(context);
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(format, type);
    ANGLE_TRY(mPixelBuffer.stageSubresourceUpdate(
        contextVk, index, gl::Extents(area.width, area.height, area.depth),
        gl::Offset(area.x, area.y, area.z), formatInfo, unpack, type, pixels));

    // Create a new graph node to store image initialization commands.
    mImage.finishCurrentCommands(contextVk->getRenderer());

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
    ANGLE_TRY(redefineImage(context, index, internalFormatInfo, newImageSize));
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

gl::Error TextureVk::copyTexture(const gl::Context *context,
                                 const gl::ImageIndex &index,
                                 GLenum internalFormat,
                                 GLenum type,
                                 size_t sourceLevel,
                                 bool unpackFlipY,
                                 bool unpackPremultiplyAlpha,
                                 bool unpackUnmultiplyAlpha,
                                 const gl::Texture *source)
{
    TextureVk *sourceVk = vk::GetImpl(source);
    const gl::ImageDesc &sourceImageDesc =
        sourceVk->mState.getImageDesc(NonCubeTextureTypeToTarget(source->getType()), sourceLevel);
    gl::Rectangle sourceArea(0, 0, sourceImageDesc.size.width, sourceImageDesc.size.height);

    const gl::InternalFormat &destFormatInfo = gl::GetInternalFormatInfo(internalFormat, type);

    ANGLE_TRY(redefineImage(context, index, destFormatInfo, sourceImageDesc.size));

    return copySubTextureImpl(vk::GetImpl(context), index, gl::kOffsetZero, destFormatInfo,
                              sourceLevel, sourceArea, unpackFlipY, unpackPremultiplyAlpha,
                              unpackUnmultiplyAlpha, sourceVk);
}

gl::Error TextureVk::copySubTexture(const gl::Context *context,
                                    const gl::ImageIndex &index,
                                    const gl::Offset &destOffset,
                                    size_t sourceLevel,
                                    const gl::Box &sourceBox,
                                    bool unpackFlipY,
                                    bool unpackPremultiplyAlpha,
                                    bool unpackUnmultiplyAlpha,
                                    const gl::Texture *source)
{
    gl::TextureTarget target                 = index.getTarget();
    size_t level                             = static_cast<size_t>(index.getLevelIndex());
    const gl::InternalFormat &destFormatInfo = *mState.getImageDesc(target, level).format.info;
    return copySubTextureImpl(vk::GetImpl(context), index, destOffset, destFormatInfo, sourceLevel,
                              sourceBox.toRect(), unpackFlipY, unpackPremultiplyAlpha,
                              unpackUnmultiplyAlpha, vk::GetImpl(source));
}

angle::Result TextureVk::copySubImageImpl(const gl::Context *context,
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
        return angle::Result::Continue();
    }

    const gl::Offset modifiedDestOffset(destOffset.x + sourceArea.x - sourceArea.x,
                                        destOffset.y + sourceArea.y - sourceArea.y, 0);

    ContextVk *contextVk         = vk::GetImpl(context);
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

    mImage.finishCurrentCommands(renderer);
    framebufferVk->getFramebuffer()->addReadDependency(&mImage);
    return angle::Result::Continue();
}

gl::Error TextureVk::copySubTextureImpl(ContextVk *contextVk,
                                        const gl::ImageIndex &index,
                                        const gl::Offset &destOffset,
                                        const gl::InternalFormat &destFormat,
                                        size_t sourceLevel,
                                        const gl::Rectangle &sourceArea,
                                        bool unpackFlipY,
                                        bool unpackPremultiplyAlpha,
                                        bool unpackUnmultiplyAlpha,
                                        TextureVk *source)
{
    RendererVk *renderer = contextVk->getRenderer();

    if (sourceLevel != 0)
    {
        WARN() << "glCopyTextureCHROMIUM with sourceLevel != 0 not implemented.";
        return angle::Result::Stop();
    }

    // Read back the requested region of the source texture
    uint8_t *sourceData = nullptr;
    ANGLE_TRY(source->copyImageDataToBuffer(contextVk, sourceLevel, 1, sourceArea, &sourceData));

    // Using the front-end ANGLE format for the colorRead and colorWrite functions.  Otherwise
    // emulated formats like luminance-alpha would not know how to interpret the data.
    const angle::Format &sourceAngleFormat = source->getImage().getFormat().angleFormat();
    const angle::Format &destAngleFormat =
        renderer->getFormat(destFormat.sizedInternalFormat).angleFormat();
    size_t destinationAllocationSize =
        sourceArea.width * sourceArea.height * destAngleFormat.pixelBytes;

    // Allocate memory in the destination texture for the copy/conversion
    uint8_t *destData = nullptr;
    ANGLE_TRY(mPixelBuffer.stageSubresourceUpdateAndGetData(
        contextVk, destinationAllocationSize, index,
        gl::Extents(sourceArea.width, sourceArea.height, 1), destOffset, &destData));

    // Source and dest data is tightly packed
    GLuint sourceDataRowPitch = sourceArea.width * sourceAngleFormat.pixelBytes;
    GLuint destDataRowPitch   = sourceArea.width * destAngleFormat.pixelBytes;

    CopyImageCHROMIUM(sourceData, sourceDataRowPitch, sourceAngleFormat.pixelBytes, 0,
                      sourceAngleFormat.pixelReadFunction, destData, destDataRowPitch,
                      destAngleFormat.pixelBytes, 0, destAngleFormat.pixelWriteFunction,
                      destFormat.format, destFormat.componentType, sourceArea.width,
                      sourceArea.height, 1, unpackFlipY, unpackPremultiplyAlpha,
                      unpackUnmultiplyAlpha);

    // Create a new graph node to store image initialization commands.
    mImage.finishCurrentCommands(contextVk->getRenderer());

    return angle::Result::Continue();
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
    ANGLE_TRY(mImage.recordCommands(contextVk, &commandBuffer));

    if (mImage.valid())
    {
        releaseImage(context, renderer);
    }

    ANGLE_TRY(initImage(contextVk, format, size, static_cast<uint32_t>(levels), commandBuffer));
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

angle::Result TextureVk::redefineImage(const gl::Context *context,
                                       const gl::ImageIndex &index,
                                       const gl::InternalFormat &internalFormat,
                                       const gl::Extents &size)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    // If there is any staged changes for this index, we can remove them since we're going to
    // override them with this call.
    mPixelBuffer.removeStagedUpdates(index);

    if (mImage.valid())
    {
        const vk::Format &vkFormat = renderer->getFormat(internalFormat.sizedInternalFormat);

        // Calculate the expected size for the index we are defining. If the size is different from
        // the given size, or the format is different, we are redefining the image so we must
        // release it.
        if (mImage.getFormat() != vkFormat || size != mImage.getSize(index))
        {
            releaseImage(context, renderer);
        }
    }

    return angle::Result::Continue();
}

angle::Result TextureVk::copyImageDataToBuffer(ContextVk *contextVk,
                                               size_t sourceLevel,
                                               uint32_t layerCount,
                                               const gl::Rectangle &sourceArea,
                                               uint8_t **outDataPtr)
{
    // Make sure the source is initialized and it's images are flushed.
    ANGLE_TRY(ensureImageInitialized(contextVk));

    const angle::Format &imageFormat = getImage().getFormat().textureFormat();
    size_t sourceCopyAllocationSize = sourceArea.width * sourceArea.height * imageFormat.pixelBytes;

    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(mImage.recordCommands(contextVk, &commandBuffer));

    // Requirement of the copyImageToBuffer, the source image must be in SRC_OPTIMAL layout.
    mImage.changeLayoutWithStages(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                  VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    // Allocate enough memory to copy the sourceArea region of the source texture into its pixel
    // buffer.
    bool newBufferAllocated       = false;
    VkBuffer copyBufferHandle     = VK_NULL_HANDLE;
    VkDeviceSize sourceCopyOffset = 0;
    ANGLE_TRY(mPixelBuffer.allocate(contextVk, sourceCopyAllocationSize, outDataPtr,
                                    &copyBufferHandle, &sourceCopyOffset, &newBufferAllocated));

    VkBufferImageCopy region               = {};
    region.bufferOffset                    = sourceCopyOffset;
    region.bufferRowLength                 = 0;
    region.bufferImageHeight               = 0;
    region.imageExtent.width               = sourceArea.width;
    region.imageExtent.height              = sourceArea.height;
    region.imageExtent.depth               = 1;
    region.imageOffset.x                   = sourceArea.x;
    region.imageOffset.y                   = sourceArea.y;
    region.imageOffset.z                   = 0;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = layerCount;
    region.imageSubresource.mipLevel       = static_cast<uint32_t>(sourceLevel);

    commandBuffer->copyImageToBuffer(mImage.getImage(), mImage.getCurrentLayout(), copyBufferHandle,
                                     1, &region);

    // Explicitly finish. If new use cases arise where we don't want to block we can change this.
    ANGLE_TRY(contextVk->getRenderer()->finish(contextVk));

    return angle::Result::Continue();
}

angle::Result TextureVk::generateMipmapsWithCPU(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    const gl::Extents baseLevelExtents = mImage.getExtents();
    uint32_t imageLayerCount           = GetImageLayerCount(mState.getType());

    uint8_t *imageData = nullptr;
    gl::Rectangle imageArea(0, 0, baseLevelExtents.width, baseLevelExtents.height);
    ANGLE_TRY(copyImageDataToBuffer(contextVk, mState.getEffectiveBaseLevel(), imageLayerCount,
                                    imageArea, &imageData));

    const angle::Format &angleFormat = mImage.getFormat().textureFormat();
    GLuint sourceRowPitch            = baseLevelExtents.width * angleFormat.pixelBytes;
    size_t baseLevelAllocationSize   = sourceRowPitch * baseLevelExtents.height;

    // We now have the base level available to be manipulated in the imageData pointer. Generate all
    // the missing mipmaps with the slow path. For each layer, use the copied data to generate all
    // the mips.
    for (GLuint layer = 0; layer < imageLayerCount; layer++)
    {
        size_t bufferOffset = layer * baseLevelAllocationSize;

        ANGLE_TRY(generateMipmapLevelsWithCPU(
            contextVk, angleFormat, layer, mState.getEffectiveBaseLevel() + 1,
            mState.getMipmapMaxLevel(), baseLevelExtents.width, baseLevelExtents.height,
            sourceRowPitch, imageData + bufferOffset));
    }

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(mImage.recordCommands(contextVk, &commandBuffer));
    return mPixelBuffer.flushUpdatesToImage(contextVk, getLevelCount(), &mImage, commandBuffer);
}

gl::Error TextureVk::generateMipmap(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // Some data is pending, or the image has not been defined at all yet
    if (!mImage.valid())
    {
        // lets initialize the image so we can generate the next levels.
        if (!mPixelBuffer.empty())
        {
            ANGLE_TRY(ensureImageInitialized(contextVk));
            ASSERT(mImage.valid());
        }
        else
        {
            // There is nothing to generate if there is nothing uploaded so far.
            return gl::NoError();
        }
    }

    RendererVk *renderer = contextVk->getRenderer();
    VkFormatProperties imageProperties;
    vk::GetFormatProperties(renderer->getPhysicalDevice(), mImage.getFormat().vkTextureFormat,
                            &imageProperties);

    // Check if the image supports blit. If it does, we can do the mipmap generation on the gpu
    // only.
    if (IsMaskFlagSet(kBlitFeatureFlags, imageProperties.linearTilingFeatures))
    {
        ANGLE_TRY(ensureImageInitialized(contextVk));
        ANGLE_TRY(mImage.generateMipmapsWithBlit(contextVk, mState.getMipmapMaxLevel()));
    }
    else
    {
        ANGLE_TRY(generateMipmapsWithCPU(context));
    }

    // We're changing this textureVk content, make sure we let the graph know.
    mImage.finishCurrentCommands(renderer);

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

angle::Result TextureVk::getAttachmentRenderTarget(const gl::Context *context,
                                                   GLenum binding,
                                                   const gl::ImageIndex &imageIndex,
                                                   FramebufferAttachmentRenderTarget **rtOut)
{
    // Non-zero mip level attachments are an ES 3.0 feature.
    ASSERT(imageIndex.getLevelIndex() == 0);

    ContextVk *contextVk = vk::GetImpl(context);
    ANGLE_TRY(ensureImageInitialized(contextVk));

    switch (imageIndex.getType())
    {
        case gl::TextureType::_2D:
            *rtOut = &mRenderTarget;
            break;
        case gl::TextureType::CubeMap:
            ANGLE_TRY(initCubeMapRenderTargets(contextVk));
            *rtOut = &mCubeMapRenderTargets[imageIndex.cubeMapFaceIndex()];
            break;
        default:
            UNREACHABLE();
    }

    return angle::Result::Continue();
}

angle::Result TextureVk::ensureImageInitialized(ContextVk *contextVk)
{
    if (mImage.valid() && mPixelBuffer.empty())
    {
        return angle::Result::Continue();
    }
    RendererVk *renderer             = contextVk->getRenderer();
    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(mImage.recordCommands(contextVk, &commandBuffer));

    const gl::ImageDesc &baseLevelDesc  = mState.getBaseLevelDesc();
    const gl::Extents &baseLevelExtents = baseLevelDesc.size;
    const uint32_t levelCount           = getLevelCount();

    if (!mImage.valid())
    {
        const vk::Format &format =
            renderer->getFormat(baseLevelDesc.format.info->sizedInternalFormat);

        ANGLE_TRY(initImage(contextVk, format, baseLevelExtents, levelCount, commandBuffer));
    }

    return mPixelBuffer.flushUpdatesToImage(contextVk, levelCount, &mImage, commandBuffer);
}

angle::Result TextureVk::initCubeMapRenderTargets(ContextVk *contextVk)
{
    // Lazy init. Check if already initialized.
    if (!mCubeMapFaceImageViews.empty())
        return angle::Result::Continue();

    mCubeMapFaceImageViews.resize(gl::kCubeFaceCount);

    for (size_t cubeMapFaceIndex = 0; cubeMapFaceIndex < gl::kCubeFaceCount; ++cubeMapFaceIndex)
    {
        vk::ImageView &imageView = mCubeMapFaceImageViews[cubeMapFaceIndex];
        ANGLE_TRY(mImage.initLayerImageView(contextVk, gl::TextureType::CubeMap,
                                            VK_IMAGE_ASPECT_COLOR_BIT, gl::SwizzleState(),
                                            &imageView, 1, cubeMapFaceIndex, 1));
        mCubeMapRenderTargets.emplace_back(&mImage, &imageView, cubeMapFaceIndex);
    }
    return angle::Result::Continue();
}

angle::Result TextureVk::syncState(const gl::Context *context,
                                   const gl::Texture::DirtyBits &dirtyBits)
{
    if (dirtyBits.none() && mSampler.valid())
    {
        return angle::Result::Continue();
    }

    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();
    if (mSampler.valid())
    {
        renderer->releaseObject(renderer->getCurrentQueueSerial(), &mSampler);
    }

    const gl::Extensions &extensions     = renderer->getNativeExtensions();
    const gl::SamplerState &samplerState = mState.getSamplerState();

    float maxAnisotropy   = samplerState.getMaxAnisotropy();
    bool anisotropyEnable = extensions.textureFilterAnisotropic && maxAnisotropy > 1.0f;

    // Create a simple sampler. Force basic parameter settings.
    VkSamplerCreateInfo samplerInfo     = {};
    samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.flags                   = 0;
    samplerInfo.magFilter               = gl_vk::GetFilter(samplerState.getMagFilter());
    samplerInfo.minFilter               = gl_vk::GetFilter(samplerState.getMinFilter());
    samplerInfo.mipmapMode              = gl_vk::GetSamplerMipmapMode(samplerState.getMinFilter());
    samplerInfo.addressModeU            = gl_vk::GetSamplerAddressMode(samplerState.getWrapS());
    samplerInfo.addressModeV            = gl_vk::GetSamplerAddressMode(samplerState.getWrapT());
    samplerInfo.addressModeW            = gl_vk::GetSamplerAddressMode(samplerState.getWrapR());
    samplerInfo.mipLodBias              = 0.0f;
    samplerInfo.anisotropyEnable        = anisotropyEnable;
    samplerInfo.maxAnisotropy           = maxAnisotropy;
    samplerInfo.compareEnable           = VK_FALSE;
    samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
    samplerInfo.minLod                  = samplerState.getMinLod();
    samplerInfo.maxLod                  = samplerState.getMaxLod();
    samplerInfo.borderColor             = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    ANGLE_VK_TRY(contextVk, mSampler.init(contextVk->getDevice(), samplerInfo));
    return angle::Result::Continue();
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

angle::Result TextureVk::initializeContents(const gl::Context *context,
                                            const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return angle::Result::Continue();
}

const vk::ImageView &TextureVk::getImageView() const
{
    ASSERT(mImage.valid());

    const GLenum minFilter = mState.getSamplerState().getMinFilter();
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

angle::Result TextureVk::initImage(ContextVk *contextVk,
                                   const vk::Format &format,
                                   const gl::Extents &extents,
                                   const uint32_t levelCount,
                                   vk::CommandBuffer *commandBuffer)
{
    const RendererVk *renderer = contextVk->getRenderer();

    const VkImageUsageFlags usage =
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    ANGLE_TRY(mImage.init(contextVk, mState.getType(), extents, format, 1, usage, levelCount));

    const VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    ANGLE_TRY(mImage.initMemory(contextVk, renderer->getMemoryProperties(), flags));

    gl::SwizzleState mappedSwizzle;
    MapSwizzleState(format.internalFormat, mState.getSwizzleState(), &mappedSwizzle);

    // Renderable textures cannot have a swizzle.
    ASSERT(!contextVk->getTextureCaps().get(format.internalFormat).textureAttachment ||
           !mappedSwizzle.swizzleRequired());

    // TODO(jmadill): Separate imageviews for RenderTargets and Sampling.
    ANGLE_TRY(mImage.initImageView(contextVk, mState.getType(), VK_IMAGE_ASPECT_COLOR_BIT,
                                   mappedSwizzle, &mMipmapImageView, levelCount));
    ANGLE_TRY(mImage.initImageView(contextVk, mState.getType(), VK_IMAGE_ASPECT_COLOR_BIT,
                                   mappedSwizzle, &mBaseLevelImageView, 1));

    // TODO(jmadill): Fold this into the RenderPass load/store ops. http://anglebug.com/2361
    VkClearColorValue black = {{0, 0, 0, 1.0f}};
    mImage.clearColor(black, 0, levelCount, commandBuffer);
    return angle::Result::Continue();
}

void TextureVk::releaseImage(const gl::Context *context, RendererVk *renderer)
{
    mImage.release(renderer);

    Serial currentSerial = renderer->getCurrentQueueSerial();

    renderer->releaseObject(currentSerial, &mBaseLevelImageView);
    renderer->releaseObject(currentSerial, &mMipmapImageView);

    for (vk::ImageView &imageView : mCubeMapFaceImageViews)
    {
        renderer->releaseObject(currentSerial, &imageView);
    }
    mCubeMapFaceImageViews.clear();
    mCubeMapRenderTargets.clear();
}

uint32_t TextureVk::getLevelCount() const
{
    ASSERT(mState.getEffectiveBaseLevel() == 0);

    // getMipmapMaxLevel will be 0 here if mipmaps are not used, so the levelCount is always +1.
    return mState.getMipmapMaxLevel() + 1;
}
}  // namespace rx
