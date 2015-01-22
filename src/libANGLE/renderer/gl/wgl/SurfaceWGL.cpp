//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SurfaceWGL.cpp: WGL implementation of egl::Surface

#include "libANGLE/renderer/gl/wgl/SurfaceWGL.h"

#include "common/debug.h"

namespace rx
{

SurfaceWGL::SurfaceWGL(egl::Display *display, const egl::Config *config, EGLint fixedSize, EGLint postSubBufferSupported,
                       EGLenum textureFormat, EGLenum textureType)
    : SurfaceGL(display, config, fixedSize, postSubBufferSupported, textureFormat, textureType)
{
}

SurfaceWGL::~SurfaceWGL()
{
}

egl::Error SurfaceWGL::swap()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceWGL::postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceWGL::querySurfacePointerANGLE(EGLint attribute, void **value)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceWGL::bindTexImage(EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceWGL::releaseTexImage(EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

void SurfaceWGL::setSwapInterval(EGLint interval)
{
    UNIMPLEMENTED();
}

EGLint SurfaceWGL::getWidth() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLint SurfaceWGL::getHeight() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLNativeWindowType SurfaceWGL::getWindowHandle() const
{
    UNIMPLEMENTED();
    return NULL;
}

}
