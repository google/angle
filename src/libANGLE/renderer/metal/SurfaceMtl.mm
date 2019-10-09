//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SurfaceMtl.mm:
//    Implements the class methods for SurfaceMtl.
//

#include "libANGLE/renderer/metal/SurfaceMtl.h"

#include "libANGLE/renderer/metal/FrameBufferMtl.h"

namespace rx
{

SurfaceMtl::SurfaceMtl(const egl::SurfaceState &state,
                       EGLNativeWindowType window,
                       EGLint width,
                       EGLint height)
    : SurfaceImpl(state)
{
    UNIMPLEMENTED();
}

SurfaceMtl::~SurfaceMtl() {}

void SurfaceMtl::destroy(const egl::Display *display)
{
    UNIMPLEMENTED();
}

egl::Error SurfaceMtl::initialize(const egl::Display *display)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

FramebufferImpl *SurfaceMtl::createDefaultFramebuffer(const gl::Context *context,
                                                      const gl::FramebufferState &state)
{
    auto fbo = new FramebufferMtl(state);

    return fbo;
}

egl::Error SurfaceMtl::makeCurrent(const gl::Context *context)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

egl::Error SurfaceMtl::unMakeCurrent(const gl::Context *context)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

egl::Error SurfaceMtl::swap(const gl::Context *context)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

egl::Error SurfaceMtl::postSubBuffer(const gl::Context *context,
                                     EGLint x,
                                     EGLint y,
                                     EGLint width,
                                     EGLint height)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

egl::Error SurfaceMtl::querySurfacePointerANGLE(EGLint attribute, void **value)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

egl::Error SurfaceMtl::bindTexImage(const gl::Context *context, gl::Texture *texture, EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

egl::Error SurfaceMtl::releaseTexImage(const gl::Context *context, EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

egl::Error SurfaceMtl::getSyncValues(EGLuint64KHR *ust, EGLuint64KHR *msc, EGLuint64KHR *sbc)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

void SurfaceMtl::setSwapInterval(EGLint interval) {}

void SurfaceMtl::setFixedWidth(EGLint width)
{
    UNIMPLEMENTED();
}

void SurfaceMtl::setFixedHeight(EGLint height)
{
    UNIMPLEMENTED();
}

// width and height can change with client window resizing
EGLint SurfaceMtl::getWidth() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLint SurfaceMtl::getHeight() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLint SurfaceMtl::isPostSubBufferSupported() const
{
    return EGL_FALSE;
}

EGLint SurfaceMtl::getSwapBehavior() const
{
    return EGL_BUFFER_DESTROYED;
}

angle::Result SurfaceMtl::getAttachmentRenderTarget(const gl::Context *context,
                                                    GLenum binding,
                                                    const gl::ImageIndex &imageIndex,
                                                    GLsizei samples,
                                                    FramebufferAttachmentRenderTarget **rtOut)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

}
