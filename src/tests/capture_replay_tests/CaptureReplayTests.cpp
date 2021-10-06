//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CaptureReplayTest.cpp:
//   Application that runs replay for testing of capture replay
//

#include "common/debug.h"
#include "common/system_utils.h"
#include "util/EGLPlatformParameters.h"
#include "util/EGLWindow.h"
#include "util/OSWindow.h"

#include <stdint.h>
#include <string.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "util/frame_capture_test_utils.h"

// Build the right context header based on replay ID
// This will expand to "angle_capture_context<#>.h"
#include ANGLE_MACRO_STRINGIZE(ANGLE_CAPTURE_REPLAY_COMPOSITE_TESTS_HEADER)

constexpr char kResultTag[] = "*RESULT";

class CaptureReplayTests
{
  public:
    CaptureReplayTests()
    {
        // Load EGL library so we can initialize the display.
        mEntryPointsLib.reset(
            angle::OpenSharedLibrary(ANGLE_EGL_LIBRARY_NAME, angle::SearchType::ModuleDir));

        mOSWindow = OSWindow::New();
        mOSWindow->disableErrorMessageDialog();
    }

    ~CaptureReplayTests()
    {
        EGLWindow::Delete(&mEGLWindow);
        OSWindow::Delete(&mOSWindow);
    }

    bool initializeTest(uint32_t testIndex, const TestTraceInfo &testTraceInfo)
    {
        if (!mOSWindow->initialize(testTraceInfo.testName, testTraceInfo.replayDrawSurfaceWidth,
                                   testTraceInfo.replayDrawSurfaceHeight))
        {
            return false;
        }

        mOSWindow->disableErrorMessageDialog();
        mOSWindow->setVisible(true);

        if (mEGLWindow && !mEGLWindow->isContextVersion(testTraceInfo.replayContextMajorVersion,
                                                        testTraceInfo.replayContextMinorVersion))
        {
            EGLWindow::Delete(&mEGLWindow);
            mEGLWindow = nullptr;
        }

        if (!mEGLWindow)
        {
            mEGLWindow = EGLWindow::New(testTraceInfo.replayContextMajorVersion,
                                        testTraceInfo.replayContextMinorVersion);
        }

        ConfigParameters configParams;
        configParams.redBits     = testTraceInfo.defaultFramebufferRedBits;
        configParams.greenBits   = testTraceInfo.defaultFramebufferGreenBits;
        configParams.blueBits    = testTraceInfo.defaultFramebufferBlueBits;
        configParams.alphaBits   = testTraceInfo.defaultFramebufferAlphaBits;
        configParams.depthBits   = testTraceInfo.defaultFramebufferDepthBits;
        configParams.stencilBits = testTraceInfo.defaultFramebufferStencilBits;

        configParams.clientArraysEnabled   = testTraceInfo.areClientArraysEnabled;
        configParams.bindGeneratesResource = testTraceInfo.bindGeneratesResources;
        configParams.webGLCompatibility    = testTraceInfo.webGLCompatibility;
        configParams.robustResourceInit    = testTraceInfo.robustResourceInit;

        mPlatformParams.renderer   = testTraceInfo.replayPlatformType;
        mPlatformParams.deviceType = testTraceInfo.replayDeviceType;

        if (!mEGLWindow->initializeGL(mOSWindow, mEntryPointsLib.get(),
                                      angle::GLESDriverType::AngleEGL, mPlatformParams,
                                      configParams))
        {
            mOSWindow->destroy();
            return false;
        }
        // Disable vsync
        if (!mEGLWindow->setSwapInterval(0))
        {
            cleanupTest();
            return false;
        }

        mStartingDirectory = angle::GetCWD().value();

        // Load trace
        mTraceLibrary.reset(new angle::TraceLibrary(testTraceInfo.testName.c_str()));

        // Set CWD to executable directory.
        std::string exeDir = angle::GetExecutableDirectory();
        if (!angle::SetCWD(exeDir.c_str()))
        {
            cleanupTest();
            return false;
        }
        if (testTraceInfo.isBinaryDataCompressed)
        {
            mTraceLibrary->setBinaryDataDecompressCallback(angle::DecompressBinaryData);
        }
        mTraceLibrary->setBinaryDataDir(ANGLE_CAPTURE_REPLAY_TEST_DATA_DIR);

        mTraceLibrary->setupReplay();
        return true;
    }

    void cleanupTest()
    {
        angle::SetCWD(mStartingDirectory.c_str());
        mTraceLibrary.reset(nullptr);
        mEGLWindow->destroyGL();
        mOSWindow->destroy();
    }

    void swap() { mEGLWindow->swap(); }

    int runTest(uint32_t testIndex, const TestTraceInfo &testTraceInfo)
    {
        if (!initializeTest(testIndex, testTraceInfo))
        {
            return -1;
        }

        for (uint32_t frame = testTraceInfo.replayFrameStart; frame <= testTraceInfo.replayFrameEnd;
             frame++)
        {
            mTraceLibrary->replayFrame(frame);

            const char *capturedSerializedState =
                reinterpret_cast<const char *>(glGetString(GL_SERIALIZED_CONTEXT_STRING_ANGLE));
            const char *replayedSerializedState = mTraceLibrary->getSerializedContextState(frame);

            bool isEqual =
                (capturedSerializedState && replayedSerializedState)
                    ? compareSerializedContexts(replayedSerializedState, capturedSerializedState)
                    : (capturedSerializedState == replayedSerializedState);

            // Swap always to allow RenderDoc/other tools to capture frames.
            swap();
            if (!isEqual)
            {
                std::ostringstream replayName;
                replayName << testTraceInfo.testName << "_ContextReplayed" << frame << ".json";
                std::ofstream debugReplay(replayName.str());
                debugReplay << (replayedSerializedState ? replayedSerializedState : "") << "\n";

                std::ostringstream captureName;
                captureName << testTraceInfo.testName << "_ContextCaptured" << frame << ".json";
                std::ofstream debugCapture(captureName.str());

                debugCapture << (capturedSerializedState ? capturedSerializedState : "") << "\n";

                cleanupTest();
                return -1;
            }
        }
        cleanupTest();
        return 0;
    }

    int run()
    {
        for (size_t i = 0; i < testTraceInfos.size(); i++)
        {
            int result = runTest(static_cast<uint32_t>(i), testTraceInfos[i]);
            std::cout << kResultTag << " " << testTraceInfos[i].testName << " " << result << "\n";
        }
        return 0;
    }

  private:
    bool compareSerializedContexts(const char *capturedSerializedContextState,
                                   const char *replaySerializedContextState)
    {

        return !strcmp(replaySerializedContextState, capturedSerializedContextState);
    }

    std::string mStartingDirectory;
    OSWindow *mOSWindow   = nullptr;
    EGLWindow *mEGLWindow = nullptr;
    EGLPlatformParameters mPlatformParams;
    // Handle to the entry point binding library.
    std::unique_ptr<angle::Library> mEntryPointsLib;
    std::unique_ptr<angle::TraceLibrary> mTraceLibrary;
};

int main(int argc, char **argv)
{
    CaptureReplayTests app;
    return app.run();
}
