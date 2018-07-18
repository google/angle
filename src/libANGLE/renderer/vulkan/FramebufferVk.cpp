//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferVk.cpp:
//    Implements the class methods for FramebufferVk.
//

#include "libANGLE/renderer/vulkan/FramebufferVk.h"

#include <vulkan/vulkan.h>
#include <array>

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/SurfaceVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"

namespace rx
{

namespace
{
constexpr size_t kMinReadPixelsBufferSize = 128000;

const gl::InternalFormat &GetReadAttachmentInfo(const gl::Context *context,
                                                RenderTargetVk *renderTarget)
{
    GLenum implFormat =
        renderTarget->getImageFormat().textureFormat().fboImplementationInternalFormat;
    return gl::GetSizedInternalFormatInfo(implFormat);
}

bool HasSrcAndDstBlitProperties(const VkPhysicalDevice &physicalDevice,
                                RenderTargetVk *srcRenderTarget,
                                RenderTargetVk *dstRenderTarget)
{
    VkFormatProperties drawImageProperties;
    vk::GetFormatProperties(physicalDevice, srcRenderTarget->getImageFormat().vkTextureFormat,
                            &drawImageProperties);

    VkFormatProperties readImageProperties;
    vk::GetFormatProperties(physicalDevice, dstRenderTarget->getImageFormat().vkTextureFormat,
                            &readImageProperties);

    // Verifies if the draw and read images have the necessary prerequisites for blitting.
    return (IsMaskFlagSet<VkFormatFeatureFlags>(drawImageProperties.optimalTilingFeatures,
                                                VK_FORMAT_FEATURE_BLIT_DST_BIT) &&
            IsMaskFlagSet<VkFormatFeatureFlags>(readImageProperties.optimalTilingFeatures,
                                                VK_FORMAT_FEATURE_BLIT_SRC_BIT));
}
}  // anonymous namespace

// static
FramebufferVk *FramebufferVk::CreateUserFBO(RendererVk *renderer, const gl::FramebufferState &state)
{
    return new FramebufferVk(renderer, state, nullptr);
}

// static
FramebufferVk *FramebufferVk::CreateDefaultFBO(RendererVk *renderer,
                                               const gl::FramebufferState &state,
                                               WindowSurfaceVk *backbuffer)
{
    return new FramebufferVk(renderer, state, backbuffer);
}

FramebufferVk::FramebufferVk(RendererVk *renderer,
                             const gl::FramebufferState &state,
                             WindowSurfaceVk *backbuffer)
    : FramebufferImpl(state),
      mBackbuffer(backbuffer),
      mActiveColorComponents(0),
      mReadPixelsBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, kMinReadPixelsBufferSize),
      mBlitPixelBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, kMinReadPixelsBufferSize)
{
    mBlitPixelBuffer.init(1, renderer);
    ASSERT(mBlitPixelBuffer.valid());

    mReadPixelsBuffer.init(1, renderer);
    ASSERT(mReadPixelsBuffer.valid());
}

FramebufferVk::~FramebufferVk() = default;

void FramebufferVk::destroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();
    renderer->releaseObject(getStoredQueueSerial(), &mFramebuffer);

    mReadPixelsBuffer.destroy(contextVk->getDevice());
    mBlitPixelBuffer.destroy(contextVk->getDevice());
}

