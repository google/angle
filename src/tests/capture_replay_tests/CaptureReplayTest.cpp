//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CaptureReplayTest.cpp:
//   Application that runs replay for testing of capture replay
//

#include "common/system_utils.h"
#include "util/EGLPlatformParameters.h"
#include "util/EGLWindow.h"
#include "util/OSWindow.h"
#include "util/egl_loader_autogen.h"
#include "util/gles_loader_autogen.h"

#include <stdint.h>
#include <string.h>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "util/frame_capture_utils.h"

// Build the right context header based on replay ID
// This will expand to "angle_capture_context<#>.h"
#include ANGLE_MACRO_STRINGIZE(ANGLE_CAPTURE_REPLAY_TEST_HEADER)

// Assign the context numbered functions based on GN arg selecting replay ID
std::function<void()> SetupContextReplay = reinterpret_cast<void (*)()>(
    ANGLE_MACRO_CONCAT(SetupContext,
                       ANGLE_MACRO_CONCAT(ANGLE_CAPTURE_REPLAY_TEST_CONTEXT_ID, Replay)));
std::function<void(int)> ReplayContextFrame = reinterpret_cast<void (*)(int)>(
    ANGLE_MACRO_CONCAT(ReplayContext,
                       ANGLE_MACRO_CONCAT(ANGLE_CAPTURE_REPLAY_TEST_CONTEXT_ID, Frame)));
std::function<void()> ResetContextReplay = reinterpret_cast<void (*)()>(
    ANGLE_MACRO_CONCAT(ResetContext,
                       ANGLE_MACRO_CONCAT(ANGLE_CAPTURE_REPLAY_TEST_CONTEXT_ID, Replay)));

class CaptureReplayTest
{
  public:
    CaptureReplayTest(int width, int height, EGLint glesMajorVersion, EGLint glesMinorVersion)
        : mWidth(width), mHeight(height), mOSWindow(nullptr), mEGLWindow(nullptr)

    {
        mPlatformParams.renderer   = EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE;
        mPlatformParams.deviceType = EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE;

        // Load EGL library so we can initialize the display.
        mEntryPointsLib.reset(
            angle::OpenSharedLibrary(ANGLE_EGL_LIBRARY_NAME, angle::SearchType::ApplicationDir));

        mEGLWindow = EGLWindow::New(glesMajorVersion, glesMinorVersion);
        mOSWindow  = OSWindow::New();
    }

    ~CaptureReplayTest()
    {
        EGLWindow::Delete(&mEGLWindow);
        OSWindow::Delete(&mOSWindow);
    }

    bool initialize()
    {
        // Set CWD to executable directory.
        std::string exeDir = angle::GetExecutableDirectory();
        if (!angle::SetCWD(exeDir.c_str()))
            return false;
        if (kIsBinaryDataCompressed)
        {
            SetBinaryDataDecompressCallback(angle::DecompressBinaryData);
        }
        SetBinaryDataDir(ANGLE_CAPTURE_REPLAY_TEST_DATA_DIR);
        SetupContextReplay();

        return true;
    }

    void draw() { ReplayContextFrame(0); }

    void swap() { mEGLWindow->swap(); }

    int run()
    {
        if (!mOSWindow->initialize("Capture Replay Test", mWidth, mHeight))
        {
            return -1;
        }

        mOSWindow->setVisible(true);

        ConfigParameters configParams;
        configParams.redBits     = 8;
        configParams.greenBits   = 8;
        configParams.blueBits    = 8;
        configParams.alphaBits   = 8;
        configParams.depthBits   = 24;
        configParams.stencilBits = 8;

        if (!mEGLWindow->initializeGL(mOSWindow, mEntryPointsLib.get(),
                                      angle::GLESDriverType::AngleEGL, mPlatformParams,
                                      configParams))
        {
            return -1;
        }

        // Disable vsync
        if (!mEGLWindow->setSwapInterval(0))
        {
            return -1;
        }

        angle::LoadGLES(eglGetProcAddress);

        int result = 0;

        if (!initialize())
        {
            result = -1;
        }

        draw();
        swap();

        mEGLWindow->destroyGL();
        mOSWindow->destroy();

        return result;
    }

  private:
    uint32_t mWidth;
    uint32_t mHeight;
    OSWindow *mOSWindow;
    EGLWindow *mEGLWindow;

    EGLPlatformParameters mPlatformParams;

    // Handle to the entry point binding library.
    std::unique_ptr<angle::Library> mEntryPointsLib;
};

int main(int argc, char **argv)
{
    // TODO (nguyenmh): http://anglebug.com/4759: initialize app with arguments taken from cmdline
    const int width               = 128;
    const int height              = 128;
    const EGLint glesMajorVersion = 2;
    const GLint glesMinorVersion  = 0;
    CaptureReplayTest app(width, height, glesMajorVersion, glesMinorVersion);
    return app.run();
}
