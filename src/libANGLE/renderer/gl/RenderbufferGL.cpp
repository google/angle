//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferGL.cpp: Implements the class methods for RenderbufferGL.

#include "libANGLE/renderer/gl/RenderbufferGL.h"

#include "common/debug.h"
#include "common/mathutil.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Context.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/gl/BlitGL.h"
#include "libANGLE/renderer/gl/ContextGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/ImageGL.h"
#include "libANGLE/renderer/gl/StateManagerGL.h"
#include "libANGLE/renderer/gl/formatutilsgl.h"
#include "libANGLE/renderer/gl/renderergl_utils.h"
#include "platform/autogen/FeaturesGL_autogen.h"

namespace rx
{

namespace
{

// Mirrors the PowerVR GLES driver's renderbuffer storage sizing closely enough
// to predict when its 32-bit byte product wraps. Every adjustment here rounds
// up, so a mismatch with a non-default driver configuration can only reject
// more, never less.
bool PowerVRRenderbufferStorageSizeWraps(GLenum internalformat,
                                         GLsizei width,
                                         GLsizei height,
                                         GLsizei samples)
{
    const gl::InternalFormat &fmt = gl::GetSizedInternalFormatInfo(internalformat);
    const bool depthOrStencil     = fmt.depthBits > 0 || fmt.stencilBits > 0;

    uint64_t allocWidth  = static_cast<uint64_t>(std::max<GLsizei>(width, 1));
    uint64_t allocHeight = static_cast<uint64_t>(std::max<GLsizei>(height, 1));

    if (depthOrStencil)
    {
        // Depth/stencil surfaces are aligned to the 32x32 ZLS tile and then,
        // because the driver stores them twiddled, each axis is rounded up to
        // a power of two before the byte product is formed.
        constexpr uint64_t kZlsTile = 32;
        allocWidth                  = rx::roundUp(allocWidth, kZlsTile);
        allocHeight                 = rx::roundUp(allocHeight, kZlsTile);
        allocWidth                  = gl::ceilPow2(static_cast<unsigned int>(allocWidth));
        allocHeight                 = gl::ceilPow2(static_cast<unsigned int>(allocHeight));
    }

    // Driver bytes per pixel for the plane whose product is formed first.
    uint64_t bytesPerPixel = fmt.pixelBytes;
    bool separateStencil   = false;
    if (fmt.depthBits > 0)
    {
        // Every depth-bearing plane is stored as 4 bytes per pixel. The packed
        // float depth/stencil format is a 4-byte depth plane followed by a
        // stencil plane of one quarter of that size.
        bytesPerPixel   = 4;
        separateStencil = (internalformat == GL_DEPTH32F_STENCIL8);
    }
    else if (fmt.stencilBits > 0)
    {
        bytesPerPixel = 1;
    }
    else if (internalformat == GL_RGB8)
    {
        // Stored as RGBX8 unless the driver build packs RGB8.
        bytesPerPixel = 4;
    }

    // The driver rounds the requested sample count up to 2, 4 or 8 and scales
    // the byte product by the same factor (ZLS extents 1x2, 2x2, 2x4).
    uint64_t sampleScale = 1;
    if (samples > 0)
    {
        sampleScale = (samples <= 2) ? 2 : ((samples <= 4) ? 4 : 8);
    }

    angle::CheckedNumeric<uint64_t> bytes(bytesPerPixel);
    bytes *= sampleScale;
    bytes *= allocWidth;
    bytes *= allocHeight;
    if (separateStencil)
    {
        bytes += bytes / 4;
    }

    constexpr uint64_t kDriverWrap = uint64_t{1} << 32;
    return !bytes.IsValid() || bytes.ValueOrDie() >= kDriverWrap;
}

angle::Result RejectIfDriverSizeWraps(const gl::Context *context,
                                      const angle::FeaturesGL &features,
                                      GLenum internalformat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLsizei samples)
{
    if (!features.rejectRenderbufferStorageWrappingDriverSize.enabled)
    {
        return angle::Result::Continue;
    }
    ANGLE_CHECK(GetImplAs<ContextGL>(context),
                !PowerVRRenderbufferStorageSizeWraps(internalformat, width, height, samples),
                "Renderbuffer storage size exceeds the driver's 32-bit allocator",
                GL_OUT_OF_MEMORY);
    return angle::Result::Continue;
}

}  // namespace

RenderbufferGL::RenderbufferGL(const gl::RenderbufferState &state, GLuint id)
    : RenderbufferImpl(state), mRenderbufferID(id)
{}

RenderbufferGL::~RenderbufferGL()
{
    ASSERT(mRenderbufferID == 0);
}

void RenderbufferGL::onDestroy(const gl::Context *context)
{
    StateManagerGL *stateManager = GetStateManagerGL(context);
    stateManager->deleteRenderbuffer(mRenderbufferID);
    mRenderbufferID = 0;
}

angle::Result RenderbufferGL::setStorage(const gl::Context *context,
                                         GLenum internalformat,
                                         GLsizei width,
                                         GLsizei height)
{
    const FunctionsGL *functions      = GetFunctionsGL(context);
    StateManagerGL *stateManager      = GetStateManagerGL(context);
    const angle::FeaturesGL &features = GetFeaturesGL(context);

    nativegl::RenderbufferFormat renderbufferFormat =
        nativegl::GetRenderbufferFormat(functions, features, internalformat);

    ANGLE_TRY(RejectIfDriverSizeWraps(context, features, renderbufferFormat.internalFormat, width,
                                      height, 0));

    if (features.reattachFboDepthStencilOnReallocation.enabled)
    {
        onStateChange(angle::SubjectMessage::ObjectReallocated);
    }

    stateManager->bindRenderbuffer(GL_RENDERBUFFER, mRenderbufferID);

    ANGLE_GL_TRY_ALWAYS_CHECK(
        context, functions->renderbufferStorage(GL_RENDERBUFFER, renderbufferFormat.internalFormat,
                                                width, height));

    mNativeInternalFormat = renderbufferFormat.internalFormat;

    return angle::Result::Continue;
}

angle::Result RenderbufferGL::setStorageMultisample(const gl::Context *context,
                                                    GLsizei samples,
                                                    GLenum internalformat,
                                                    GLsizei width,
                                                    GLsizei height,
                                                    gl::MultisamplingMode mode)
{
    const FunctionsGL *functions      = GetFunctionsGL(context);
    StateManagerGL *stateManager      = GetStateManagerGL(context);
    const angle::FeaturesGL &features = GetFeaturesGL(context);

    nativegl::RenderbufferFormat renderbufferFormat =
        nativegl::GetRenderbufferFormat(functions, features, internalformat);

    ANGLE_TRY(RejectIfDriverSizeWraps(context, features, renderbufferFormat.internalFormat, width,
                                      height, samples));

    if (features.reattachFboDepthStencilOnReallocation.enabled)
    {
        onStateChange(angle::SubjectMessage::ObjectReallocated);
    }

    stateManager->bindRenderbuffer(GL_RENDERBUFFER, mRenderbufferID);

    if (mode == gl::MultisamplingMode::Regular)
    {
        ANGLE_GL_TRY_ALWAYS_CHECK(context, functions->renderbufferStorageMultisample(
                                               GL_RENDERBUFFER, samples,
                                               renderbufferFormat.internalFormat, width, height));
    }
    else
    {
        ASSERT(mode == gl::MultisamplingMode::MultisampledRenderToTexture);

        if (functions->renderbufferStorageMultisampleEXT)
        {
            ANGLE_GL_TRY_ALWAYS_CHECK(
                context,
                functions->renderbufferStorageMultisampleEXT(
                    GL_RENDERBUFFER, samples, renderbufferFormat.internalFormat, width, height));
        }
        else
        {
            ASSERT(functions->renderbufferStorageMultisampleIMG);
            ANGLE_GL_TRY_ALWAYS_CHECK(
                context,
                functions->renderbufferStorageMultisampleIMG(
                    GL_RENDERBUFFER, samples, renderbufferFormat.internalFormat, width, height));
        }
    }

    mNativeInternalFormat = renderbufferFormat.internalFormat;

    return angle::Result::Continue;
}

angle::Result RenderbufferGL::setStorageEGLImageTarget(const gl::Context *context,
                                                       egl::Image *image)
{
    ImageGL *imageGL = GetImplAs<ImageGL>(image);
    return imageGL->setRenderbufferStorage(context, this, &mNativeInternalFormat);
}

GLuint RenderbufferGL::getRenderbufferID() const
{
    return mRenderbufferID;
}

angle::Result RenderbufferGL::initializeContents(const gl::Context *context,
                                                 GLenum binding,
                                                 const gl::ImageIndex &imageIndex)
{
    BlitGL *blitter = GetBlitGL(context);
    return blitter->clearRenderbuffer(context, this, mNativeInternalFormat);
}

GLenum RenderbufferGL::getNativeInternalFormat() const
{
    return mNativeInternalFormat;
}

}  // namespace rx
