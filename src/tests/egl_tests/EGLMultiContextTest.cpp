//
// Copyright 2016-2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLMultiContextTest.cpp:
//   Tests relating to multiple non-shared Contexts.

#include <gtest/gtest.h>

#include "EGLMultiThreadSteps.h"
#include "test_utils/ANGLETest.h"
#include "test_utils/angle_test_configs.h"
#include "test_utils/gl_raii.h"
#include "util/EGLWindow.h"

using namespace angle;

namespace
{

EGLBoolean SafeDestroyContext(EGLDisplay display, EGLContext &context)
{
    EGLBoolean result = EGL_TRUE;
    if (context != EGL_NO_CONTEXT)
    {
        result  = eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;
    }
    return result;
}

class EGLMultiContextTest : public ANGLETest
{
  public:
    EGLMultiContextTest() : mContexts{EGL_NO_CONTEXT, EGL_NO_CONTEXT}, mTexture(0) {}

    void testTearDown() override
    {
        glDeleteTextures(1, &mTexture);

        EGLDisplay display = getEGLWindow()->getDisplay();

        if (display != EGL_NO_DISPLAY)
        {
            for (auto &context : mContexts)
            {
                SafeDestroyContext(display, context);
            }
        }

        // Set default test state to not give an error on shutdown.
        getEGLWindow()->makeCurrent();
    }

