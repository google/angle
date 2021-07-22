//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLProtectedContentTest.cpp:
//   EGL extension EGL_EXT_protected_content
//

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <thread>
#include "test_utils/ANGLETest.h"
#include "util/EGLWindow.h"
#include "util/OSWindow.h"

using namespace std::chrono_literals;

using namespace angle;

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EGLProtectedContentTest);

class EGLProtectedContentTest : public ANGLETest
{
  public:
    EGLProtectedContentTest() : mDisplay(EGL_NO_DISPLAY) {}

    void testSetUp() override
    {
        EGLint dispattrs[] = {EGL_PLATFORM_ANGLE_TYPE_ANGLE, GetParam().getRenderer(), EGL_NONE};
        mDisplay           = eglGetPlatformDisplayEXT(
            EGL_PLATFORM_ANGLE_ANGLE, reinterpret_cast<void *>(EGL_DEFAULT_DISPLAY), dispattrs);
        EXPECT_TRUE(mDisplay != EGL_NO_DISPLAY);
        EXPECT_EGL_TRUE(eglInitialize(mDisplay, nullptr, nullptr));
        mMajorVersion = GetParam().majorVersion;
    }

    void testTearDown() override
    {
        if (mDisplay != EGL_NO_DISPLAY)
        {
            eglTerminate(mDisplay);
            eglReleaseThread();
            mDisplay = EGL_NO_DISPLAY;
        }
        ASSERT_EGL_SUCCESS() << "Error during test TearDown";
    }

    bool chooseConfig(EGLConfig *config)
    {
        EGLint clientVersion = mMajorVersion == 3 ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT;
        EGLint attribs[]     = {EGL_RED_SIZE,
                            8,
                            EGL_GREEN_SIZE,
                            8,
                            EGL_BLUE_SIZE,
                            8,
                            EGL_ALPHA_SIZE,
                            8,
                            EGL_RENDERABLE_TYPE,
                            clientVersion,
                            EGL_SURFACE_TYPE,
                            (EGL_PBUFFER_BIT | EGL_WINDOW_BIT),
                            EGL_NONE};

        EGLint count = 0;
        bool result  = eglChooseConfig(mDisplay, attribs, config, 1, &count);
        EXPECT_EGL_TRUE(result);
        EXPECT_EGL_TRUE(count > 0);
        return result;
    }

    bool createContext(EGLBoolean isProtected, EGLConfig config, EGLContext *context)
    {
        bool result                 = false;
        EGLint attribsProtected[]   = {EGL_CONTEXT_MAJOR_VERSION, mMajorVersion,
                                     EGL_PROTECTED_CONTENT_EXT, EGL_TRUE, EGL_NONE};
        EGLint attribsUnProtected[] = {EGL_CONTEXT_MAJOR_VERSION, mMajorVersion, EGL_NONE};

        *context = eglCreateContext(mDisplay, config, nullptr,
                                    (isProtected ? attribsProtected : attribsUnProtected));
        result   = (*context != EGL_NO_CONTEXT);
        EXPECT_TRUE(result);
        return result;
    }

    bool createPbufferSurface(EGLBoolean isProtected, EGLConfig config, EGLSurface *surface)
    {
        bool result               = false;
        EGLint attribsProtected[] = {
            EGL_WIDTH, kWidth, EGL_HEIGHT, kHeight, EGL_PROTECTED_CONTENT_EXT, EGL_TRUE, EGL_NONE};
        EGLint attribsUnProtected[] = {EGL_WIDTH, kWidth, EGL_HEIGHT, kHeight, EGL_NONE};

        *surface = eglCreatePbufferSurface(mDisplay, config,
                                           (isProtected ? attribsProtected : attribsUnProtected));
        result   = (*surface != EGL_NO_SURFACE);
        EXPECT_TRUE(result);
        return result;
    }

    bool createWindowSurface(EGLBoolean isProtected,
                             EGLConfig config,
                             EGLNativeWindowType win,
                             EGLSurface *surface)
    {
        bool result                 = false;
        EGLint attribsProtected[]   = {EGL_PROTECTED_CONTENT_EXT, EGL_TRUE, EGL_NONE};
        EGLint attribsUnProtected[] = {EGL_NONE};

        *surface = eglCreateWindowSurface(mDisplay, config, win,
                                          (isProtected ? attribsProtected : attribsUnProtected));
        result   = (*surface != EGL_NO_SURFACE);
        EXPECT_TRUE(result);
        return result;
    }

    void PbufferTest(bool isProtectedContext, bool isProtectedSurface);
    void WindowTest(bool isProtectedContext, bool isProtectedSurface);

    EGLDisplay mDisplay         = EGL_NO_DISPLAY;
    EGLint mMajorVersion        = 0;
    static const EGLint kWidth  = 16;
    static const EGLint kHeight = 16;
};

