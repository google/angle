//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "ANGLEPerfTest.h"

#include "third_party/perf/perf_test.h"

#include <iostream>
#include <cassert>

ANGLEPerfTest::ANGLEPerfTest(const std::string &name, const std::string &suffix)
    : mName(name),
      mSuffix(suffix),
      mRunning(false),
      mNumFrames(0)
{
    mTimer.reset(CreateTimer());
}

void ANGLEPerfTest::run()
{
    mTimer->start();
    double prevTime = 0.0;

    while (mRunning)
    {
        double elapsedTime = mTimer->getElapsedTime();
        double deltaTime = elapsedTime - prevTime;

        ++mNumFrames;
        step(static_cast<float>(deltaTime), elapsedTime);

        if (!mRunning)
        {
            break;
        }

        prevTime = elapsedTime;
    }
}

void ANGLEPerfTest::printResult(const std::string &trace, double value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void ANGLEPerfTest::printResult(const std::string &trace, size_t value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void ANGLEPerfTest::SetUp()
{
    mRunning = true;
}

void ANGLEPerfTest::TearDown()
{
    double totalTime = mTimer->getElapsedTime();
    double averageTime = 1000.0 * totalTime / static_cast<double>(mNumFrames);

    printResult("total_time", totalTime, "s", true);
    printResult("frames", static_cast<size_t>(mNumFrames), "frames", true);
    printResult("average_time", averageTime, "ms", true);
}

std::string RenderTestParams::suffix() const
{
    switch (requestedRenderer)
    {
        case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE: return "_d3d11";
        case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE: return "_d3d9";
        default: assert(0); return "_unk";
    }
}

ANGLERenderTest::ANGLERenderTest(const std::string &name, const RenderTestParams &testParams)
    : ANGLEPerfTest(name, testParams.suffix()),
      mTestParams(testParams),
      mDrawIterations(10),
      mRunTimeSeconds(5.0)
{
}

void ANGLERenderTest::SetUp()
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

    ANGLEPerfTest::SetUp();
}

void ANGLERenderTest::TearDown()
{
    ANGLEPerfTest::TearDown();

    destroyBenchmark();

    mEGLWindow->destroyGL();
    mOSWindow->destroy();
}

void ANGLERenderTest::step(float dt, double totalTime)
{
    stepBenchmark(dt, totalTime);

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

    if (mRunning)
    {
        draw();
        mEGLWindow->swap();
        mOSWindow->messageLoop();
    }
}

void ANGLERenderTest::draw()
{
    if (mTimer->getElapsedTime() > mRunTimeSeconds)
    {
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

bool ANGLERenderTest::popEvent(Event *event)
{
    return mOSWindow->popEvent(event);
}

OSWindow *ANGLERenderTest::getWindow()
{
    return mOSWindow.get();
}
