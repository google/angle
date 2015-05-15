//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WindowSurfaceGLX.cpp: GLX implementation of egl::Surface for windows

#include "libANGLE/renderer/gl/glx/WindowSurfaceGLX.h"

#include "common/debug.h"

#include "libANGLE/renderer/gl/glx/FunctionsGLX.h"

namespace rx
{

WindowSurfaceGLX::WindowSurfaceGLX(const FunctionsGLX &glx, EGLNativeWindowType window, Display *display, GLXContext context, GLXFBConfig fbConfig)
    : SurfaceGL(),
      mGLX(glx),
      mParent(window),
      mDisplay(display),
      mContext(context),
      mFBConfig(fbConfig),
      mWindow(0),
      mGLXWindow(0)
{
}

WindowSurfaceGLX::~WindowSurfaceGLX()
{
    if (mGLXWindow)
    {
        mGLX.destroyWindow(mGLXWindow);
    }

    if (mWindow)
    {
        XDestroyWindow(mDisplay, mWindow);
    }
}

egl::Error WindowSurfaceGLX::initialize()
{
    // The visual of the X window, GLX window and GLX context must match,
    // however we received a user-created window that can have any visual
    // and wouldn't work with our GLX context. To work in all cases, we
    // create a child window with the right visual that covers all of its
    // parent.

    XVisualInfo *visualInfo = mGLX.getVisualFromFBConfig(mFBConfig);
    if (!visualInfo)
    {
        return egl::Error(EGL_BAD_NATIVE_WINDOW, "Failed to get the XVisualInfo for the child window.");
    }
    Visual* visual = visualInfo->visual;

    XWindowAttributes parentAttribs;
    XGetWindowAttributes(mDisplay, mParent, &parentAttribs);

    // The depth, colormap and visual must match otherwise we get a X error
    // so we specify the colormap attribute. Also we do not want the window
    // to be taken into account for input so we specify the event and
    // do-not-propagate masks to 0 (the defaults).
    XSetWindowAttributes attributes;
    unsigned long attributeMask = CWColormap;

    Colormap colormap = XCreateColormap(mDisplay, mParent, visual, AllocNone);
    if(!colormap)
    {
        XFree(visualInfo);
        return egl::Error(EGL_BAD_NATIVE_WINDOW, "Failed to create the Colormap for the child window.");
    }
    attributes.colormap = colormap;

    //TODO(cwallez) set up our own error handler to see if the call failed
    mWindow = XCreateWindow(mDisplay, mParent, 0, 0, parentAttribs.width, parentAttribs.height,
                            0, visualInfo->depth, InputOutput, visual, attributeMask, &attributes);
    mGLXWindow = mGLX.createWindow(mFBConfig, mWindow, nullptr);

    XMapWindow(mDisplay, mWindow);
    XFlush(mDisplay);

    XFree(visualInfo);
    XFreeColormap(mDisplay, colormap);

    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::makeCurrent()
{
    if (mGLX.makeCurrent(mGLXWindow, mContext) != True)
    {
        return egl::Error(EGL_BAD_DISPLAY);
    }
    return egl::Error(EGL_SUCCESS);
}

egl::Error WindowSurfaceGLX::swap()
{
    //TODO(cwallez) resize support
    //TODO(cwallez) set up our own error handler to see if the call failed
    mGLX.swapBuffers(mGLXWindow);
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
    // TODO(cwallez) WGL has this, implement it
}

EGLint WindowSurfaceGLX::getWidth() const
{
    Window root;
    int x, y;
    unsigned width, height, border, depth;
    if (!XGetGeometry(mDisplay, mParent, &root, &x, &y, &width, &height, &border, &depth))
    {
        return 0;
    }
    return width;
}

EGLint WindowSurfaceGLX::getHeight() const
{
    Window root;
    int x, y;
    unsigned width, height, border, depth;
    if (!XGetGeometry(mDisplay, mParent, &root, &x, &y, &width, &height, &border, &depth))
    {
        return 0;
    }
    return height;
}

EGLint WindowSurfaceGLX::isPostSubBufferSupported() const
{
    UNIMPLEMENTED();
    return EGL_FALSE;
}

}
