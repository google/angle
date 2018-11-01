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

bool ClipToRenderTarget(const gl::Rectangle &area,
                        RenderTargetVk *renderTarget,
                        gl::Rectangle *rectOut)
{
    const gl::Extents &renderTargetExtents = renderTarget->getImageExtents();
    gl::Rectangle renderTargetRect(0, 0, renderTargetExtents.width, renderTargetExtents.height);
    return ClipRectangle(area, renderTargetRect, rectOut);
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

// Special rules apply to VkBufferImageCopy with depth/stencil. The components are tightly packed
// into a depth or stencil section of the destination buffer. See the spec:
// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkBufferImageCopy.html
const angle::Format &GetDepthStencilImageToBufferFormat(const angle::Format &imageFormat,
                                                        VkImageAspectFlagBits copyAspect)
{
    if (copyAspect == VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        ASSERT(imageFormat.id == angle::FormatID::D24_UNORM_S8_UINT ||
               imageFormat.id == angle::FormatID::D32_FLOAT_S8X24_UINT ||
               imageFormat.id == angle::FormatID::S8_UINT);
        return angle::Format::Get(angle::FormatID::S8_UINT);
    }

    ASSERT(copyAspect == VK_IMAGE_ASPECT_DEPTH_BIT);

    switch (imageFormat.id)
    {
        case angle::FormatID::D16_UNORM:
            return imageFormat;
        case angle::FormatID::D24_UNORM_X8_UINT:
            return imageFormat;
        case angle::FormatID::D24_UNORM_S8_UINT:
            return angle::Format::Get(angle::FormatID::D24_UNORM_X8_UINT);
        case angle::FormatID::D32_FLOAT:
            return imageFormat;
        case angle::FormatID::D32_FLOAT_S8X24_UINT:
            return angle::Format::Get(angle::FormatID::D32_FLOAT);
        default:
            UNREACHABLE();
            return imageFormat;
    }
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
      mReadPixelBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, kMinReadPixelsBufferSize),
      mBlitPixelBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, kMinReadPixelsBufferSize)
{
    mBlitPixelBuffer.init(1, renderer);
    mReadPixelBuffer.init(4, renderer);
}

FramebufferVk::~FramebufferVk() = default;

void FramebufferVk::destroy(const gl::Context *context)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();
    mFramebuffer.release(renderer);

    mReadPixelBuffer.destroy(contextVk->getDevice());
    mBlitPixelBuffer.destroy(contextVk->getDevice());
}

angle::Result FramebufferVk::discard(const gl::Context *context,
                                     size_t count,
                                     const GLenum *attachments)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result FramebufferVk::invalidate(const gl::Context *context,
                                        size_t count,
                                        const GLenum *attachments)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result FramebufferVk::invalidateSub(const gl::Context *context,
                                           size_t count,
                                           const GLenum *attachments,
                                           const gl::Rectangle &area)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result FramebufferVk::clear(const gl::Context *context, GLbitfield mask)
{
    ContextVk *contextVk = vk::GetImpl(context);

    // This command buffer is only started once.
    vk::CommandBuffer *commandBuffer = nullptr;

    const gl::FramebufferAttachment *depthAttachment = mState.getDepthAttachment();
    bool clearDepth = (depthAttachment && (mask & GL_DEPTH_BUFFER_BIT) != 0);
    ASSERT(!clearDepth || depthAttachment->isAttached());

    // If depth write is disabled, pretend that GL_DEPTH_BUFFER_BIT is not specified altogether.
    clearDepth = clearDepth && contextVk->getGLState().getDepthStencilState().depthMask;

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
            ANGLE_TRY(clearWithClearAttachments(contextVk, false, clearDepth, clearStencil));
        }
        return angle::Result::Continue();
    }

    // If we clear the depth OR the stencil but not both, and we have a packed depth stencil
    // attachment, we need to use clearAttachments instead of clearDepthStencil since Vulkan won't
    // allow us to clear one or the other separately.
    // Note: this might be bugged if we emulate single depth or stencil with a packed format.
    // TODO(jmadill): Investigate emulated packed formats. http://anglebug.com/2815
    bool isSingleClearOnPackedDepthStencilAttachment =
        depthStencilAttachment && (clearDepth != clearStencil);
    if (glState.isScissorTestEnabled() || isSingleClearOnPackedDepthStencilAttachment)
    {
        // With scissor test enabled, we clear very differently and we don't need to access
        // the image inside each attachment we can just use clearCmdAttachments with our
        // scissor region instead.
        ANGLE_TRY(clearWithClearAttachments(contextVk, clearColor, clearDepth, clearStencil));
        return angle::Result::Continue();
    }

    // Standard Depth/stencil clear without scissor.
    if (clearDepth || clearStencil)
    {
        ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &commandBuffer));

        VkClearDepthStencilValue clearDepthStencilValue =
            contextVk->getClearDepthStencilValue().depthStencil;

        // Apply the stencil mask to the clear value.
        clearDepthStencilValue.stencil &=
            contextVk->getGLState().getDepthStencilState().stencilWritemask;

        RenderTargetVk *renderTarget         = mRenderTargetCache.getDepthStencil();
        const angle::Format &format          = renderTarget->getImageFormat().textureFormat();
        const VkImageAspectFlags aspectFlags = vk::GetDepthStencilAspectFlags(format);

        vk::ImageHelper *image = renderTarget->getImageForWrite(&mFramebuffer);
        image->clearDepthStencil(aspectFlags, clearDepthStencilValue, commandBuffer);
    }

    if (!clearColor)
    {
        return angle::Result::Continue();
    }

    const auto *attachment = mState.getFirstNonNullAttachment();
    ASSERT(attachment && attachment->isAttached());

    if (!commandBuffer)
    {
        ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &commandBuffer));
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
        vk::ImageHelper *image = colorRenderTarget->getImageForWrite(&mFramebuffer);
        GLint mipLevelToClear  = (attachment->type() == GL_TEXTURE) ? attachment->mipLevel() : 0;

        // If we're clearing a cube map face ensure we only clear the selected layer.
        image->clearColorLayer(modifiedClearColorValue, mipLevelToClear, 1,
                               colorRenderTarget->getLayerIndex(), 1, commandBuffer);
    }

    return angle::Result::Continue();
}