    EGLContext mContexts[2];
    GLuint mTexture;
};

// Test that a compute shader running in one thread will still work when rendering is happening in
// another thread (with non-shared contexts).  The non-shared context will still share a Vulkan
// command buffer.
TEST_P(EGLMultiContextTest, ComputeShaderOkayWithRendering)
{
    ANGLE_SKIP_TEST_IF(!platformSupportsMultithreading());
    ANGLE_SKIP_TEST_IF(!isVulkanRenderer());
    ANGLE_SKIP_TEST_IF(getClientMajorVersion() < 3 || getClientMinorVersion() < 1);

    // Initialize contexts
    EGLWindow *window = getEGLWindow();
    EGLDisplay dpy    = window->getDisplay();
    EGLConfig config  = window->getConfig();

    constexpr size_t kThreadCount    = 2;
    EGLSurface surface[kThreadCount] = {EGL_NO_SURFACE, EGL_NO_SURFACE};
    EGLContext ctx[kThreadCount]     = {EGL_NO_CONTEXT, EGL_NO_CONTEXT};

    EGLint pbufferAttributes[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE, EGL_NONE};

    for (size_t t = 0; t < kThreadCount; ++t)
    {
        surface[t] = eglCreatePbufferSurface(dpy, config, pbufferAttributes);
        EXPECT_EGL_SUCCESS();

        ctx[t] = window->createContext(EGL_NO_CONTEXT);
        EXPECT_NE(EGL_NO_CONTEXT, ctx[t]);
    }

    // Synchronization tools to ensure the two threads are interleaved as designed by this test.
    std::mutex mutex;
    std::condition_variable condVar;

    enum class Step
    {
        Thread0Start,
        Thread0DispatchedCompute,
        Thread1Drew,
        Thread0DispatchedComputeAgain,
        Finish,
        Abort,
    };
    Step currentStep = Step::Thread0Start;

    // This first thread dispatches a compute shader.  It immediately starts.
    std::thread deletingThread = std::thread([&]() {
        ThreadSynchronization<Step> threadSynchronization(&currentStep, &mutex, &condVar);

        EXPECT_EGL_TRUE(eglMakeCurrent(dpy, surface[0], surface[0], ctx[0]));
        EXPECT_EGL_SUCCESS();

        // Potentially wait to be signalled to start.
        ASSERT_TRUE(threadSynchronization.waitForStep(Step::Thread0Start));

        // Wake up and do next step: Create, detach, and dispatch a compute shader program.
        constexpr char kCS[]  = R"(#version 310 es
layout(local_size_x=1) in;
void main()
{
})";
        GLuint computeProgram = glCreateProgram();
        GLuint cs             = CompileShader(GL_COMPUTE_SHADER, kCS);
        EXPECT_NE(0u, cs);

        glAttachShader(computeProgram, cs);
        glDeleteShader(cs);
        glLinkProgram(computeProgram);
        GLint linkStatus;
        glGetProgramiv(computeProgram, GL_LINK_STATUS, &linkStatus);
        EXPECT_GL_TRUE(linkStatus);
        glDetachShader(computeProgram, cs);
        EXPECT_GL_NO_ERROR();
        glUseProgram(computeProgram);

        glDispatchCompute(8, 4, 2);
        EXPECT_GL_NO_ERROR();

        // Signal the second thread and wait for it to draw and flush.
        threadSynchronization.nextStep(Step::Thread0DispatchedCompute);
        ASSERT_TRUE(threadSynchronization.waitForStep(Step::Thread1Drew));

        // Wake up and do next step: Dispatch the same compute shader again.
        glDispatchCompute(8, 4, 2);

        // Signal the second thread and wait for it to draw and flush again.
        threadSynchronization.nextStep(Step::Thread0DispatchedComputeAgain);
        ASSERT_TRUE(threadSynchronization.waitForStep(Step::Finish));

        // Wake up and do next step: Dispatch the same compute shader again, and force flush the
        // underlying command buffer.
        glDispatchCompute(8, 4, 2);
        glFinish();

        // Clean-up and exit this thread.
        EXPECT_GL_NO_ERROR();
        EXPECT_EGL_TRUE(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
        EXPECT_EGL_SUCCESS();
    });

    // This second thread renders.  It starts once the other thread does its first nextStep()
    std::thread continuingThread = std::thread([&]() {
        ThreadSynchronization<Step> threadSynchronization(&currentStep, &mutex, &condVar);

        EXPECT_EGL_TRUE(eglMakeCurrent(dpy, surface[1], surface[1], ctx[1]));
        EXPECT_EGL_SUCCESS();

        // Wait for first thread to create and dispatch a compute shader.
        ASSERT_TRUE(threadSynchronization.waitForStep(Step::Thread0DispatchedCompute));

        // Wake up and do next step: Create graphics resources, draw, and force flush the
        // underlying command buffer.
        GLTexture texture;
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        GLRenderbuffer renderbuffer;
        GLFramebuffer fbo;
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        constexpr int kRenderbufferSize = 4;
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, kRenderbufferSize, kRenderbufferSize);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                  renderbuffer);
        glBindTexture(GL_TEXTURE_2D, texture);

        GLProgram graphicsProgram;
        graphicsProgram.makeRaster(essl1_shaders::vs::Texture2D(), essl1_shaders::fs::Texture2D());
        ASSERT_TRUE(graphicsProgram.valid());

        drawQuad(graphicsProgram.get(), essl1_shaders::PositionAttrib(), 0.5f);
        glFinish();

        // Signal the first thread and wait for it to dispatch a compute shader again.
        threadSynchronization.nextStep(Step::Thread1Drew);
        ASSERT_TRUE(threadSynchronization.waitForStep(Step::Thread0DispatchedComputeAgain));

        // Wake up and do next step: Draw and force flush the underlying command buffer again.
        drawQuad(graphicsProgram.get(), essl1_shaders::PositionAttrib(), 0.5f);
        glFinish();

        // Signal the first thread and wait exit this thread.
        threadSynchronization.nextStep(Step::Finish);

        EXPECT_EGL_TRUE(eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
        EXPECT_EGL_SUCCESS();
    });

    deletingThread.join();
    continuingThread.join();

    ASSERT_NE(currentStep, Step::Abort);

    // Clean up
    for (size_t t = 0; t < kThreadCount; ++t)
    {
        eglDestroySurface(dpy, surface[t]);
        eglDestroyContext(dpy, ctx[t]);
    }
}
}  // anonymous namespace

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EGLMultiContextTest);
ANGLE_INSTANTIATE_TEST_ES31(EGLMultiContextTest);