gl::Error FramebufferVk::discard(const gl::Context *context,
                                 size_t count,
                                 const GLenum *attachments)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::invalidate(const gl::Context *context,
                                    size_t count,
                                    const GLenum *attachments)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::invalidateSub(const gl::Context *context,
                                       size_t count,
                                       const GLenum *attachments,
                                       const gl::Rectangle &area)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clear(const gl::Context *context, GLbitfield mask)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // This command buffer is only started once.
    vk::CommandBuffer *commandBuffer = nullptr;

    const gl::FramebufferAttachment *depthAttachment = mState.getDepthAttachment();
    bool clearDepth = (depthAttachment && (mask & GL_DEPTH_BUFFER_BIT) != 0);
    ASSERT(!clearDepth || depthAttachment->isAttached());

    const gl::FramebufferAttachment *stencilAttachment = mState.getStencilAttachment();
    bool clearStencil = (stencilAttachment && (mask & GL_STENCIL_BUFFER_BIT) != 0);
    ASSERT(!clearStencil || stencilAttachment->isAttached());

    bool clearColor = IsMaskFlagSet(static_cast<int>(mask), GL_COLOR_BUFFER_BIT);

    const gl::FramebufferAttachment *depthStencilAttachment = mState.getDepthStencilAttachment();
    const gl::State &glState                                = context->getGLState();

    // The most costly clear mode is when we need to mask out specific color channels. This can
    // only be done with a draw call. The scissor region however can easily be integrated with
    // this method. Similarly for depth/stencil clear.
    VkColorComponentFlags colorMaskFlags = contextVk->getClearColorMask();
    if (clearColor && (mActiveColorComponents & colorMaskFlags) != mActiveColorComponents)
    {
        ANGLE_TRY(clearWithDraw(contextVk, colorMaskFlags));

        // Stencil clears must be handled separately. The only way to write out a stencil value from
        // a fragment shader in Vulkan is with VK_EXT_shader_stencil_export. Support for this
        // extension is sparse. Hence, we call into the RenderPass clear path. We similarly clear
        // depth to keep the code simple, but depth clears could be combined with the masked color
        // clears as an optimization.

        if (clearDepth || clearStencil)
        {
            // Masked stencil clears are currently not implemented.
            // TODO(jmadill): Masked stencil clear. http://anglebug.com/2540
            ANGLE_TRY(clearWithClearAttachments(contextVk, false, clearDepth, clearStencil));
        }
        return gl::NoError();
    }

    // If we clear the depth OR the stencil but not both, and we have a packed depth stencil
    // attachment, we need to use clearAttachment instead of clearDepthStencil since Vulkan won't
    // allow us to clear one or the other separately.
    bool isSingleClearOnPackedDepthStencilAttachment =
        depthStencilAttachment && (clearDepth != clearStencil);
    if (glState.isScissorTestEnabled() || isSingleClearOnPackedDepthStencilAttachment)
    {
        // With scissor test enabled, we clear very differently and we don't need to access
        // the image inside each attachment we can just use clearCmdAttachments with our
        // scissor region instead.

        // Masked stencil clears are currently not implemented.
        // TODO(jmadill): Masked stencil clear. http://anglebug.com/2540
        ANGLE_TRY(clearWithClearAttachments(contextVk, clearColor, clearDepth, clearStencil));
        return gl::NoError();
    }

    // Standard Depth/stencil clear without scissor.
    if (clearDepth || clearStencil)
    {
        ANGLE_TRY(beginWriteResource(contextVk, &commandBuffer));

        const VkClearDepthStencilValue &clearDepthStencilValue =
            contextVk->getClearDepthStencilValue().depthStencil;

        RenderTargetVk *renderTarget         = mRenderTargetCache.getDepthStencil();
        const angle::Format &format          = renderTarget->getImageFormat().textureFormat();
        const VkImageAspectFlags aspectFlags = vk::GetDepthStencilAspectFlags(format);

        vk::ImageHelper *image = renderTarget->getImageForWrite(this);
        image->clearDepthStencil(aspectFlags, clearDepthStencilValue, commandBuffer);
    }

    if (!clearColor)
    {
        return gl::NoError();
    }

    const auto *attachment = mState.getFirstNonNullAttachment();
    ASSERT(attachment && attachment->isAttached());

    if (!commandBuffer)
    {
        ANGLE_TRY(beginWriteResource(contextVk, &commandBuffer));
    }

    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets           = mRenderTargetCache.getColors();
    const VkClearColorValue &clearColorValue = contextVk->getClearColorValue().color;
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        VkClearColorValue modifiedClearColorValue = clearColorValue;
        RenderTargetVk *colorRenderTarget         = colorRenderTargets[colorIndex];

        // Its possible we're clearing a render target that has no alpha channel but we represent it
        // with a texture that has one. We must not affect its alpha channel no matter what the
        // clear value is in that case.
        if (mEmulatedAlphaAttachmentMask[colorIndex])
        {
            modifiedClearColorValue.float32[3] = 1.0;
        }

        ASSERT(colorRenderTarget);
        vk::ImageHelper *image = colorRenderTarget->getImageForWrite(this);
        GLint mipLevelToClear  = (attachment->type() == GL_TEXTURE) ? attachment->mipLevel() : 0;
        image->clearColor(modifiedClearColorValue, mipLevelToClear, 1, commandBuffer);
    }

    return gl::NoError();
}

gl::Error FramebufferVk::clearBufferfv(const gl::Context *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       const GLfloat *values)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clearBufferuiv(const gl::Context *context,
                                        GLenum buffer,
                                        GLint drawbuffer,
                                        const GLuint *values)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clearBufferiv(const gl::Context *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       const GLint *values)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clearBufferfi(const gl::Context *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       GLfloat depth,
                                       GLint stencil)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

GLenum FramebufferVk::getImplementationColorReadFormat(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, mRenderTargetCache.getColorRead(mState)).format;
}

GLenum FramebufferVk::getImplementationColorReadType(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, mRenderTargetCache.getColorRead(mState)).type;
}

gl::Error FramebufferVk::readPixels(const gl::Context *context,
                                    const gl::Rectangle &area,
                                    GLenum format,
                                    GLenum type,
                                    void *pixels)
{
    // Clip read area to framebuffer.
    const gl::Extents &fbSize = getState().getReadAttachment()->getSize();
    const gl::Rectangle fbRect(0, 0, fbSize.width, fbSize.height);
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    gl::Rectangle clippedArea;
    if (!ClipRectangle(area, fbRect, &clippedArea))
    {
        // nothing to read
        return gl::NoError();
    }
    gl::Rectangle flippedArea = clippedArea;
    if (contextVk->isViewportFlipEnabledForReadFBO())
    {
        flippedArea.y = fbRect.height - flippedArea.y - flippedArea.height;
    }

    const gl::State &glState = context->getGLState();

    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(beginWriteResource(contextVk, &commandBuffer));

    gl::PixelPackState packState(glState.getPackState());
    if (contextVk->isViewportFlipEnabledForReadFBO())
    {
        packState.reverseRowOrder = !packState.reverseRowOrder;
    }

    const gl::InternalFormat &sizedFormatInfo = gl::GetInternalFormatInfo(format, type);

    GLuint outputPitch = 0;
    ANGLE_TRY_CHECKED_MATH(sizedFormatInfo.computeRowPitch(type, area.width, packState.alignment,
                                                           packState.rowLength, &outputPitch));
    GLuint outputSkipBytes = 0;
    ANGLE_TRY_CHECKED_MATH(
        sizedFormatInfo.computeSkipBytes(type, outputPitch, 0, packState, false, &outputSkipBytes));

    outputSkipBytes += (clippedArea.x - area.x) * sizedFormatInfo.pixelBytes +
                       (clippedArea.y - area.y) * outputPitch;

    PackPixelsParams params;
    params.area        = flippedArea;
    params.format      = format;
    params.type        = type;
    params.outputPitch = outputPitch;
    params.packBuffer  = glState.getTargetBuffer(gl::BufferBinding::PixelPack);
    params.pack        = packState;

    ANGLE_TRY(readPixelsImpl(contextVk, flippedArea, params, VK_IMAGE_ASPECT_COLOR_BIT,
                             getColorReadRenderTarget(),
                             static_cast<uint8_t *>(pixels) + outputSkipBytes));
    mReadPixelsBuffer.releaseRetainedBuffers(renderer);
    return gl::NoError();
}