angle::Result FramebufferVk::clearBufferfv(const gl::Context *context,
                                           GLenum buffer,
                                           GLint drawbuffer,
                                           const GLfloat *values)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result FramebufferVk::clearBufferuiv(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            const GLuint *values)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result FramebufferVk::clearBufferiv(const gl::Context *context,
                                           GLenum buffer,
                                           GLint drawbuffer,
                                           const GLint *values)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result FramebufferVk::clearBufferfi(const gl::Context *context,
                                           GLenum buffer,
                                           GLint drawbuffer,
                                           GLfloat depth,
                                           GLint stencil)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

GLenum FramebufferVk::getImplementationColorReadFormat(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, mRenderTargetCache.getColorRead(mState)).format;
}

GLenum FramebufferVk::getImplementationColorReadType(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, mRenderTargetCache.getColorRead(mState)).type;
}

angle::Result FramebufferVk::readPixels(const gl::Context *context,
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
        return angle::Result::Continue();
    }
    gl::Rectangle flippedArea = clippedArea;
    if (contextVk->isViewportFlipEnabledForReadFBO())
    {
        flippedArea.y = fbRect.height - flippedArea.y - flippedArea.height;
    }

    const gl::State &glState = context->getGLState();
    const gl::PixelPackState &packState = glState.getPackState();

    const gl::InternalFormat &sizedFormatInfo = gl::GetInternalFormatInfo(format, type);

    GLuint outputPitch = 0;
    ANGLE_VK_CHECK_MATH(contextVk,
                        sizedFormatInfo.computeRowPitch(type, area.width, packState.alignment,
                                                        packState.rowLength, &outputPitch));
    GLuint outputSkipBytes = 0;
    ANGLE_VK_CHECK_MATH(contextVk, sizedFormatInfo.computeSkipBytes(type, outputPitch, 0, packState,
                                                                    false, &outputSkipBytes));

    outputSkipBytes += (clippedArea.x - area.x) * sizedFormatInfo.pixelBytes +
                       (clippedArea.y - area.y) * outputPitch;

    const angle::Format &angleFormat = GetFormatFromFormatType(format, type);

    PackPixelsParams params(flippedArea, angleFormat, outputPitch, packState.reverseRowOrder,
                            glState.getTargetBuffer(gl::BufferBinding::PixelPack), 0);
    if (contextVk->isViewportFlipEnabledForReadFBO())
    {
        params.reverseRowOrder = !params.reverseRowOrder;
    }

    ANGLE_TRY(readPixelsImpl(contextVk, flippedArea, params, VK_IMAGE_ASPECT_COLOR_BIT,
                             getColorReadRenderTarget(),
                             static_cast<uint8_t *>(pixels) + outputSkipBytes));
    mReadPixelBuffer.releaseRetainedBuffers(renderer);
    return angle::Result::Continue();
}

