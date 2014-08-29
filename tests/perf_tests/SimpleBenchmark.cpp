//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "SimpleBenchmark.h"
#include <iostream>

SimpleBenchmark::SimpleBenchmark(const std::string &name, size_t width, size_t height, EGLint glesMajorVersion, EGLint requestedRenderer)
    : mNumFrames(0),
      mName(name),
      mRunning(false),
      mDrawIterations(10),
      mRunTimeSeconds(5.0)
{
    mOSWindow.reset(CreateOSWindow());
    mEGLWindow.reset(new EGLWindow(width, height, glesMajorVersion, requestedRenderer));
    mTimer.reset(CreateTimer());
}

bool SimpleBenchmark::initialize()
{
    std::cout << "========= " << mName << " =========" << std::endl;
    return initializeBenchmark();
}

void SimpleBenchmark::destroy()
{
    double totalTime = mTimer->getElapsedTime();
    std::cout << " - total time: " << totalTime << " sec" << std::endl;
    std::cout << " - frames: " << mNumFrames << std::endl;
    std::cout << " - average frame time: " << 1000.0 * totalTime / mNumFrames << " msec" << std::endl;
    std::cout << "=========" << std::endl << std::endl;

    destroyBenchmark();
}

void SimpleBenchmark::step(float dt, double totalTime)
{
    stepBenchmark(dt, totalTime);
}

void SimpleBenchmark::draw()
{
    if (mTimer->getElapsedTime() > mRunTimeSeconds) {
        mRunning = false;
        return;
    }

    ++mNumFrames;

    beginDrawBenchmark();

    for (unsigned int iteration = 0; iteration < mDrawIterations; ++iteration)
    {
        drawBenchmark();
    }

    endDrawBenchmark();
}

int SimpleBenchmark::run()
{
    if (!mOSWindow->initialize(mName, mEGLWindow->getWidth(), mEGLWindow->getHeight()))
    {
        return -1;
    }

    if (!mEGLWindow->initializeGL(mOSWindow.get()))
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
                mRunning = false;
            }
        }

        if (!mRunning)
        {
            break;
        }

        draw();
        mEGLWindow->swap();

        mOSWindow->messageLoop();

        prevTime = elapsedTime;
    }

    destroy();
    mEGLWindow->destroyGL();
    mOSWindow->destroy();

    return result;
}

bool SimpleBenchmark::popEvent(Event *event)
{
    return mOSWindow->popEvent(event);
}

OSWindow *SimpleBenchmark::getWindow()
{
    return mOSWindow.get();
}