RenderTargetVk *FramebufferVk::getDepthStencilRenderTarget() const
{
    return mRenderTargetCache.getDepthStencil();
}

void FramebufferVk::blitUsingCopy(vk::CommandBuffer *commandBuffer,
                                  const gl::Rectangle &readArea,
                                  const gl::Rectangle &destArea,
                                  RenderTargetVk *readRenderTarget,
                                  RenderTargetVk *drawRenderTarget,
                                  const gl::Rectangle *scissor,
                                  bool blitDepthBuffer,
                                  bool blitStencilBuffer)
{
    gl::Rectangle scissoredDrawRect = destArea;
    gl::Rectangle scissoredReadRect = readArea;

    if (scissor)
    {
        if (!ClipRectangle(destArea, *scissor, &scissoredDrawRect))
        {
            return;
        }

        if (!ClipRectangle(readArea, *scissor, &scissoredReadRect))
        {
            return;
        }
    }

    const gl::Extents sourceFrameBufferExtents = readRenderTarget->getImageExtents();
    const gl::Extents drawFrameBufferExtents   = drawRenderTarget->getImageExtents();

    // After cropping for the scissor, we also want to crop for the size of the buffers.
    gl::Rectangle readFrameBufferBounds(0, 0, sourceFrameBufferExtents.width,
                                        sourceFrameBufferExtents.height);
    gl::Rectangle drawFrameBufferBounds(0, 0, drawFrameBufferExtents.width,
                                        drawFrameBufferExtents.height);
    if (!ClipRectangle(scissoredReadRect, readFrameBufferBounds, &scissoredReadRect))
    {
        return;
    }

    if (!ClipRectangle(scissoredDrawRect, drawFrameBufferBounds, &scissoredDrawRect))
    {
        return;
    }

    ASSERT(readFrameBufferBounds == drawFrameBufferBounds);
    ASSERT(scissoredReadRect == readFrameBufferBounds);
    ASSERT(scissoredDrawRect == drawFrameBufferBounds);

    VkFlags aspectFlags =
        vk::GetDepthStencilAspectFlags(readRenderTarget->getImageFormat().textureFormat());
    vk::ImageHelper *readImage = readRenderTarget->getImageForRead(
        this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);
    vk::ImageHelper *writeImage = drawRenderTarget->getImageForWrite(this);
    // Requirement of the copyImageToBuffer, the dst image must be in
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
    writeImage->changeLayoutWithStages(aspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    VkImageAspectFlags aspectMask =
        vk::GetDepthStencilAspectFlagsForCopy(blitDepthBuffer, blitStencilBuffer);
    vk::ImageHelper::Copy(readImage, writeImage, gl::Offset(), gl::Offset(),
                          gl::Extents(scissoredDrawRect.width, scissoredDrawRect.height, 1),
                          aspectMask, commandBuffer);
}

RenderTargetVk *FramebufferVk::getColorReadRenderTarget() const
{
    RenderTargetVk *renderTarget = mRenderTargetCache.getColorRead(mState);
    ASSERT(renderTarget && renderTarget->getImage().valid());
    return renderTarget;
}

angle::Result FramebufferVk::blitWithReadback(ContextVk *contextVk,
                                              const gl::Rectangle &sourceArea,
                                              const gl::Rectangle &destArea,
                                              bool blitDepthBuffer,
                                              bool blitStencilBuffer,
                                              vk::CommandBuffer *commandBuffer,
                                              RenderTargetVk *readRenderTarget,
                                              RenderTargetVk *drawRenderTarget)
{
    RendererVk *renderer          = contextVk->getRenderer();
    vk::ImageHelper *imageForRead = readRenderTarget->getImageForRead(
        this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);
    const angle::Format &drawFormat = drawRenderTarget->getImageFormat().angleFormat();
    const gl::InternalFormat &sizedInternalDrawFormat =
        gl::GetSizedInternalFormatInfo(drawFormat.glInternalFormat);

    GLuint outputPitch = 0;

    if (blitStencilBuffer)
    {
        // If we're copying the stencil bits, we need to adjust the outputPitch
        // because in Vulkan, if we copy the stencil out of a any texture, the stencil
        // will be tightly packed in an S8 buffer (as specified in the spec here
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkBufferImageCopy.html)
        outputPitch = destArea.width * (drawFormat.stencilBits / 8);
    }
    else
    {
        outputPitch = destArea.width * drawFormat.pixelBytes;
    }

    PackPixelsParams packPixelsParams;
    packPixelsParams.pack.alignment       = 1;
    packPixelsParams.pack.reverseRowOrder = true;
    packPixelsParams.type                 = sizedInternalDrawFormat.type;
    packPixelsParams.format               = sizedInternalDrawFormat.format;
    packPixelsParams.area.height          = destArea.height;
    packPixelsParams.area.width           = destArea.width;
    packPixelsParams.area.x               = destArea.x;
    packPixelsParams.area.y               = destArea.y;
    packPixelsParams.outputPitch          = outputPitch;

    GLuint pixelBytes    = imageForRead->getFormat().angleFormat().pixelBytes;
    GLuint sizeToRequest = sourceArea.width * sourceArea.height * pixelBytes;

    uint8_t *copyPtr   = nullptr;
    VkBuffer handleOut = VK_NULL_HANDLE;
    uint32_t offsetOut = 0;

    VkImageAspectFlags copyFlags =
        vk::GetDepthStencilAspectFlagsForCopy(blitDepthBuffer, blitStencilBuffer);

    ANGLE_TRY(mBlitPixelBuffer.allocate(contextVk, sizeToRequest, &copyPtr, &handleOut, &offsetOut,
                                        nullptr));
    ANGLE_TRY(readPixelsImpl(contextVk, sourceArea, packPixelsParams, copyFlags, readRenderTarget,
                             copyPtr));
    ANGLE_TRY(mBlitPixelBuffer.flush(contextVk));

    // Reinitialize the commandBuffer after a read pixels because it calls
    // renderer->finish which makes command buffers obsolete.
    ANGLE_TRY(beginWriteResource(contextVk, &commandBuffer));

    // We read the bytes of the image in a buffer, now we have to copy them into the
    // destination target.
    vk::ImageHelper *imageForWrite = drawRenderTarget->getImageForWrite(this);

    imageForWrite->changeLayoutWithStages(
        imageForWrite->getAspectFlags(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    VkBufferImageCopy region;
    region.bufferOffset                    = offsetOut;
    region.bufferImageHeight               = sourceArea.height;
    region.bufferRowLength                 = sourceArea.width;
    region.imageExtent.width               = destArea.width;
    region.imageExtent.height              = destArea.height;
    region.imageExtent.depth               = 1;
    region.imageSubresource.mipLevel       = 0;
    region.imageSubresource.aspectMask     = copyFlags;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageOffset.x                   = destArea.x;
    region.imageOffset.y                   = destArea.y;
    region.imageOffset.z                   = 0;

    commandBuffer->copyBufferToImage(handleOut, imageForWrite->getImage(),
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    mBlitPixelBuffer.releaseRetainedBuffers(renderer);
    return angle::Result::Continue();
}

gl::Error FramebufferVk::blit(const gl::Context *context,
                              const gl::Rectangle &sourceArea,
                              const gl::Rectangle &destArea,
                              GLbitfield mask,
                              GLenum filter)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    const gl::State &glState                 = context->getGLState();
    const gl::Framebuffer *sourceFramebuffer = glState.getReadFramebuffer();
    const gl::Rectangle *scissor = glState.isScissorTestEnabled() ? &glState.getScissor() : nullptr;
    bool blitColorBuffer         = (mask & GL_COLOR_BUFFER_BIT) != 0;
    bool blitDepthBuffer         = (mask & GL_DEPTH_BUFFER_BIT) != 0;
    bool blitStencilBuffer       = (mask & GL_STENCIL_BUFFER_BIT) != 0;

    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(beginWriteResource(contextVk, &commandBuffer));
    FramebufferVk *sourceFramebufferVk = vk::GetImpl(sourceFramebuffer);
    bool flipSource                    = contextVk->isViewportFlipEnabledForReadFBO();
    bool flipDest                      = contextVk->isViewportFlipEnabledForDrawFBO();

    if (blitColorBuffer)
    {
        RenderTargetVk *readRenderTarget = sourceFramebufferVk->getColorReadRenderTarget();

        for (size_t colorAttachment : mState.getEnabledDrawBuffers())
        {
            RenderTargetVk *drawRenderTarget = mRenderTargetCache.getColors()[colorAttachment];
            ASSERT(drawRenderTarget);
            ASSERT(HasSrcAndDstBlitProperties(renderer->getPhysicalDevice(), readRenderTarget,
                                              drawRenderTarget));
            blitImpl(commandBuffer, sourceArea, destArea, readRenderTarget, drawRenderTarget,
                     filter, scissor, true, false, false, flipSource, flipDest);
        }
    }

    if (blitDepthBuffer || blitStencilBuffer)
    {
        RenderTargetVk *readRenderTarget = sourceFramebufferVk->getDepthStencilRenderTarget();
        ASSERT(readRenderTarget);

        RenderTargetVk *drawRenderTarget = mRenderTargetCache.getDepthStencil();

        if (HasSrcAndDstBlitProperties(renderer->getPhysicalDevice(), readRenderTarget,
                                       drawRenderTarget))
        {
            blitImpl(commandBuffer, sourceArea, destArea, readRenderTarget, drawRenderTarget,
                     filter, scissor, false, blitDepthBuffer, blitStencilBuffer, flipSource,
                     flipDest);
        }
        else
        {
            ASSERT(filter == GL_NEAREST);
            if (flipSource || flipDest)
            {
                ANGLE_TRY(blitWithReadback(contextVk, sourceArea, destArea, blitDepthBuffer,
                                           blitStencilBuffer, commandBuffer, readRenderTarget,
                                           drawRenderTarget));
            }
            else
            {
                blitUsingCopy(commandBuffer, sourceArea, destArea, readRenderTarget,
                              drawRenderTarget, scissor, blitDepthBuffer, blitStencilBuffer);
            }
        }
    }

    return gl::NoError();
}

void FramebufferVk::blitImpl(vk::CommandBuffer *commandBuffer,
                             const gl::Rectangle &readRectIn,
                             const gl::Rectangle &drawRectIn,
                             RenderTargetVk *readRenderTarget,
                             RenderTargetVk *drawRenderTarget,
                             GLenum filter,
                             const gl::Rectangle *scissor,
                             bool colorBlit,
                             bool depthBlit,
                             bool stencilBlit,
                             bool flipSource,
                             bool flipDest)
{
    // Since blitRenderbufferRect is called for each render buffer that needs to be blitted,
    // it should never be the case that both color and depth/stencil need to be blitted at
    // at the same time.
    ASSERT(colorBlit != (depthBlit || stencilBlit));

    gl::Rectangle scissoredDrawRect = drawRectIn;
    gl::Rectangle scissoredReadRect = readRectIn;

    if (scissor)
    {
        if (!ClipRectangle(drawRectIn, *scissor, &scissoredDrawRect))
        {
            return;
        }

        if (!ClipRectangle(readRectIn, *scissor, &scissoredReadRect))
        {
            return;
        }
    }

    const gl::Extents sourceFrameBufferExtents = readRenderTarget->getImageExtents();

    // After cropping for the scissor, we also want to crop for the size of the buffers.
    gl::Rectangle readFrameBufferBounds(0, 0, sourceFrameBufferExtents.width,
                                        sourceFrameBufferExtents.height);
    if (!ClipRectangle(scissoredReadRect, readFrameBufferBounds, &scissoredReadRect))
    {
        return;
    }

    const vk::Format &readImageFormat = readRenderTarget->getImageFormat();
    VkImageAspectFlags aspectMask =
        colorBlit ? VK_IMAGE_ASPECT_COLOR_BIT
                  : vk::GetDepthStencilAspectFlags(readImageFormat.textureFormat());
    vk::ImageHelper *srcImage = readRenderTarget->getImageForRead(
        this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);

    if (flipSource)
    {
        scissoredReadRect.y =
            sourceFrameBufferExtents.height - scissoredReadRect.y - scissoredReadRect.height;
    }

    VkImageBlit blit                   = {};
    blit.srcOffsets[0]                 = {scissoredReadRect.x0(),
                          flipSource ? scissoredReadRect.y1() : scissoredReadRect.y0(), 0};
    blit.srcOffsets[1]                 = {scissoredReadRect.x1(),
                          flipSource ? scissoredReadRect.y0() : scissoredReadRect.y1(), 1};
    blit.srcSubresource.aspectMask     = aspectMask;
    blit.srcSubresource.mipLevel       = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.dstSubresource.aspectMask     = aspectMask;
    blit.dstSubresource.mipLevel       = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    const gl::Extents &drawFrameBufferExtents = drawRenderTarget->getImageExtents();
    gl::Rectangle drawFrameBufferBounds(0, 0, drawFrameBufferExtents.width,
                                        drawFrameBufferExtents.height);
    if (!ClipRectangle(scissoredDrawRect, drawFrameBufferBounds, &scissoredDrawRect))
    {
        return;
    }

    if (flipDest)
    {
        scissoredDrawRect.y =
            drawFrameBufferBounds.height - scissoredDrawRect.y - scissoredDrawRect.height;
    }

    blit.dstOffsets[0] = {scissoredDrawRect.x0(),
                          flipDest ? scissoredDrawRect.y1() : scissoredDrawRect.y0(), 0};
    blit.dstOffsets[1] = {scissoredDrawRect.x1(),
                          flipDest ? scissoredDrawRect.y0() : scissoredDrawRect.y1(), 1};

    vk::ImageHelper *dstImage = drawRenderTarget->getImageForWrite(this);

    // Requirement of the copyImageToBuffer, the dst image must be in
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
    dstImage->changeLayoutWithStages(aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    commandBuffer->blitImage(srcImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             dstImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                             gl_vk::GetFilter(filter));
}

bool FramebufferVk::checkStatus(const gl::Context *context) const
{
    // if we have both a depth and stencil buffer, they must refer to the same object
    // since we only support packed_depth_stencil and not separate depth and stencil
    if (mState.hasSeparateDepthAndStencilAttachments())
    {
        return false;
    }

    return true;
}

gl::Error FramebufferVk::syncState(const gl::Context *context,
                                   const gl::Framebuffer::DirtyBits &dirtyBits)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    ASSERT(dirtyBits.any());
    for (size_t dirtyBit : dirtyBits)
    {
        switch (dirtyBit)
        {
            case gl::Framebuffer::DIRTY_BIT_DEPTH_ATTACHMENT:
            case gl::Framebuffer::DIRTY_BIT_STENCIL_ATTACHMENT:
                ANGLE_TRY(mRenderTargetCache.updateDepthStencilRenderTarget(context, mState));
                break;
            case gl::Framebuffer::DIRTY_BIT_DRAW_BUFFERS:
            case gl::Framebuffer::DIRTY_BIT_READ_BUFFER:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_WIDTH:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_HEIGHT:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_SAMPLES:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_FIXED_SAMPLE_LOCATIONS:
                break;
            default:
            {
                ASSERT(gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0 == 0 &&
                       dirtyBit < gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_MAX);
                size_t colorIndex =
                    static_cast<size_t>(dirtyBit - gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0);
                ANGLE_TRY(mRenderTargetCache.updateColorRenderTarget(context, mState, colorIndex));

                // Update cached masks for masked clears.
                RenderTargetVk *renderTarget = mRenderTargetCache.getColors()[colorIndex];
                if (renderTarget)
                {
                    const angle::Format &emulatedFormat =
                        renderTarget->getImageFormat().textureFormat();
                    updateActiveColorMasks(
                        colorIndex, emulatedFormat.redBits > 0, emulatedFormat.greenBits > 0,
                        emulatedFormat.blueBits > 0, emulatedFormat.alphaBits > 0);

                    const angle::Format &sourceFormat =
                        renderTarget->getImageFormat().angleFormat();
                    mEmulatedAlphaAttachmentMask.set(
                        colorIndex, sourceFormat.alphaBits == 0 && emulatedFormat.alphaBits > 0);

                    contextVk->updateColorMask(context->getGLState().getBlendState());
                }
                else
                {
                    updateActiveColorMasks(colorIndex, false, false, false, false);
                }
                break;
            }
        }
    }

    mActiveColorComponents = gl_vk::GetColorComponentFlags(
        mActiveColorComponentMasksForClear[0].any(), mActiveColorComponentMasksForClear[1].any(),
        mActiveColorComponentMasksForClear[2].any(), mActiveColorComponentMasksForClear[3].any());

    mRenderPassDesc.reset();
    renderer->releaseObject(getStoredQueueSerial(), &mFramebuffer);

    // Will freeze the current set of dependencies on this FBO. The next time we render we will
    // create a new entry in the command graph.
    onResourceChanged(renderer);

    contextVk->invalidateCurrentPipeline();

    return gl::NoError();
}

const vk::RenderPassDesc &FramebufferVk::getRenderPassDesc()
{
    if (mRenderPassDesc.valid())
    {
        return mRenderPassDesc.value();
    }

    vk::RenderPassDesc desc;

    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);
        desc.packColorAttachment(colorRenderTarget->getImage());
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        desc.packDepthStencilAttachment(depthStencilRenderTarget->getImage());
    }

    mRenderPassDesc = desc;
    return mRenderPassDesc.value();
}