void EGLProtectedContentTest::PbufferTest(bool isProtectedContext, bool isProtectedSurface)
{
    if (isProtectedContext || isProtectedSurface)
    {
        ANGLE_SKIP_TEST_IF(!IsEGLDisplayExtensionEnabled(mDisplay, "EGL_EXT_protected_content"));
    }

    EGLConfig config = EGL_NO_CONFIG_KHR;
    EXPECT_TRUE(chooseConfig(&config));

    EGLContext context = EGL_NO_CONTEXT;
    EXPECT_TRUE(createContext(isProtectedContext, config, &context));
    ASSERT_EGL_SUCCESS() << "eglCreateContext failed.";

    EGLSurface pBufferSurface = EGL_NO_SURFACE;
    EXPECT_TRUE(createPbufferSurface(isProtectedSurface, config, &pBufferSurface));
    ASSERT_EGL_SUCCESS() << "eglCreatePbufferSurface failed.";

    EXPECT_TRUE(eglMakeCurrent(mDisplay, pBufferSurface, pBufferSurface, context));
    ASSERT_EGL_SUCCESS() << "eglMakeCurrent failed.";

    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glFinish();
    ASSERT_GL_NO_ERROR() << "glFinish failed";

    EXPECT_TRUE(eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context));
    ASSERT_EGL_SUCCESS() << "eglMakeCurrent - uncurrent failed.";

    eglDestroySurface(mDisplay, pBufferSurface);
    pBufferSurface = EGL_NO_SURFACE;

    eglDestroyContext(mDisplay, context);
    context = EGL_NO_CONTEXT;
}

// Unprotected context with Unprotected PbufferSurface
TEST_P(EGLProtectedContentTest, UnprotectedContextWithUnprotectedPbufferSurface)
{
    PbufferTest(false, false);
}

void EGLProtectedContentTest::WindowTest(bool isProtectedContext, bool isProtectedSurface)
{
    if (isProtectedContext || isProtectedSurface)
    {
        ANGLE_SKIP_TEST_IF(!IsEGLDisplayExtensionEnabled(mDisplay, "EGL_EXT_protected_content"));
    }

    EGLConfig config = EGL_NO_CONFIG_KHR;
    EXPECT_TRUE(chooseConfig(&config));

    EGLContext context = EGL_NO_CONTEXT;
    EXPECT_TRUE(createContext(isProtectedContext, config, &context));
    ASSERT_EGL_SUCCESS() << "eglCreateContext failed.";

    OSWindow *osWindow = OSWindow::New();
    osWindow->initialize("ProtectedContentTest", kWidth, kHeight);
    EGLSurface windowSurface          = EGL_NO_SURFACE;
    EGLBoolean createWinSurfaceResult = createWindowSurface(
        isProtectedSurface, config, osWindow->getNativeWindow(), &windowSurface);
    EXPECT_TRUE(createWinSurfaceResult);
    ASSERT_EGL_SUCCESS() << "eglCreateWindowSurface failed.";

    EXPECT_TRUE(eglMakeCurrent(mDisplay, windowSurface, windowSurface, context));
    ASSERT_EGL_SUCCESS() << "eglMakeCurrent failed.";

    // Red
    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR() << "glClear failed";
    eglSwapBuffers(mDisplay, windowSurface);
    ASSERT_EGL_SUCCESS() << "eglSwapBuffers failed.";
    std::this_thread::sleep_for(1s);

    // Green
    glClearColor(0.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR() << "glClear failed";
    eglSwapBuffers(mDisplay, windowSurface);
    ASSERT_EGL_SUCCESS() << "eglSwapBuffers failed.";
    std::this_thread::sleep_for(1s);

    // Blue
    glClearColor(0.0, 0.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR() << "glClear failed";
    eglSwapBuffers(mDisplay, windowSurface);
    ASSERT_EGL_SUCCESS() << "eglSwapBuffers failed.";
    std::this_thread::sleep_for(1s);

    EXPECT_TRUE(eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, context));
    ASSERT_EGL_SUCCESS() << "eglMakeCurrent - uncurrent failed.";

    eglDestroySurface(mDisplay, windowSurface);
    windowSurface = EGL_NO_SURFACE;
    osWindow->destroy();
    OSWindow::Delete(&osWindow);

    eglDestroyContext(mDisplay, context);
    context = EGL_NO_CONTEXT;
}

// Unprotected context with Unprotected WindowSurface
TEST_P(EGLProtectedContentTest, UnprotectedContextWithUnprotectedWindowSurface)
{
    std::cout << "Operator should see RED, GREEN, BLUE on screen" << std::endl;
    WindowTest(false, false);
}

// Protected context with Protected WindowSurface
TEST_P(EGLProtectedContentTest, ProtectedContextWithProtectedWindowSurface)
{
    std::cout << "Operator should see RED, GREEN, BLUE on screen" << std::endl;
    WindowTest(true, true);
}

ANGLE_INSTANTIATE_TEST(EGLProtectedContentTest,
                       WithNoFixture(ES2_VULKAN()),
                       WithNoFixture(ES3_VULKAN()));
