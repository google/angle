//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLInitializePerfTest:
//   Performance test for device creation.
//

#include "ANGLEPerfTest.h"
#include "test_utils/angle_test_configs.h"
#include "test_utils/angle_test_instantiate.h"

using namespace testing;

namespace
{

class EGLInitializePerfTest : public ANGLEPerfTest,
                              public WithParamInterface<angle::PlatformParameters>
{
  public:
    EGLInitializePerfTest();
    ~EGLInitializePerfTest();

    void step(float dt, double totalTime) override;

  private:
    OSWindow *mOSWindow;
    EGLDisplay mDisplay;
};

EGLInitializePerfTest::EGLInitializePerfTest()
    : ANGLEPerfTest("EGLInitialize", "_run"),
      mOSWindow(nullptr),
      mDisplay(EGL_NO_DISPLAY)
{
    auto platform = GetParam().mEGLPlatformParameters;

    std::vector<EGLint> displayAttributes;
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    displayAttributes.push_back(platform.renderer);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE);
    displayAttributes.push_back(platform.majorVersion);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE);
    displayAttributes.push_back(platform.minorVersion);

    if (platform.renderer == EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE ||
        platform.renderer == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
    {
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
        displayAttributes.push_back(platform.deviceType);
    }
    displayAttributes.push_back(EGL_NONE);

    mOSWindow = CreateOSWindow();
    mOSWindow->initialize("EGLInitialize Test", 64, 64);

    auto eglGetPlatformDisplayEXT =
        reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (eglGetPlatformDisplayEXT == nullptr)
    {
        std::cerr << "Error getting platform display!" << std::endl;
        return;
    }

    mDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                        mOSWindow->getNativeDisplay(),
                                        &displayAttributes[0]);
}

EGLInitializePerfTest::~EGLInitializePerfTest()
{
    SafeDelete(mOSWindow);
}

void EGLInitializePerfTest::step(float dt, double totalTime)
{
    ASSERT_TRUE(mDisplay != EGL_NO_DISPLAY);

    EGLint majorVersion, minorVersion;
    ASSERT_TRUE(eglInitialize(mDisplay, &majorVersion, &minorVersion) == EGL_TRUE);
    ASSERT_TRUE(eglTerminate(mDisplay) == EGL_TRUE);

    if (mTimer->getElapsedTime() >= 5.0)
    {
        mRunning = false;
    }
}

TEST_P(EGLInitializePerfTest, Run)
{
    run();
}

ANGLE_INSTANTIATE_TEST(EGLInitializePerfTest, angle::ES2_D3D11());

} // namespace
