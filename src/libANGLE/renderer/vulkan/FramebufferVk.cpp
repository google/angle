//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferVk.cpp:
//    Implements the class methods for FramebufferVk.
//

#include "libANGLE/renderer/vulkan/FramebufferVk.h"

#include <array>
#include <vulkan/vulkan.h>

#include "common/debug.h"
#include "image_util/imageformats.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/SurfaceVk.h"
#include "libANGLE/renderer/vulkan/formatutilsvk.h"

namespace rx
{

namespace
{

gl::ErrorOrResult<const gl::InternalFormat *> GetReadAttachmentInfo(
    const gl::FramebufferAttachment *readAttachment)
{
    RenderTargetVk *renderTarget = nullptr;
    ANGLE_TRY(readAttachment->getRenderTarget(&renderTarget));

    GLenum implFormat = renderTarget->format->format().fboImplementationInternalFormat;
    return &gl::GetInternalFormatInfo(implFormat);
}

}  // anonymous namespace

// static
FramebufferVk *FramebufferVk::CreateUserFBO(const gl::FramebufferState &state)
{
    return new FramebufferVk(state);
}

// static
FramebufferVk *FramebufferVk::CreateDefaultFBO(const gl::FramebufferState &state,
                                               WindowSurfaceVk *backbuffer)
{
    return new FramebufferVk(state, backbuffer);
}

FramebufferVk::FramebufferVk(const gl::FramebufferState &state) : FramebufferImpl(state)
{
}

FramebufferVk::FramebufferVk(const gl::FramebufferState &state, WindowSurfaceVk *backbuffer)
    : FramebufferImpl(state)
{
}

FramebufferVk::~FramebufferVk()
{
}

gl::Error FramebufferVk::discard(size_t count, const GLenum *attachments)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferVk::invalidate(size_t count, const GLenum *attachments)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferVk::invalidateSub(size_t count,
                                       const GLenum *attachments,
                                       const gl::Rectangle &area)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferVk::clear(ContextImpl *context, GLbitfield mask)
{
    ContextVk *contextVk = GetAs<ContextVk>(context);

    if (mState.getDepthAttachment() && (mask & GL_DEPTH_BUFFER_BIT) != 0)
    {
        // TODO(jmadill): Depth clear
        UNIMPLEMENTED();
    }

    if (mState.getStencilAttachment() && (mask & GL_STENCIL_BUFFER_BIT) != 0)
    {
        // TODO(jmadill): Stencil clear
        UNIMPLEMENTED();
    }

    if ((mask & GL_COLOR_BUFFER_BIT) == 0)
    {
        return gl::NoError();
    }

    const auto &glState    = context->getGLState();
    const auto &clearColor = glState.getColorClearValue();
    VkClearColorValue clearColorValue;
    clearColorValue.float32[0] = clearColor.red;
    clearColorValue.float32[1] = clearColor.green;
    clearColorValue.float32[2] = clearColor.blue;
    clearColorValue.float32[3] = clearColor.alpha;

    // TODO(jmadill): Scissored clears.
    const auto *attachment = mState.getFirstNonNullAttachment();
    ASSERT(attachment && attachment->isAttached());
    const auto &size = attachment->getSize();
    const gl::Rectangle renderArea(0, 0, size.width, size.height);

    vk::CommandBuffer *commandBuffer = contextVk->getCommandBuffer();
    ANGLE_TRY(commandBuffer->begin());

    for (const auto &colorAttachment : mState.getColorAttachments())
    {
        if (colorAttachment.isAttached())
        {
            RenderTargetVk *renderTarget = nullptr;
            ANGLE_TRY(colorAttachment.getRenderTarget(&renderTarget));
            renderTarget->image->changeLayoutTop(
                VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer);
            commandBuffer->clearSingleColorImage(*renderTarget->image, clearColorValue);
        }
    }

    commandBuffer->end();

    ANGLE_TRY(contextVk->submitCommands(*commandBuffer));

    return gl::NoError();
}

gl::Error FramebufferVk::clearBufferfv(ContextImpl *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       const GLfloat *values)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferVk::clearBufferuiv(ContextImpl *context,
                                        GLenum buffer,
                                        GLint drawbuffer,
                                        const GLuint *values)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferVk::clearBufferiv(ContextImpl *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       const GLint *values)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferVk::clearBufferfi(ContextImpl *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       GLfloat depth,
                                       GLint stencil)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

GLenum FramebufferVk::getImplementationColorReadFormat() const
{
    auto errOrResult = GetReadAttachmentInfo(mState.getReadAttachment());

    // TODO(jmadill): Handle getRenderTarget error.
    if (errOrResult.isError())
    {
        ERR("Internal error in FramebufferVk::getImplementationColorReadFormat.");
        return GL_NONE;
    }

    return errOrResult.getResult()->format;
}

GLenum FramebufferVk::getImplementationColorReadType() const
{
    auto errOrResult = GetReadAttachmentInfo(mState.getReadAttachment());

    // TODO(jmadill): Handle getRenderTarget error.
    if (errOrResult.isError())
    {
        ERR("Internal error in FramebufferVk::getImplementationColorReadFormat.");
        return GL_NONE;
    }

    return errOrResult.getResult()->type;
}

gl::Error FramebufferVk::readPixels(ContextImpl *context,
                                    const gl::Rectangle &area,
                                    GLenum format,
                                    GLenum type,
                                    GLvoid *pixels) const
{
    const auto &glState         = context->getGLState();
    const auto *readFramebuffer = glState.getReadFramebuffer();
    const auto *readAttachment  = readFramebuffer->getReadColorbuffer();

    RenderTargetVk *renderTarget = nullptr;
    ANGLE_TRY(readAttachment->getRenderTarget(&renderTarget));

    ContextVk *contextVk = GetAs<ContextVk>(context);
    RendererVk *renderer = contextVk->getRenderer();

    vk::Image *readImage = renderTarget->image;
    vk::StagingImage stagingImage;
    ANGLE_TRY_RESULT(renderer->createStagingImage(TextureDimension::TEX_2D, *renderTarget->format,
                                                  renderTarget->extents),
                     stagingImage);

    vk::CommandBuffer *commandBuffer = contextVk->getCommandBuffer();
    commandBuffer->begin();
    stagingImage.getImage().changeLayoutTop(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
                                            commandBuffer);

    gl::Box copyRegion;
    copyRegion.x      = area.x;
    copyRegion.y      = area.y;
    copyRegion.z      = 0;
    copyRegion.width  = area.width;
    copyRegion.height = area.height;
    copyRegion.depth  = 1;

    readImage->changeLayoutTop(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               commandBuffer);
    commandBuffer->copySingleImage(*readImage, stagingImage.getImage(), copyRegion,
                                   VK_IMAGE_ASPECT_COLOR_BIT);
    commandBuffer->end();

    ANGLE_TRY(renderer->submitAndFinishCommandBuffer(*commandBuffer));

    // TODO(jmadill): parameters
    uint8_t *mapPointer = nullptr;
    ANGLE_TRY(stagingImage.getDeviceMemory().map(0, stagingImage.getSize(), 0, &mapPointer));

    const auto &angleFormat = renderTarget->format->format();

    // TODO(jmadill): Use pixel bytes from the ANGLE format directly.
    const auto &glFormat = gl::GetInternalFormatInfo(angleFormat.glInternalFormat);
    int inputPitch       = glFormat.pixelBytes * area.width;

    PackPixelsParams params;
    params.area        = area;
    params.format      = format;
    params.type        = type;
    params.outputPitch = inputPitch;
    params.pack        = glState.getPackState();

    PackPixels(params, angleFormat, inputPitch, mapPointer, reinterpret_cast<uint8_t *>(pixels));

    stagingImage.getDeviceMemory().unmap();

    return vk::NoError();
}

gl::Error FramebufferVk::blit(ContextImpl *context,
                              const gl::Rectangle &sourceArea,
                              const gl::Rectangle &destArea,
                              GLbitfield mask,
                              GLenum filter)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

bool FramebufferVk::checkStatus() const
{
    UNIMPLEMENTED();
    return bool();
}

void FramebufferVk::syncState(const gl::Framebuffer::DirtyBits &dirtyBits)
{
    // TODO(jmadill): Smarter update.
}

gl::Error FramebufferVk::getSamplePosition(size_t index, GLfloat *xy) const
{
    UNIMPLEMENTED();
    return gl::InternalError() << "getSamplePosition is unimplemented.";
}

}  // namespace rx