angle::Result FramebufferVk::getFramebuffer(ContextVk *contextVk, vk::Framebuffer **framebufferOut)
{
    // If we've already created our cached Framebuffer, return it.
    if (mFramebuffer.valid())
    {
        *framebufferOut = &mFramebuffer;
        return angle::Result::Continue();
    }

    const vk::RenderPassDesc &desc = getRenderPassDesc();

    vk::RenderPass *renderPass = nullptr;
    ANGLE_TRY(contextVk->getRenderer()->getCompatibleRenderPass(contextVk, desc, &renderPass));

    // If we've a Framebuffer provided by a Surface (default FBO/backbuffer), query it.
    if (mBackbuffer)
    {
        return mBackbuffer->getCurrentFramebuffer(contextVk, *renderPass, framebufferOut);
    }

    // Gather VkImageViews over all FBO attachments, also size of attached region.
    std::vector<VkImageView> attachments;
    gl::Extents attachmentsSize;

    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);
        attachments.push_back(colorRenderTarget->getImageView()->getHandle());

        ASSERT(attachmentsSize.empty() || attachmentsSize == colorRenderTarget->getImageExtents());
        attachmentsSize = colorRenderTarget->getImageExtents();
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        attachments.push_back(depthStencilRenderTarget->getImageView()->getHandle());

        ASSERT(attachmentsSize.empty() ||
               attachmentsSize == depthStencilRenderTarget->getImageExtents());
        attachmentsSize = depthStencilRenderTarget->getImageExtents();
    }

    ASSERT(!attachments.empty());

    VkFramebufferCreateInfo framebufferInfo;

    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext           = nullptr;
    framebufferInfo.flags           = 0;
    framebufferInfo.renderPass      = renderPass->getHandle();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = static_cast<uint32_t>(attachmentsSize.width);
    framebufferInfo.height          = static_cast<uint32_t>(attachmentsSize.height);
    framebufferInfo.layers          = 1;

    ANGLE_TRY(mFramebuffer.init(contextVk, framebufferInfo));

    *framebufferOut = &mFramebuffer;
    return angle::Result::Continue();
}