RenderTargetVk *FramebufferVk::getDepthStencilRenderTarget() const
{
    return mRenderTargetCache.getDepthStencil();
}

angle::Result FramebufferVk::blitWithCopy(ContextVk *contextVk,
                                          const gl::Rectangle &copyArea,
                                          RenderTargetVk *readRenderTarget,
                                          RenderTargetVk *drawRenderTarget,
                                          bool blitDepthBuffer,
                                          bool blitStencilBuffer)
{
    vk::ImageHelper *writeImage = drawRenderTarget->getImageForWrite(&mFramebuffer);

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &commandBuffer));

    vk::ImageHelper *readImage = readRenderTarget->getImageForRead(
        &mFramebuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);

    VkImageAspectFlags aspectMask =
        vk::GetDepthStencilAspectFlagsForCopy(blitDepthBuffer, blitStencilBuffer);
    vk::ImageHelper::Copy(readImage, writeImage, gl::Offset(), gl::Offset(),
                          gl::Extents(copyArea.width, copyArea.height, 1), aspectMask,
                          commandBuffer);
    return angle::Result::Continue();
}

RenderTargetVk *FramebufferVk::getColorReadRenderTarget() const
{
    RenderTargetVk *renderTarget = mRenderTargetCache.getColorRead(mState);
    ASSERT(renderTarget && renderTarget->getImage().valid());
    return renderTarget;
}

