//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "ANGLEPerfTest.h"

#include "third_party/perf/perf_test.h"

#include <iostream>
#include <cassert>

std::string PerfTestParams::suffix() const
{
    switch (requestedRenderer)
    {
        case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE: return "_d3d11";
        case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE: return "_d3d9";
        default: assert(0); return "_unk";
    }
}

ANGLEPerfTest::ANGLEPerfTest(const std::string &name, const PerfTestParams &testParams)
    : mTestParams(testParams),
      mNumFrames(0),
      mName(name),
      mRunning(false),
      mSuffix(testParams.suffix()),
      mDrawIterations(10),
      mRunTimeSeconds(5.0)
{
}

void ANGLEPerfTest::SetUp()
{
    EGLPlatformParameters platformParams(mTestParams.requestedRenderer,
                                         EGL_DONT_CARE,
                                         EGL_DONT_CARE,
                                         mTestParams.deviceType);

    mOSWindow.reset(CreateOSWindow());
    mEGLWindow.reset(new EGLWindow(mTestParams.widowWidth,
                                   mTestParams.windowHeight,
                                   mTestParams.glesMajorVersion,
                                   platformParams));
    mTimer.reset(CreateTimer());

    if (!mOSWindow->initialize(mName, mEGLWindow->getWidth(), mEGLWindow->getHeight()))
    {
        FAIL() << "Failed initializing OSWindow";
        return;
    }

    if (!mEGLWindow->initializeGL(mOSWindow.get()))
    {
        FAIL() << "Failed initializing EGLWindow";
        return;
    }

    if (!initializeBenchmark())
    {
        FAIL() << "Failed initializing base perf test";
        return;
    }

    mRunning = true;
}

void ANGLEPerfTest::printResult(const std::string &trace, double value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void ANGLEPerfTest::printResult(const std::string &trace, size_t value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void ANGLEPerfTest::TearDown()
{
    double totalTime = mTimer->getElapsedTime();
    double averageTime = 1000.0 * totalTime / static_cast<double>(mNumFrames);

    printResult("total_time", totalTime, "s", true);
    printResult("frames", static_cast<size_t>(mNumFrames), "frames", true);
    printResult("average_time", averageTime, "ms", true);

    destroyBenchmark();

    mEGLWindow->destroyGL();
    mOSWindow->destroy();
}

void ANGLEPerfTest::step(float dt, double totalTime)
{
    stepBenchmark(dt, totalTime);
}

void ANGLEPerfTest::draw()
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

void ANGLEPerfTest::run()
{
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
}

bool ANGLEPerfTest::popEvent(Event *event)
{
    return mOSWindow->popEvent(event);
}

OSWindow *ANGLEPerfTest::getWindow()
{
    return mOSWindow.get();
}
