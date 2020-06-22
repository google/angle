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

#define ANGLE_STATIC_ASSERT_SIZE(className, classSize) \
    static_assert(                                                       \
        sizeof(className) == classSize,                                  \
        ANGLE_STRINGIFY(className) " class has changed. Please "         \
            "update its serialize method in " __FILE__);

namespace angle
{

Result ReadPixelsFromAttachment(gl::Context *context,
                                gl::Framebuffer *framebuffer,
                                const gl::FramebufferAttachment &framebufferAttachment,
                                ScratchBuffer *scratchBuffer,
                                MemoryBuffer **pixels)
{
    gl::Extents extents = framebufferAttachment.getSize();
    gl::Format format   = framebufferAttachment.getFormat();
    ANGLE_CHECK_GL_ALLOC(
        context,
        scratchBuffer->get(format.info->pixelBytes * extents.width * extents.height, pixels));
    ANGLE_TRY(framebuffer->readPixels(context, gl::Rectangle{0, 0, extents.width, extents.height},
                                      format.info->format, format.info->type, gl::PixelPackState{},
                                      nullptr, (*pixels)->data()));
    return Result::Continue;
}

Result SerializeContext(gl::BinaryOutputStream *bos, gl::Context *context)
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

Result SerializeFramebuffer(gl::Context *context,
                            gl::BinaryOutputStream *bos,
                            gl::Framebuffer *framebuffer)
{
#if defined(ANGLE_IS_64_BIT_CPU) && (defined(ANGLE_IS_WIN) || defined(ANGLE_IS_LINUX))
    constexpr size_t kSizeOfGlFramebuffer = 744;
    ANGLE_STATIC_ASSERT_SIZE(gl::Framebuffer, kSizeOfGlFramebuffer)
#endif
    return SerializeFramebufferState(context, bos, framebuffer, framebuffer->getState());
}

Result SerializeFramebufferState(gl::Context *context,
                                 gl::BinaryOutputStream *bos,
                                 gl::Framebuffer *framebuffer,
                                 const gl::FramebufferState &framebufferState)
{
#if defined(ANGLE_IS_64_BIT_CPU) && (defined(ANGLE_IS_WIN) || defined(ANGLE_IS_LINUX))
    constexpr size_t kSizeOfGlFramebufferState = 488;
    ANGLE_STATIC_ASSERT_SIZE(gl::FramebufferState, kSizeOfGlFramebufferState)
#endif
    bos->writeInt(framebufferState.id().value);
    bos->writeString(framebufferState.getLabel());
    bos->writeIntVector(framebufferState.getDrawBufferStates());
    bos->writeInt(framebufferState.getReadBufferState());
    bos->writeInt(framebufferState.getDefaultWidth());
    bos->writeInt(framebufferState.getDefaultHeight());
    bos->writeInt(framebufferState.getDefaultSamples());
    bos->writeInt(framebufferState.getDefaultFixedSampleLocations());
    bos->writeInt(framebufferState.getDefaultLayers());

    context->bindFramebuffer(GL_FRAMEBUFFER, framebufferState.id());
    ANGLE_TRY(framebuffer->syncState(context, GL_FRAMEBUFFER));

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
    scratchBuffer.clear();
    return Result::Continue;
}

Result SerializeFramebufferAttachment(gl::Context *context,
                                      gl::BinaryOutputStream *bos,
                                      ScratchBuffer *scratchBuffer,
                                      gl::Framebuffer *framebuffer,
                                      const gl::FramebufferAttachment &framebufferAttachment)
{
#if defined(ANGLE_IS_64_BIT_CPU) && (defined(ANGLE_IS_WIN) || defined(ANGLE_IS_LINUX))
    constexpr size_t kSizeOfGlFramebufferAttachment = 48;
    ANGLE_STATIC_ASSERT_SIZE(gl::FramebufferAttachment, kSizeOfGlFramebufferAttachment)
#endif
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
    MemoryBuffer *pixelsPtr = nullptr;
    context->readBuffer(framebufferAttachment.getBinding());
    ANGLE_TRY(framebuffer->syncState(context, GL_FRAMEBUFFER));
    ANGLE_TRY(ReadPixelsFromAttachment(context, framebuffer, framebufferAttachment, scratchBuffer,
                                       &pixelsPtr));
    bos->writeBytes(pixelsPtr->data(), pixelsPtr->size());
    return Result::Continue;
}

void SerializeImageIndex(gl::BinaryOutputStream *bos, const gl::ImageIndex &imageIndex)
{
    // size of ImageIndex on 64-bit and 86-bit systems are the same
    constexpr size_t kSizeOfGlImageIndex = 16;
    ANGLE_STATIC_ASSERT_SIZE(gl::ImageIndex, kSizeOfGlImageIndex)

    bos->writeEnum(imageIndex.getType());
    bos->writeInt(imageIndex.getLevelIndex());
    bos->writeInt(imageIndex.getLayerIndex());
    bos->writeInt(imageIndex.getLayerCount());
}

}  // namespace angle
