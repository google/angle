//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLThreadTest.h: Tests multi-threaded uses of EGL.

#include "gtest/gtest.h"
#include "system_utils.h"

#include <thread>

#include <EGL/egl.h>
#include <EGL/eglext.h>

class EGLThreadTest : public testing::Test
{
  public:
    void threadingTest();

  protected:
    EGLDisplay mDisplay = EGL_NO_DISPLAY;
};

void EGLThreadTest::threadingTest()
{
    mDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EXPECT_TRUE(mDisplay != EGL_NO_DISPLAY);

    eglInitialize(mDisplay, nullptr, nullptr);
    eglGetCurrentContext();
}

// Test a bug in our EGL TLS implementation.
TEST_F(EGLThreadTest, ThreadInitCrash)
{
    std::thread runner(&EGLThreadTest::threadingTest, this);

    // wait for signal from thread
    runner.join();

    // crash, because the TLS value is NULL on main thread
    eglGetCurrentSurface(EGL_DRAW);
    eglGetCurrentContext();

    eglTerminate(mDisplay);
}
