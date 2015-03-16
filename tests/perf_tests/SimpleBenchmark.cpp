//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "SimpleBenchmark.h"

#include "third_party/perf/perf_test.h"

#include <iostream>
#include <cassert>

std::string BenchmarkParams::suffix() const
{
    switch (requestedRenderer)
    {
        case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE: return "_d3d11";
        case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE: return "_d3d9";
        default: assert(0); return "_unk";
    }
}

SimpleBenchmark::SimpleBenchmark(const std::string &name, size_t width, size_t height,
                                 EGLint glesMajorVersion, const BenchmarkParams &params)
    : mNumFrames(0),
      mName(name),
      mRunning(false),
      mDrawIterations(10),
      mRunTimeSeconds(5.0),
      mSuffix(params.suffix())
{
    mOSWindow.reset(CreateOSWindow());
    mEGLWindow.reset(new EGLWindow(width, height, glesMajorVersion, EGLPlatformParameters(params.requestedRenderer)));
    mTimer.reset(CreateTimer());
}

bool SimpleBenchmark::initialize()
{
    return initializeBenchmark();
}

void SimpleBenchmark::printResult(const std::string &trace, double value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void SimpleBenchmark::printResult(const std::string &trace, size_t value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void SimpleBenchmark::destroy()
{
    double totalTime = mTimer->getElapsedTime();
    double averageTime = 1000.0 * totalTime / static_cast<double>(mNumFrames);

    printResult("total_time", totalTime, "s", true);
    printResult("frames", static_cast<size_t>(mNumFrames), "frames", true);
    printResult("average_time", averageTime, "ms", true);

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