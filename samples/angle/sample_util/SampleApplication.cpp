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
                                     EGLint glesMajorVersion, RendererType requestedRenderer)
    : mSurface(EGL_NO_SURFACE),
      mContext(EGL_NO_CONTEXT),
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
    eglSwapBuffers(mWindow->getDisplay(), mSurface);
}

Window *SampleApplication::getWindow() const
{
    return mWindow.get();
}

EGLConfig SampleApplication::getConfig() const
{
    return mConfig;
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
    if (!mWindow->initialize(mName, mWidth, mHeight, mRequestedRenderer))
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
    if (!eglChooseConfig(mWindow->getDisplay(), configAttributes, &mConfig, 1, &configCount) || (configCount != 1))
    {
        destroyGL();
        return false;
    }

    const EGLint surfaceAttributes[] =
    {
        EGL_POST_SUB_BUFFER_SUPPORTED_NV, EGL_TRUE,
        EGL_NONE, EGL_NONE,
    };

    mSurface = eglCreateWindowSurface(mWindow->getDisplay(), mConfig, mWindow->getNativeWindow(), surfaceAttributes);
    if (mSurface == EGL_NO_SURFACE)
    {
        eglGetError(); // Clear error and try again
        mSurface = eglCreateWindowSurface(mWindow->getDisplay(), mConfig, NULL, NULL);
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
    mContext = eglCreateContext(mWindow->getDisplay(), mConfig, NULL, contextAttibutes);
    if (eglGetError() != EGL_SUCCESS)
    {
        destroyGL();
        return false;
    }

    eglMakeCurrent(mWindow->getDisplay(), mSurface, mSurface, mContext);
    if (eglGetError() != EGL_SUCCESS)
    {
        destroyGL();
        return false;
    }

    // Turn off vsync
    eglSwapInterval(mWindow->getDisplay(), 0);

    return true;
}

void SampleApplication::destroyGL()
{
    eglDestroySurface(mWindow->getDisplay(), mSurface);
    mSurface = 0;

    eglDestroyContext(mWindow->getDisplay(), mContext);
    mContext = 0;
}