angle::Result FramebufferVk::blitWithReadback(ContextVk *contextVk,
                                              const gl::Rectangle &copyArea,
                                              VkImageAspectFlagBits aspect,
                                              RenderTargetVk *readRenderTarget,
                                              RenderTargetVk *drawRenderTarget)
{
    RendererVk *renderer            = contextVk->getRenderer();
    const angle::Format &readFormat = readRenderTarget->getImageFormat().textureFormat();

    ASSERT(aspect == VK_IMAGE_ASPECT_DEPTH_BIT || aspect == VK_IMAGE_ASPECT_STENCIL_BIT);

    // This path is only currently used for y-flipping depth/stencil blits.
    PackPixelsParams packPixelsParams;
    packPixelsParams.reverseRowOrder      = true;
    packPixelsParams.area.width           = copyArea.width;
    packPixelsParams.area.height          = copyArea.height;
    packPixelsParams.area.x               = copyArea.x;
    packPixelsParams.area.y               = copyArea.y;

    // Read back depth values into the destination buffer.
    const angle::Format &copyFormat = GetDepthStencilImageToBufferFormat(readFormat, aspect);
    packPixelsParams.destFormat     = &copyFormat;
    packPixelsParams.outputPitch    = copyFormat.pixelBytes * copyArea.width;

    // Allocate a space in the destination buffer to write to.
    size_t blitAllocationSize = copyFormat.pixelBytes * copyArea.width * copyArea.height;
    uint8_t *destPtr          = nullptr;
    VkBuffer destBufferHandle = VK_NULL_HANDLE;
    VkDeviceSize destOffset   = 0;
    ANGLE_TRY(mBlitPixelBuffer.allocate(contextVk, blitAllocationSize, &destPtr, &destBufferHandle,
                                        &destOffset, nullptr));

    ANGLE_TRY(
        readPixelsImpl(contextVk, copyArea, packPixelsParams, aspect, readRenderTarget, destPtr));

    VkBufferImageCopy copyRegion               = {};
    copyRegion.bufferOffset                    = destOffset;
    copyRegion.bufferImageHeight               = copyArea.height;
    copyRegion.bufferRowLength                 = copyArea.width;
    copyRegion.imageExtent.width               = copyArea.width;
    copyRegion.imageExtent.height              = copyArea.height;
    copyRegion.imageExtent.depth               = 1;
    copyRegion.imageSubresource.mipLevel       = 0;
    copyRegion.imageSubresource.aspectMask     = aspect;
    copyRegion.imageSubresource.baseArrayLayer = 0;
    copyRegion.imageSubresource.layerCount     = 1;
    copyRegion.imageOffset.x                   = copyArea.x;
    copyRegion.imageOffset.y                   = copyArea.y;
    copyRegion.imageOffset.z                   = 0;

    ANGLE_TRY(mBlitPixelBuffer.flush(contextVk));

    // Reinitialize the commandBuffer after a read pixels because it calls
    // renderer->finish which makes command buffers obsolete.
    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &commandBuffer));

    // We read the bytes of the image in a buffer, now we have to copy them into the
    // destination target.
    vk::ImageHelper *imageForWrite = drawRenderTarget->getImageForWrite(&mFramebuffer);

    imageForWrite->changeLayoutWithStages(
        imageForWrite->getAspectFlags(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    commandBuffer->copyBufferToImage(destBufferHandle, imageForWrite->getImage(),
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    mBlitPixelBuffer.releaseRetainedBuffers(renderer);

    return angle::Result::Continue();
}

angle::Result FramebufferVk::blit(const gl::Context *context,
                                  const gl::Rectangle &sourceArea,
                                  const gl::Rectangle &destArea,
                                  GLbitfield mask,
                                  GLenum filter)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    const gl::State &glState                 = context->getGLState();
    const gl::Framebuffer *sourceFramebuffer = glState.getReadFramebuffer();
    bool blitColorBuffer                     = (mask & GL_COLOR_BUFFER_BIT) != 0;
    bool blitDepthBuffer                     = (mask & GL_DEPTH_BUFFER_BIT) != 0;
    bool blitStencilBuffer                   = (mask & GL_STENCIL_BUFFER_BIT) != 0;

    FramebufferVk *sourceFramebufferVk = vk::GetImpl(sourceFramebuffer);
    bool flipSource                    = contextVk->isViewportFlipEnabledForReadFBO();
    bool flipDest                      = contextVk->isViewportFlipEnabledForDrawFBO();

    gl::Rectangle readRect = sourceArea;
    gl::Rectangle drawRect = destArea;

    if (glState.isScissorTestEnabled())
    {
        const gl::Rectangle scissorRect = glState.getScissor();
        if (!ClipRectangle(sourceArea, scissorRect, &readRect))
        {
            return angle::Result::Continue();
        }

        if (!ClipRectangle(destArea, scissorRect, &drawRect))
        {
            return angle::Result::Continue();
        }
    }

    // After cropping for the scissor, we also want to crop for the size of the buffers.

    if (blitColorBuffer)
    {
        RenderTargetVk *readRenderTarget = sourceFramebufferVk->getColorReadRenderTarget();

        gl::Rectangle readRenderTargetRect;
        if (!ClipToRenderTarget(readRect, readRenderTarget, &readRenderTargetRect))
        {
            return angle::Result::Continue();
        }

        for (size_t colorAttachment : mState.getEnabledDrawBuffers())
        {
            RenderTargetVk *drawRenderTarget = mRenderTargetCache.getColors()[colorAttachment];
            ASSERT(drawRenderTarget);
            ASSERT(HasSrcAndDstBlitProperties(renderer->getPhysicalDevice(), readRenderTarget,
                                              drawRenderTarget));

            gl::Rectangle drawRenderTargetRect;
            if (!ClipToRenderTarget(drawRect, drawRenderTarget, &drawRenderTargetRect))
            {
                return angle::Result::Continue();
            }

            ANGLE_TRY(blitWithCommand(contextVk, readRenderTargetRect, drawRenderTargetRect,
                                      readRenderTarget, drawRenderTarget, filter, true, false,
                                      false, flipSource, flipDest));
        }
    }

    if (blitDepthBuffer || blitStencilBuffer)
    {
        RenderTargetVk *readRenderTarget = sourceFramebufferVk->getDepthStencilRenderTarget();
        ASSERT(readRenderTarget);

        gl::Rectangle readRenderTargetRect;
        if (!ClipToRenderTarget(readRect, readRenderTarget, &readRenderTargetRect))
        {
            return angle::Result::Continue();
        }

        RenderTargetVk *drawRenderTarget = mRenderTargetCache.getDepthStencil();
        ASSERT(drawRenderTarget);

        gl::Rectangle drawRenderTargetRect;
        if (!ClipToRenderTarget(drawRect, drawRenderTarget, &drawRenderTargetRect))
        {
            return angle::Result::Continue();
        }

        ASSERT(readRenderTargetRect == drawRenderTargetRect);
        ASSERT(filter == GL_NEAREST);

        if (HasSrcAndDstBlitProperties(renderer->getPhysicalDevice(), readRenderTarget,
                                       drawRenderTarget))
        {
            ANGLE_TRY(blitWithCommand(contextVk, readRenderTargetRect, drawRenderTargetRect,
                                      readRenderTarget, drawRenderTarget, filter, false,
                                      blitDepthBuffer, blitStencilBuffer, flipSource, flipDest));
        }
        else
        {
            if (flipSource || flipDest)
            {
                if (blitDepthBuffer)
                {
                    ANGLE_TRY(blitWithReadback(contextVk, readRenderTargetRect,
                                               VK_IMAGE_ASPECT_DEPTH_BIT, readRenderTarget,
                                               drawRenderTarget));
                }
                if (blitStencilBuffer)
                {
                    ANGLE_TRY(blitWithReadback(contextVk, readRenderTargetRect,
                                               VK_IMAGE_ASPECT_STENCIL_BIT, readRenderTarget,
                                               drawRenderTarget));
                }
            }
            else
            {
                ANGLE_TRY(blitWithCopy(contextVk, readRenderTargetRect, readRenderTarget,
                                       drawRenderTarget, blitDepthBuffer, blitStencilBuffer));
            }
        }
    }

    return angle::Result::Continue();
}

angle::Result FramebufferVk::blitWithCommand(ContextVk *contextVk,
                                             const gl::Rectangle &readRectIn,
                                             const gl::Rectangle &drawRectIn,
                                             RenderTargetVk *readRenderTarget,
                                             RenderTargetVk *drawRenderTarget,
                                             GLenum filter,
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

    vk::ImageHelper *dstImage = drawRenderTarget->getImageForWrite(&mFramebuffer);

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &commandBuffer));

    const vk::Format &readImageFormat = readRenderTarget->getImageFormat();
    VkImageAspectFlags aspectMask =
        colorBlit ? VK_IMAGE_ASPECT_COLOR_BIT
                  : vk::GetDepthStencilAspectFlags(readImageFormat.textureFormat());
    vk::ImageHelper *srcImage = readRenderTarget->getImageForRead(
        &mFramebuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);

    const gl::Extents sourceFrameBufferExtents = readRenderTarget->getImageExtents();
    gl::Rectangle readRect                     = readRectIn;

    if (flipSource)
    {
        readRect.y = sourceFrameBufferExtents.height - readRect.y - readRect.height;
    }

    VkImageBlit blit               = {};
    blit.srcOffsets[0]             = {readRect.x0(), flipSource ? readRect.y1() : readRect.y0(), 0};
    blit.srcOffsets[1]             = {readRect.x1(), flipSource ? readRect.y0() : readRect.y1(), 1};
    blit.srcSubresource.aspectMask = aspectMask;
    blit.srcSubresource.mipLevel   = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.dstSubresource.aspectMask     = aspectMask;
    blit.dstSubresource.mipLevel       = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    const gl::Extents &drawFrameBufferExtents = drawRenderTarget->getImageExtents();
    gl::Rectangle drawRect                    = drawRectIn;

    if (flipDest)
    {
        drawRect.y = drawFrameBufferExtents.height - drawRect.y - drawRect.height;
    }

    blit.dstOffsets[0] = {drawRect.x0(), flipDest ? drawRect.y1() : drawRect.y0(), 0};
    blit.dstOffsets[1] = {drawRect.x1(), flipDest ? drawRect.y0() : drawRect.y1(), 1};

    // Requirement of the copyImageToBuffer, the dst image must be in
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
    dstImage->changeLayoutWithStages(aspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                     VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    commandBuffer->blitImage(srcImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             dstImage->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                             gl_vk::GetFilter(filter));

    return angle::Result::Continue();
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

angle::Result FramebufferVk::syncState(const gl::Context *context,
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
    mFramebuffer.release(renderer);

    // Will freeze the current set of dependencies on this FBO. The next time we render we will
    // create a new entry in the command graph.
    mFramebuffer.finishCurrentCommands(renderer);

    contextVk->invalidateCurrentPipeline();

    return angle::Result::Continue();
}

const vk::RenderPassDesc &FramebufferVk::getRenderPassDesc()
{
    if (mRenderPassDesc.valid())
    {
        return mRenderPassDesc.value();
    }

    vk::RenderPassDesc desc;
    desc.setSamples(getSamples());

    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);
        desc.packAttachment(colorRenderTarget->getImage().getFormat());
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        desc.packAttachment(depthStencilRenderTarget->getImage().getFormat());
    }

    mRenderPassDesc = desc;
    return mRenderPassDesc.value();
}

