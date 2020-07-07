//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// frame_capture_utils.cpp:
//   ANGLE frame capture util implementation.
//

#include "libANGLE/frame_capture_utils.h"

#include "common/MemoryBuffer.h"
#include "common/angleutils.h"
#include "libANGLE/BinaryStream.h"
#include "libANGLE/Context.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/renderer/FramebufferImpl.h"

namespace angle
{

bool IsValidColorAttachmentBinding(GLenum binding, size_t colorAttachmentsCount)
{
    return binding == GL_BACK || (binding >= GL_COLOR_ATTACHMENT0 &&
                                  (binding - GL_COLOR_ATTACHMENT0) < colorAttachmentsCount);
}

Result ReadPixelsFromAttachment(const gl::Context *context,
                                gl::Framebuffer *framebuffer,
                                const gl::FramebufferAttachment &framebufferAttachment,
                                ScratchBuffer *scratchBuffer,
                                MemoryBuffer **pixels)
{
    gl::Extents extents       = framebufferAttachment.getSize();
    GLenum binding            = framebufferAttachment.getBinding();
    gl::InternalFormat format = *framebufferAttachment.getFormat().info;
    if (IsValidColorAttachmentBinding(binding,
                                      framebuffer->getState().getColorAttachments().size()))
    {
        format = framebuffer->getImplementation()->getImplementationColorReadFormat(context);
    }
    ANGLE_CHECK_GL_ALLOC(const_cast<gl::Context *>(context),
                         scratchBuffer->getInitialized(
                             format.pixelBytes * extents.width * extents.height, pixels, 0));
    ANGLE_TRY(framebuffer->readPixels(context, gl::Rectangle{0, 0, extents.width, extents.height},
                                      format.format, format.type, gl::PixelPackState{}, nullptr,
                                      (*pixels)->data()));
    return Result::Continue;
}

Result SerializeContext(gl::BinaryOutputStream *bos, const gl::Context *context)
{
    const gl::FramebufferManager &framebufferManager =
        context->getState().getFramebufferManagerForCapture();
    for (const auto &framebuffer : framebufferManager)
    {
        gl::Framebuffer *framebufferPtr = framebuffer.second;
        ANGLE_TRY(SerializeFramebuffer(context, bos, framebufferPtr));
    }
    return Result::Continue;
}

Result SerializeFramebuffer(const gl::Context *context,
                            gl::BinaryOutputStream *bos,
                            gl::Framebuffer *framebuffer)
{
    return SerializeFramebufferState(context, bos, framebuffer, framebuffer->getState());
}

Result SerializeFramebufferState(const gl::Context *context,
                                 gl::BinaryOutputStream *bos,
                                 gl::Framebuffer *framebuffer,
                                 const gl::FramebufferState &framebufferState)
{
    bos->writeInt(framebufferState.id().value);
    bos->writeString(framebufferState.getLabel());
    bos->writeIntVector(framebufferState.getDrawBufferStates());
    bos->writeInt(framebufferState.getReadBufferState());
    bos->writeInt(framebufferState.getDefaultWidth());
    bos->writeInt(framebufferState.getDefaultHeight());
    bos->writeInt(framebufferState.getDefaultSamples());
    bos->writeInt(framebufferState.getDefaultFixedSampleLocations());
    bos->writeInt(framebufferState.getDefaultLayers());

    const std::vector<gl::FramebufferAttachment> &colorAttachments =
        framebufferState.getColorAttachments();
    ScratchBuffer scratchBuffer(1);
    for (const gl::FramebufferAttachment &colorAttachment : colorAttachments)
    {
        if (colorAttachment.isAttached())
        {
            ANGLE_TRY(SerializeFramebufferAttachment(context, bos, &scratchBuffer, framebuffer,
                                                     colorAttachment));
        }
    }
    if (framebuffer->getDepthStencilAttachment())
    {
        ANGLE_TRY(SerializeFramebufferAttachment(context, bos, &scratchBuffer, framebuffer,
                                                 *framebuffer->getDepthStencilAttachment()));
    }
    else
    {
        if (framebuffer->getDepthAttachment())
        {
            ANGLE_TRY(SerializeFramebufferAttachment(context, bos, &scratchBuffer, framebuffer,
                                                     *framebuffer->getDepthAttachment()));
        }
        if (framebuffer->getStencilAttachment())
        {
            ANGLE_TRY(SerializeFramebufferAttachment(context, bos, &scratchBuffer, framebuffer,
                                                     *framebuffer->getStencilAttachment()));
        }
    }
    scratchBuffer.clear();
    return Result::Continue;
}

Result SerializeFramebufferAttachment(const gl::Context *context,
                                      gl::BinaryOutputStream *bos,
                                      ScratchBuffer *scratchBuffer,
                                      gl::Framebuffer *framebuffer,
                                      const gl::FramebufferAttachment &framebufferAttachment)
{
    bos->writeInt(framebufferAttachment.type());
    // serialize target variable
    bos->writeInt(framebufferAttachment.getBinding());
    if (framebufferAttachment.type() == GL_TEXTURE)
    {
        SerializeImageIndex(bos, framebufferAttachment.getTextureImageIndex());
    }
    bos->writeInt(framebufferAttachment.getNumViews());
    bos->writeInt(framebufferAttachment.isMultiview());
    bos->writeInt(framebufferAttachment.getBaseViewIndex());
    bos->writeInt(framebufferAttachment.getRenderToTextureSamples());

    GLenum prevReadBufferState = framebuffer->getReadBufferState();
    GLenum binding             = framebufferAttachment.getBinding();
    if (IsValidColorAttachmentBinding(binding,
                                      framebuffer->getState().getColorAttachments().size()))
    {
        framebuffer->setReadBuffer(framebufferAttachment.getBinding());
        ANGLE_TRY(framebuffer->syncState(context, GL_FRAMEBUFFER));
    }
    MemoryBuffer *pixelsPtr = nullptr;
    ANGLE_TRY(ReadPixelsFromAttachment(context, framebuffer, framebufferAttachment, scratchBuffer,
                                       &pixelsPtr));
    bos->writeBytes(pixelsPtr->data(), pixelsPtr->size());
    // Reset framebuffer state
    framebuffer->setReadBuffer(prevReadBufferState);
    return Result::Continue;
}

void SerializeImageIndex(gl::BinaryOutputStream *bos, const gl::ImageIndex &imageIndex)
{
    bos->writeEnum(imageIndex.getType());
    bos->writeInt(imageIndex.getLevelIndex());
    bos->writeInt(imageIndex.getLayerIndex());
    bos->writeInt(imageIndex.getLayerCount());
}

}  // namespace angle
