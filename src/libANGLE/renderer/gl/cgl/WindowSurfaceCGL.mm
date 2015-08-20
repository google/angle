//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WindowSurfaceCGL.cpp: CGL implementation of egl::Surface for windows

#include "libANGLE/renderer/gl/cgl/WindowSurfaceCGL.h"

#include "common/debug.h"
#include "libANGLE/renderer/gl/cgl/DisplayCGL.h"

namespace rx
{

WindowSurfaceCGL::WindowSurfaceCGL(RendererGL *renderer) : SurfaceGL(renderer)
{
}

WindowSurfaceCGL::~WindowSurfaceCGL()
{
}

egl::Error WindowSurfaceCGL::initialize()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceCGL::makeCurrent()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceCGL::swap()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceCGL::postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceCGL::querySurfacePointerANGLE(EGLint attribute, void **value)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceCGL::bindTexImage(EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceCGL::releaseTexImage(EGLint buffer)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_SUCCESS);
}

void WindowSurfaceCGL::setSwapInterval(EGLint interval)
{
    UNIMPLEMENTED();
}

EGLint WindowSurfaceCGL::getWidth() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLint WindowSurfaceCGL::getHeight() const
{
    UNIMPLEMENTED();
    return 0;
}

EGLint WindowSurfaceCGL::isPostSubBufferSupported() const
{
    UNIMPLEMENTED();
    return EGL_FALSE;
}

EGLint WindowSurfaceCGL::getSwapBehavior() const
{
    return EGL_BUFFER_DESTROYED;
}

}