angle::Result FramebufferVk::getFramebuffer(ContextVk *contextVk, vk::Framebuffer **framebufferOut)
{
    // If we've already created our cached Framebuffer, return it.
    if (mFramebuffer.valid())
    {
        *framebufferOut = &mFramebuffer.getFramebuffer();
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

    VkFramebufferCreateInfo framebufferInfo = {};

    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags           = 0;
    framebufferInfo.renderPass      = renderPass->getHandle();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = static_cast<uint32_t>(attachmentsSize.width);
    framebufferInfo.height          = static_cast<uint32_t>(attachmentsSize.height);
    framebufferInfo.layers          = 1;

    ANGLE_TRY(mFramebuffer.init(contextVk, framebufferInfo));

    *framebufferOut = &mFramebuffer.getFramebuffer();
    return angle::Result::Continue();
}

angle::Result FramebufferVk::clearWithClearAttachments(ContextVk *contextVk,
                                                       bool clearColor,
                                                       bool clearDepth,
                                                       bool clearStencil)
{
    // Trigger a new command node to ensure overlapping writes happen sequentially.
    mFramebuffer.finishCurrentCommands(contextVk->getRenderer());

    // This command can only happen inside a render pass, so obtain one if its already happening
    // or create a new one if not.
    vk::CommandBuffer *commandBuffer = nullptr;
    vk::RecordingMode mode           = vk::RecordingMode::Start;
    ANGLE_TRY(getCommandBufferForDraw(contextVk, &commandBuffer, &mode));

    // The array layer is offset by the ImageView. So we shouldn't need to set a base array layer.
    VkClearRect clearRect    = {};
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount     = 1;

    // When clearing, the scissor region must be clipped to the renderArea per the validation rules
    // in Vulkan.
    gl::Rectangle intersection;
    if (!gl::ClipRectangle(contextVk->getGLState().getScissor(),
                           mFramebuffer.getRenderPassRenderArea(), &intersection))
    {
        // There is nothing to clear since the scissor is outside of the render area.
        return angle::Result::Continue();
    }

    clearRect.rect = gl_vk::GetRect(intersection);

    if (contextVk->isViewportFlipEnabledForDrawFBO())
    {
        clearRect.rect.offset.y = mFramebuffer.getRenderPassRenderArea().height -
                                  clearRect.rect.offset.y - clearRect.rect.extent.height;
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

    VkClearValue depthStencilClearValue = contextVk->getClearDepthStencilValue();

    // Apply the stencil mask to the clear value.  Stencil mask is generally respected through the
    // respective pipeline state, but clear uses its own special function.
    if (clearStencil)
    {
        depthStencilClearValue.depthStencil.stencil &=
            contextVk->getGLState().getDepthStencilState().stencilWritemask;
    }

    if (clearDepth && clearStencil && mState.getDepthStencilAttachment() != nullptr)
    {
        // When we have a packed depth/stencil attachment we can do 1 clear for both when it
        // applies.
        VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
        clearAttachment.aspectMask      = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        clearAttachment.colorAttachment = VK_ATTACHMENT_UNUSED;
        clearAttachment.clearValue         = depthStencilClearValue;
        ++clearAttachmentIndex;
    }
    else
    {
        if (clearDepth)
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_DEPTH_BIT;
            clearAttachment.colorAttachment    = VK_ATTACHMENT_UNUSED;
            clearAttachment.clearValue         = depthStencilClearValue;
            ++clearAttachmentIndex;
        }

        if (clearStencil)
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_STENCIL_BIT;
            clearAttachment.colorAttachment    = VK_ATTACHMENT_UNUSED;
            clearAttachment.clearValue         = depthStencilClearValue;
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
    mFramebuffer.finishCurrentCommands(renderer);

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

    const gl::Rectangle &renderArea = mFramebuffer.getRenderPassRenderArea();
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
    ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &writeCommands));

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

angle::Result FramebufferVk::getSamplePosition(const gl::Context *context,
                                               size_t index,
                                               GLfloat *xy) const
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
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

    return startNewRenderPass(contextVk, commandBufferOut);
}