angle::Result FramebufferVk::clearWithClearAttachments(ContextVk *contextVk,
                                                       bool clearColor,
                                                       bool clearDepth,
                                                       bool clearStencil)
{
    // Trigger a new command node to ensure overlapping writes happen sequentially.
    onResourceChanged(contextVk->getRenderer());

    // This command can only happen inside a render pass, so obtain one if its already happening
    // or create a new one if not.
    vk::CommandBuffer *commandBuffer = nullptr;
    vk::RecordingMode mode           = vk::RecordingMode::Start;
    ANGLE_TRY(getCommandBufferForDraw(contextVk, &commandBuffer, &mode));

    // TODO(jmadill): Cube map attachments. http://anglebug.com/2470
    // We assume for now that we always need to clear only 1 layer starting at the
    // baseArrayLayer 0, this might need to change depending how we'll implement
    // cube maps, 3d textures and array textures.
    VkClearRect clearRect;
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount     = 1;

    // When clearing, the scissor region must be clipped to the renderArea per the validation rules
    // in Vulkan.
    gl::Rectangle intersection;
    if (!gl::ClipRectangle(contextVk->getGLState().getScissor(), getRenderPassRenderArea(),
                           &intersection))
    {
        // There is nothing to clear since the scissor is outside of the render area.
        return angle::Result::Continue();
    }

    clearRect.rect = gl_vk::GetRect(intersection);

    if (contextVk->isViewportFlipEnabledForDrawFBO())
    {
        clearRect.rect.offset.y = getRenderPassRenderArea().height - clearRect.rect.offset.y -
                                  clearRect.rect.extent.height;
    }

    gl::AttachmentArray<VkClearAttachment> clearAttachments;
    int clearAttachmentIndex = 0;

    if (clearColor)
    {
        RenderTargetVk *renderTarget = getColorReadRenderTarget();
        const vk::Format &format     = renderTarget->getImageFormat();
        VkClearValue modifiedClear   = contextVk->getClearColorValue();

        // We need to make sure we are not clearing the alpha channel if we are using a buffer
        // format that doesn't have an alpha channel.
        if (format.angleFormat().alphaBits == 0)
        {
            modifiedClear.color.float32[3] = 1.0;
        }

        // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
        for (size_t colorIndex : mState.getEnabledDrawBuffers())
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
            clearAttachment.colorAttachment    = static_cast<uint32_t>(colorIndex);
            clearAttachment.clearValue         = modifiedClear;
            ++clearAttachmentIndex;
        }
    }

    if (clearDepth && clearStencil && mState.getDepthStencilAttachment() != nullptr)
    {
        // When we have a packed depth/stencil attachment we can do 1 clear for both when it
        // applies.
        VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
        clearAttachment.aspectMask      = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        clearAttachment.colorAttachment = VK_ATTACHMENT_UNUSED;
        clearAttachment.clearValue      = contextVk->getClearDepthStencilValue();
        ++clearAttachmentIndex;
    }
    else
    {
        if (clearDepth)
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_DEPTH_BIT;
            clearAttachment.colorAttachment    = VK_ATTACHMENT_UNUSED;
            clearAttachment.clearValue         = contextVk->getClearDepthStencilValue();
            ++clearAttachmentIndex;
        }

        if (clearStencil)
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_STENCIL_BIT;
            clearAttachment.colorAttachment    = VK_ATTACHMENT_UNUSED;
            clearAttachment.clearValue         = contextVk->getClearDepthStencilValue();
            ++clearAttachmentIndex;
        }
    }

    commandBuffer->clearAttachments(static_cast<uint32_t>(clearAttachmentIndex),
                                    clearAttachments.data(), 1, &clearRect);
    return angle::Result::Continue();
}

