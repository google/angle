//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "SampleApplication.h"

#ifdef _WIN32
#include "win32/Win32Timer.h"
#include "win32/Win32Window.h"
#else
#error unsupported OS.
#endif

SampleApplication::SampleApplication(const std::string& name, size_t width, size_t height,
                                     EGLint glesMajorVersion, EGLint requestedRenderer)
    : mSurface(EGL_NO_SURFACE),
      mContext(EGL_NO_CONTEXT),
      mDisplay(EGL_NO_DISPLAY),
      mClientVersion(glesMajorVersion),
      mRequestedRenderer(requestedRenderer),
      mWidth(width),
      mHeight(height),
      mName(name),
      mRunning(false),
#ifdef _WIN32
      mTimer(new Win32Timer()),
      mWindow(new Win32Window())
#endif
{
}

SampleApplication::~SampleApplication()
{
}

bool SampleApplication::initialize()
{
    return true;
}

void SampleApplication::destroy()
{
}

void SampleApplication::step(float dt, double totalTime)
{
}

void SampleApplication::draw()
{
}

void SampleApplication::swap()
{
    eglSwapBuffers(mDisplay, mSurface);
}

Window *SampleApplication::getWindow() const
{
    return mWindow.get();
}

EGLConfig SampleApplication::getConfig() const
{
    return mConfig;
}

EGLDisplay SampleApplication::getDisplay() const
{
    return mDisplay;
}

EGLSurface SampleApplication::getSurface() const
{
    return mSurface;
}

EGLContext SampleApplication::getContext() const
{
    return mContext;
}

int SampleApplication::run()
{
    if (!mWindow->initialize(mName, mWidth, mHeight))
    {
        return -1;
    }

    if (!initializeGL())
    {
        return -1;
    }

    mRunning = true;
    int result = 0;

    if (!initialize())
    {
        mRunning = false;
        result = -1;
    }

    mTimer->start();
    double prevTime = 0.0;

    while (mRunning)
    {
        double elapsedTime = mTimer->getElapsedTime();
        double deltaTime = elapsedTime - prevTime;

        step(static_cast<float>(deltaTime), elapsedTime);

        // Clear events that the application did not process from this frame
        Event event;
        while (popEvent(&event))
        {
            // If the application did not catch a close event, close now
            if (event.Type == Event::EVENT_CLOSED)
            {
                exit();
            }
        }

        if (!mRunning)
        {
            break;
        }

        draw();
        swap();

        mWindow->messageLoop();

        prevTime = elapsedTime;
    }

    destroy();
    destroyGL();
    mWindow->destroy();

    return result;
}

void SampleApplication::exit()
{
    mRunning = false;
}

bool SampleApplication::popEvent(Event *event)
{
    return mWindow->popEvent(event);
}

bool SampleApplication::initializeGL()
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

    mDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, mWindow->getNativeDisplay(), displayAttributes);
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

    mSurface = eglCreateWindowSurface(mDisplay, mConfig, mWindow->getNativeWindow(), surfaceAttributes);
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

void SampleApplication::destroyGL()
{
    eglDestroySurface(mDisplay, mSurface);
    mSurface = 0;

    eglDestroyContext(mDisplay, mContext);
    mContext = 0;

    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(mDisplay);
    mDisplay = EGL_NO_DISPLAY;
}
