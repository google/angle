//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WindowSurfaceGLX.cpp: GLX implementation of egl::Surface for windows

#include "libANGLE/renderer/gl/glx/WindowSurfaceGLX.h"

#include "common/debug.h"

namespace rx
{

WindowSurfaceGLX::WindowSurfaceGLX()
    : SurfaceGL()
{
}

WindowSurfaceGLX::~WindowSurfaceGLX()
{
}

egl::Error WindowSurfaceGLX::initialize()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::makeCurrent()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::swap()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::querySurfacePointerANGLE(EGLint attribute, void **value)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::bindTexImage(EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::releaseTexImage(EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

void WindowSurfaceGLX::setSwapInterval(EGLint interval)
{
    UNIMPLEMENTED();
}

EGLint WindowSurfaceGLX::getWidth() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLint WindowSurfaceGLX::getHeight() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLint WindowSurfaceGLX::isPostSubBufferSupported() const
{
    UNIMPLEMENTED();
    return EGL_FALSE;
}

}
