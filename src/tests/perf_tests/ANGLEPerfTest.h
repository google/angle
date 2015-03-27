//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ANGLEPerfTests:
//   Base class for google test performance tests
//

#ifndef PERF_TESTS_ANGLE_PERF_TEST_H_
#define PERF_TESTS_ANGLE_PERF_TEST_H_

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string>

#include "EGLWindow.h"
#include "OSWindow.h"
#include "Timer.h"
#include "shared_utils.h"

class Event;

class ANGLEPerfTest : public testing::Test
{
  public:
    ANGLEPerfTest(const std::string &name, const std::string &suffix);
    virtual ~ANGLEPerfTest() { };

    virtual void step(float dt, double totalTime) = 0;

    DISALLOW_COPY_AND_ASSIGN(ANGLEPerfTest);

  protected:
    void run();
    void printResult(const std::string &trace, double value, const std::string &units, bool important) const;
    void printResult(const std::string &trace, size_t value, const std::string &units, bool important) const;
    void SetUp() override;
    void TearDown() override;

    std::string mName;
    std::string mSuffix;

    bool mRunning;
    std::unique_ptr<Timer> mTimer;
    int mNumFrames;
};

struct RenderTestParams
{
    virtual std::string suffix() const;

    EGLint requestedRenderer;
    EGLint deviceType;
    EGLint glesMajorVersion;
    EGLint widowWidth;
    EGLint windowHeight;
};

class ANGLERenderTest : public ANGLEPerfTest
{
  public:
    ANGLERenderTest(const std::string &name, const RenderTestParams &testParams);

    virtual bool initializeBenchmark() { return true; }
    virtual void destroyBenchmark() { }

    virtual void stepBenchmark(float dt, double totalTime) { }

    virtual void beginDrawBenchmark() { }
    virtual void drawBenchmark() = 0;
    virtual void endDrawBenchmark() { }

    bool popEvent(Event *event);

    OSWindow *getWindow();

  protected:
    const RenderTestParams &mTestParams;
    unsigned int mDrawIterations;
    double mRunTimeSeconds;

  private:
    DISALLOW_COPY_AND_ASSIGN(ANGLERenderTest);

    void SetUp() override;
    void TearDown() override;

    void step(float dt, double totalTime) override;
    void draw();

    std::unique_ptr<EGLWindow> mEGLWindow;
    std::unique_ptr<OSWindow> mOSWindow;
};

#endif // PERF_TESTS_ANGLE_PERF_TEST_H_