angle::Result FramebufferVk::clearWithDraw(ContextVk *contextVk,
                                           VkColorComponentFlags colorMaskFlags)
{
    RendererVk *renderer             = contextVk->getRenderer();
    vk::ShaderLibrary *shaderLibrary = renderer->getShaderLibrary();

    // Trigger a new command node to ensure overlapping writes happen sequentially.
    onResourceChanged(renderer);

    const vk::ShaderAndSerial *fullScreenQuad = nullptr;
    ANGLE_TRY(shaderLibrary->getShader(contextVk, vk::InternalShaderID::FullScreenQuad_vert,
                                       &fullScreenQuad));

    const vk::ShaderAndSerial *pushConstantColor = nullptr;
    ANGLE_TRY(shaderLibrary->getShader(contextVk, vk::InternalShaderID::PushConstantColor_frag,
                                       &pushConstantColor));

    // The shader uses a simple pipeline layout with a push constant range.
    vk::PipelineLayoutDesc pipelineLayoutDesc;
    pipelineLayoutDesc.updatePushConstantRange(gl::ShaderType::Fragment, 0,
                                               sizeof(VkClearColorValue));

    // The shader does not use any descriptor sets.
    vk::DescriptorSetLayoutPointerArray descriptorSetLayouts;

    vk::BindingPointer<vk::PipelineLayout> pipelineLayout;
    ANGLE_TRY(renderer->getPipelineLayout(contextVk, pipelineLayoutDesc, descriptorSetLayouts,
                                          &pipelineLayout));

    vk::RecordingMode recordingMode = vk::RecordingMode::Start;
    vk::CommandBuffer *drawCommands = nullptr;
    ANGLE_TRY(getCommandBufferForDraw(contextVk, &drawCommands, &recordingMode));

    const gl::Rectangle &renderArea = getRenderPassRenderArea();
    bool invertViewport             = contextVk->isViewportFlipEnabledForDrawFBO();

    // This pipeline desc could be cached.
    vk::PipelineDesc pipelineDesc;
    pipelineDesc.initDefaults();
    pipelineDesc.updateColorWriteMask(colorMaskFlags, getEmulatedAlphaAttachmentMask());
    pipelineDesc.updateRenderPassDesc(getRenderPassDesc());
    pipelineDesc.updateShaders(fullScreenQuad->getSerial(), pushConstantColor->getSerial());
    pipelineDesc.updateViewport(this, renderArea, 0.0f, 1.0f, invertViewport);

    const gl::State &glState = contextVk->getGLState();
    if (glState.isScissorTestEnabled())
    {
        gl::Rectangle intersection;
        if (!gl::ClipRectangle(glState.getScissor(), renderArea, &intersection))
        {
            return angle::Result::Continue();
        }

        pipelineDesc.updateScissor(intersection, invertViewport, renderArea);
    }
    else
    {
        pipelineDesc.updateScissor(renderArea, invertViewport, renderArea);
    }

    vk::PipelineAndSerial *pipeline = nullptr;
    ANGLE_TRY(renderer->getPipeline(contextVk, *fullScreenQuad, *pushConstantColor,
                                    pipelineLayout.get(), pipelineDesc, gl::AttributesMask(),
                                    &pipeline));
    pipeline->updateSerial(renderer->getCurrentQueueSerial());

    vk::CommandBuffer *writeCommands = nullptr;
    ANGLE_TRY(appendWriteResource(contextVk, &writeCommands));

    // If the format of the framebuffer does not have an alpha channel, we need to make sure we does
    // not affect the alpha channel of the type we're using to emulate the format.
    // TODO(jmadill): Implement EXT_draw_buffers http://anglebug.com/2394
    RenderTargetVk *renderTarget = mRenderTargetCache.getColors()[0];
    ASSERT(renderTarget);

    const vk::Format &imageFormat     = renderTarget->getImageFormat();
    VkClearColorValue clearColorValue = contextVk->getClearColorValue().color;
    bool overrideAlphaWithOne =
        imageFormat.textureFormat().alphaBits > 0 && imageFormat.angleFormat().alphaBits == 0;
    clearColorValue.float32[3] = overrideAlphaWithOne ? 1.0f : clearColorValue.float32[3];

    drawCommands->pushConstants(pipelineLayout.get(), VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                                sizeof(VkClearColorValue), clearColorValue.float32);

    // TODO(jmadill): Masked combined color and depth/stencil clear. http://anglebug.com/2455
    // Any active queries submitted by the user should also be paused here.
    drawCommands->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get());
    drawCommands->draw(6, 1, 0, 0);

    return angle::Result::Continue();
}

