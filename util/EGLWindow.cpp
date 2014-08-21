//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <cassert>

#include "EGLWindow.h"
#include "OSWindow.h"

#ifdef _WIN32
#include "win32/Win32Timer.h"
#include "win32/Win32Window.h"
#else
#error unsupported OS.
#endif

EGLWindow::EGLWindow(size_t width, size_t height,
                     EGLint glesMajorVersion, EGLint requestedRenderer)
    : mSurface(EGL_NO_SURFACE),
      mContext(EGL_NO_CONTEXT),
      mDisplay(EGL_NO_DISPLAY),
      mClientVersion(glesMajorVersion),
      mRequestedRenderer(requestedRenderer),
      mWidth(width),
      mHeight(height)
{
}

EGLWindow::~EGLWindow()
{
    destroyGL();
}

void EGLWindow::swap()
{
    eglSwapBuffers(mDisplay, mSurface);
}

EGLConfig EGLWindow::getConfig() const
{
    return mConfig;
}

EGLDisplay EGLWindow::getDisplay() const
{
    return mDisplay;
}

EGLSurface EGLWindow::getSurface() const
{
    return mSurface;
}

EGLContext EGLWindow::getContext() const
{
    return mContext;
}

bool EGLWindow::initializeGL(const OSWindow *osWindow)
{
    PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (!eglGetPlatformDisplayEXT)
    {
        return false;
    }

    const EGLint displayAttributes[] =
    {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, mRequestedRenderer,
        EGL_NONE,
    };

    mDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, osWindow->getNativeDisplay(), displayAttributes);
    if (mDisplay == EGL_NO_DISPLAY)
    {
        destroyGL();
        return false;
    }

    EGLint majorVersion, minorVersion;
    if (!eglInitialize(mDisplay, &majorVersion, &minorVersion))
    {
        destroyGL();
        return false;
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    if (eglGetError() != EGL_SUCCESS)
    {
        destroyGL();
        return false;
    }

    const EGLint configAttributes[] =
    {
        EGL_RED_SIZE,       8,
        EGL_GREEN_SIZE,     8,
        EGL_BLUE_SIZE,      8,
        EGL_ALPHA_SIZE,     8,
        EGL_DEPTH_SIZE,     24,
        EGL_STENCIL_SIZE,   8,
        EGL_SAMPLE_BUFFERS, EGL_DONT_CARE,
        EGL_NONE
    };

    EGLint configCount;
    if (!eglChooseConfig(mDisplay, configAttributes, &mConfig, 1, &configCount) || (configCount != 1))
    {
        destroyGL();
        return false;
    }

    const EGLint surfaceAttributes[] =
    {
        EGL_POST_SUB_BUFFER_SUPPORTED_NV, EGL_TRUE,
        EGL_NONE, EGL_NONE,
    };

    mSurface = eglCreateWindowSurface(mDisplay, mConfig, osWindow->getNativeWindow(), surfaceAttributes);
    if (mSurface == EGL_NO_SURFACE)
    {
        eglGetError(); // Clear error and try again
        mSurface = eglCreateWindowSurface(mDisplay, mConfig, NULL, NULL);
    }

    if (eglGetError() != EGL_SUCCESS)
    {
        destroyGL();
        return false;
    }

    EGLint contextAttibutes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, mClientVersion,
        EGL_NONE
    };
    mContext = eglCreateContext(mDisplay, mConfig, NULL, contextAttibutes);
    if (eglGetError() != EGL_SUCCESS)
    {
        destroyGL();
        return false;
    }

    eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);
    if (eglGetError() != EGL_SUCCESS)
    {
        destroyGL();
        return false;
    }

    // Turn off vsync
    eglSwapInterval(mDisplay, 0);

    return true;
}

void EGLWindow::destroyGL()
{
    if (mSurface != EGL_NO_SURFACE)
    {
        assert(mDisplay != EGL_NO_DISPLAY);
        eglDestroySurface(mDisplay, mSurface);
        mSurface = EGL_NO_SURFACE;
    }

    if (mContext != EGL_NO_CONTEXT)
    {
        assert(mDisplay != EGL_NO_DISPLAY);
        eglDestroyContext(mDisplay, mContext);
        mContext = EGL_NO_CONTEXT;
    }

    if (mDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
    }
}