angle::Result FramebufferVk::startNewRenderPass(ContextVk *contextVk,
                                                vk::CommandBuffer **commandBufferOut)
{
    vk::Framebuffer *framebuffer = nullptr;
    ANGLE_TRY(getFramebuffer(contextVk, &framebuffer));

    // TODO(jmadill): Proper clear value implementation. http://anglebug.com/2361
    std::vector<VkClearValue> attachmentClearValues;

    vk::CommandBuffer *writeCommands = nullptr;
    ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &writeCommands));

    vk::RenderPassDesc renderPassDesc;

    // Initialize RenderPass info.
    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);

        colorRenderTarget->onColorDraw(&mFramebuffer, writeCommands, &renderPassDesc);
        attachmentClearValues.emplace_back(contextVk->getClearColorValue());
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        depthStencilRenderTarget->onDepthStencilDraw(&mFramebuffer, writeCommands, &renderPassDesc);
        attachmentClearValues.emplace_back(contextVk->getClearDepthStencilValue());
    }

    gl::Rectangle renderArea =
        gl::Rectangle(0, 0, mState.getDimensions().width, mState.getDimensions().height);

    return mFramebuffer.beginRenderPass(contextVk, *framebuffer, renderArea,
                                        mRenderPassDesc.value(), attachmentClearValues,
                                        commandBufferOut);
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
                                            VkImageAspectFlagBits copyAspectFlags,
                                            RenderTargetVk *renderTarget,
                                            void *pixels)
{
    RendererVk *renderer = contextVk->getRenderer();

    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(mFramebuffer.recordCommands(contextVk, &commandBuffer));

    // Note that although we're reading from the image, we need to update the layout below.

    vk::ImageHelper *srcImage = renderTarget->getImageForRead(
        &mFramebuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, commandBuffer);

    const angle::Format *readFormat = &srcImage->getFormat().textureFormat();

    if (copyAspectFlags != VK_IMAGE_ASPECT_COLOR_BIT)
    {
        readFormat = &GetDepthStencilImageToBufferFormat(*readFormat, copyAspectFlags);
    }

    VkBuffer bufferHandle    = VK_NULL_HANDLE;
    uint8_t *readPixelBuffer = nullptr;
    bool newBufferAllocated  = false;
    VkDeviceSize stagingOffset = 0;
    size_t allocationSize    = readFormat->pixelBytes * area.width * area.height;

    ANGLE_TRY(mReadPixelBuffer.allocate(contextVk, allocationSize, &readPixelBuffer, &bufferHandle,
                                        &stagingOffset, &newBufferAllocated));

    VkBufferImageCopy region               = {};
    region.bufferImageHeight               = area.height;
    region.bufferOffset                    = stagingOffset;
    region.bufferRowLength                 = area.width;
    region.imageExtent.width               = area.width;
    region.imageExtent.height              = area.height;
    region.imageExtent.depth               = 1;
    region.imageOffset.x                   = area.x;
    region.imageOffset.y                   = area.y;
    region.imageOffset.z                   = 0;
    region.imageSubresource.aspectMask     = copyAspectFlags;
    region.imageSubresource.baseArrayLayer = renderTarget->getLayerIndex();
    region.imageSubresource.layerCount     = 1;
    region.imageSubresource.mipLevel       = 0;

    commandBuffer->copyImageToBuffer(srcImage->getImage(), srcImage->getCurrentLayout(),
                                     bufferHandle, 1, &region);

    // Triggers a full finish.
    // TODO(jmadill): Don't block on asynchronous readback.
    ANGLE_TRY(renderer->finish(contextVk));

    // The buffer we copied to needs to be invalidated before we read from it because its not been
    // created with the host coherent bit.
    ANGLE_TRY(mReadPixelBuffer.invalidate(contextVk));

    PackPixels(packPixelsParams, *readFormat, area.width * readFormat->pixelBytes, readPixelBuffer,
               static_cast<uint8_t *>(pixels));

    return angle::Result::Continue();
}

const gl::Extents &FramebufferVk::getReadImageExtents() const
{
    return getColorReadRenderTarget()->getImageExtents();
}

RenderTargetVk *FramebufferVk::getFirstRenderTarget() const
{
    for (auto *renderTarget : mRenderTargetCache.getColors())
    {
        if (renderTarget)
        {
            return renderTarget;
        }
    }

    return mRenderTargetCache.getDepthStencil();
}

GLint FramebufferVk::getSamples() const
{
    RenderTargetVk *firstRT = getFirstRenderTarget();
    return firstRT ? firstRT->getImage().getSamples() : 0;
}

}  // namespace rx