gl::Error FramebufferVk::getSamplePosition(const gl::Context *context,
                                           size_t index,
                                           GLfloat *xy) const
{
    UNIMPLEMENTED();
    return gl::InternalError() << "getSamplePosition is unimplemented.";
}

angle::Result FramebufferVk::getCommandBufferForDraw(ContextVk *contextVk,
                                                     vk::CommandBuffer **commandBufferOut,
                                                     vk::RecordingMode *modeOut)
{
    RendererVk *renderer = contextVk->getRenderer();

    // This will clear the current write operation if it is complete.
    if (appendToStartedRenderPass(renderer, commandBufferOut))
    {
        *modeOut = vk::RecordingMode::Append;
        return angle::Result::Continue();
    }

    vk::Framebuffer *framebuffer = nullptr;
    ANGLE_TRY(getFramebuffer(contextVk, &framebuffer));

    // TODO(jmadill): Proper clear value implementation. http://anglebug.com/2361
    std::vector<VkClearValue> attachmentClearValues;

    vk::CommandBuffer *writeCommands = nullptr;
    ANGLE_TRY(appendWriteResource(contextVk, &writeCommands));

    vk::RenderPassDesc renderPassDesc;

    // Initialize RenderPass info.
    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);

        colorRenderTarget->onColorDraw(this, writeCommands, &renderPassDesc);
        attachmentClearValues.emplace_back(contextVk->getClearColorValue());
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        depthStencilRenderTarget->onDepthStencilDraw(this, writeCommands, &renderPassDesc);
        attachmentClearValues.emplace_back(contextVk->getClearDepthStencilValue());
    }

    gl::Rectangle renderArea =
        gl::Rectangle(0, 0, mState.getDimensions().width, mState.getDimensions().height);

    *modeOut = vk::RecordingMode::Start;
    return beginRenderPass(contextVk, *framebuffer, renderArea, mRenderPassDesc.value(),
                           attachmentClearValues, commandBufferOut);
}

void FramebufferVk::updateActiveColorMasks(size_t colorIndex, bool r, bool g, bool b, bool a)
{
    mActiveColorComponentMasksForClear[0].set(colorIndex, r);
    mActiveColorComponentMasksForClear[1].set(colorIndex, g);
    mActiveColorComponentMasksForClear[2].set(colorIndex, b);
    mActiveColorComponentMasksForClear[3].set(colorIndex, a);
}

gl::DrawBufferMask FramebufferVk::getEmulatedAlphaAttachmentMask()
{
    return mEmulatedAlphaAttachmentMask;
}

angle::Result FramebufferVk::readPixelsImpl(ContextVk *contextVk,
                                            const gl::Rectangle &area,
                                            const PackPixelsParams &packPixelsParams,
                                            const VkImageAspectFlags &copyAspectFlags,
                                            RenderTargetVk *renderTarget,
                                            void *pixels)
{
    RendererVk *renderer = contextVk->getRenderer();

    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(beginWriteResource(contextVk, &commandBuffer));

    // Note that although we're reading from the image, we need to update the layout below.

    vk::ImageHelper *srcImage =
        renderTarget->getImageForRead(this, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);

    const angle::Format &angleFormat = srcImage->getFormat().textureFormat();

    GLuint pixelBytes = angleFormat.pixelBytes;

    // If we're copying only the stencil bits, we need to adjust the allocation size and the source
    // pitch because in Vulkan, if we copy the stencil out of a D24S8 texture, the stencil will be
    // tightly packed in an S8 buffer (as specified in the spec here
    // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkBufferImageCopy.html)
    if (copyAspectFlags == VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        pixelBytes = angleFormat.stencilBits / 8;
    }

    VkBuffer bufferHandle            = VK_NULL_HANDLE;
    uint8_t *readPixelBuffer         = nullptr;
    bool newBufferAllocated          = false;
    uint32_t stagingOffset           = 0;
    size_t allocationSize            = area.width * pixelBytes * area.height;

    ANGLE_TRY(mReadPixelsBuffer.allocate(contextVk, allocationSize, &readPixelBuffer, &bufferHandle,
                                         &stagingOffset, &newBufferAllocated));

    VkBufferImageCopy region;
    region.bufferImageHeight               = area.height;
    region.bufferOffset                    = static_cast<VkDeviceSize>(stagingOffset);
    region.bufferRowLength                 = area.width;
    region.imageExtent.width               = area.width;
    region.imageExtent.height              = area.height;
    region.imageExtent.depth               = 1;
    region.imageOffset.x                   = area.x;
    region.imageOffset.y                   = area.y;
    region.imageOffset.z                   = 0;
    region.imageSubresource.aspectMask     = copyAspectFlags;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount     = 1;
    region.imageSubresource.mipLevel       = 0;

    commandBuffer->copyImageToBuffer(srcImage->getImage(), srcImage->getCurrentLayout(),
                                     bufferHandle, 1, &region);

    // Triggers a full finish.
    // TODO(jmadill): Don't block on asynchronous readback.
    ANGLE_TRY(renderer->finish(contextVk));

    // The buffer we copied to needs to be invalidated before we read from it because its not been
    // created with the host coherent bit.
    ANGLE_TRY(mReadPixelsBuffer.invalidate(contextVk));

    PackPixels(packPixelsParams, angleFormat, area.width * pixelBytes, readPixelBuffer,
               static_cast<uint8_t *>(pixels));

    return angle::Result::Continue();
}

const gl::Extents &FramebufferVk::getReadImageExtents() const
{
    return getColorReadRenderTarget()->getImageExtents();
}
}  // namespace rx
