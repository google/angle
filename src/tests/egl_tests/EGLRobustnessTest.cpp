//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// EGLRobustnessTest.cpp: tests for EGL_EXT_create_context_robustness
//
// Tests causing GPU resets are disabled, use the following args to run them:
// --gtest_also_run_disabled_tests --gtest_filter=EGLRobustnessTest\*

#include <gtest/gtest.h>

#include "common/debug.h"
#include "test_utils/ANGLETest.h"
#include "util/OSWindow.h"

using namespace angle;

class EGLRobustnessTest : public ANGLETest<>
{
  public:
    void testSetUp() override
    {
        mOSWindow = OSWindow::New();
        mOSWindow->initialize("EGLRobustnessTest", 500, 500);
        setWindowVisible(mOSWindow, true);

        const auto &platform = GetParam().eglParameters;

        std::vector<EGLint> displayAttributes;
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
        displayAttributes.push_back(platform.renderer);
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE);
        displayAttributes.push_back(platform.majorVersion);
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE);
        displayAttributes.push_back(platform.minorVersion);

        if (platform.deviceType != EGL_DONT_CARE)
        {
            displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
            displayAttributes.push_back(platform.deviceType);
        }

        displayAttributes.push_back(EGL_NONE);

        mDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE,
                                            reinterpret_cast<void *>(mOSWindow->getNativeDisplay()),
                                            &displayAttributes[0]);
        ASSERT_NE(EGL_NO_DISPLAY, mDisplay);

        ASSERT_TRUE(eglInitialize(mDisplay, nullptr, nullptr) == EGL_TRUE);

        const char *extensions = eglQueryString(mDisplay, EGL_EXTENSIONS);
        if (strstr(extensions, "EGL_EXT_create_context_robustness") == nullptr)
        {
            std::cout << "Test skipped due to missing EGL_EXT_create_context_robustness"
                      << std::endl;
            return;
        }

        int nConfigs = 0;
        ASSERT_TRUE(eglGetConfigs(mDisplay, nullptr, 0, &nConfigs) == EGL_TRUE);
        ASSERT_LE(1, nConfigs);

        std::vector<EGLConfig> allConfigs(nConfigs);
        int nReturnedConfigs = 0;
        ASSERT_TRUE(eglGetConfigs(mDisplay, allConfigs.data(), nConfigs, &nReturnedConfigs) ==
                    EGL_TRUE);
        ASSERT_EQ(nConfigs, nReturnedConfigs);

        for (const EGLConfig &config : allConfigs)
        {
            EGLint surfaceType;
            eglGetConfigAttrib(mDisplay, config, EGL_SURFACE_TYPE, &surfaceType);

            if ((surfaceType & EGL_WINDOW_BIT) != 0)
            {
                mConfig      = config;
                mInitialized = true;
                break;
            }
        }

        if (mInitialized)
        {
            mWindow =
                eglCreateWindowSurface(mDisplay, mConfig, mOSWindow->getNativeWindow(), nullptr);
            ASSERT_EGL_SUCCESS();
        }
    }

    void testTearDown() override
    {
        eglDestroySurface(mDisplay, mWindow);
        eglDestroyContext(mDisplay, mContext);
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(mDisplay);
        EXPECT_EGL_SUCCESS();

        OSWindow::Delete(&mOSWindow);
    }

    void createContext(EGLint resetStrategy)
    {
        const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2,
                                         EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT,
                                         resetStrategy, EGL_NONE};
        mContext = eglCreateContext(mDisplay, mConfig, EGL_NO_CONTEXT, contextAttribs);
        ASSERT_NE(EGL_NO_CONTEXT, mContext);

        eglMakeCurrent(mDisplay, mWindow, mWindow, mContext);
        ASSERT_EGL_SUCCESS();

        const char *extensionString = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
        ASSERT_NE(nullptr, strstr(extensionString, "GL_ANGLE_instanced_arrays"));
    }

    void createRobustContext(bool loseContextOnReset)
    {
        const EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION,
            3,
            EGL_CONTEXT_MINOR_VERSION_KHR,
            0,
            EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT,
            EGL_TRUE,
            EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT,
            loseContextOnReset ? EGL_LOSE_CONTEXT_ON_RESET : EGL_NO_RESET_NOTIFICATION_EXT,
            EGL_NONE};

        mContext = eglCreateContext(mDisplay, mConfig, EGL_NO_CONTEXT, contextAttribs);
        ASSERT_NE(EGL_NO_CONTEXT, mContext);

        eglMakeCurrent(mDisplay, mWindow, mWindow, mContext);
        ASSERT_EGL_SUCCESS();
    }

    void forceContextReset()
    {
        // Cause a GPU reset by drawing 100,000,000 fullscreen quads
        GLuint program = CompileProgram(
            "attribute vec4 pos;\n"
            "void main() {gl_Position = pos;}\n",
            "precision mediump float;\n"
            "void main() {gl_FragColor = vec4(1.0);}\n");
        ASSERT_NE(0u, program);
        glUseProgram(program);

        GLfloat vertices[] = {
            -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f,  -1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 1.0f,
        };

        const int kNumQuads = 10000;
        std::vector<GLushort> indices(6 * kNumQuads);

        for (size_t i = 0; i < kNumQuads; i++)
        {
            indices[i * 6 + 0] = 0;
            indices[i * 6 + 1] = 1;
            indices[i * 6 + 2] = 2;
            indices[i * 6 + 3] = 1;
            indices[i * 6 + 4] = 2;
            indices[i * 6 + 5] = 3;
        }

        glBindAttribLocation(program, 0, "pos");
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(0);

        glViewport(0, 0, mOSWindow->getWidth(), mOSWindow->getHeight());
        glClearColor(1.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawElementsInstancedANGLE(GL_TRIANGLES, kNumQuads * 6, GL_UNSIGNED_SHORT, indices.data(),
                                     10000);

        glFinish();
    }

    void invalidShaderLocalVariableAccess()
    {
        constexpr char kVS[] = R"(#version 310 es
            in highp vec4 a_position;
            void main(void) {
                gl_Position = a_position;
            })";

        constexpr char kFS[] = R"(#version 310 es
            layout(location = 0) out highp vec4 fragColor;
            uniform highp int u_index;
            layout(std140, binding = 0) uniform Block
            {
                highp float color_out[4];
            } ub_in[3];

            void main (void)
            {
                highp vec4 color = vec4(0.0f);
                color[u_index] = ub_in[0].color_out[0];
                fragColor = color;
            })";

        GLuint program = CompileProgram(kVS, kFS);
        ASSERT_NE(0u, program);
        glUseProgram(program);
        GLint indexLocation = glGetUniformLocation(program, "u_index");
        ASSERT_NE(-1, indexLocation);

        // delibrately pass in -1 to u_index to test robustness extension protects write out of
        // bound
        GLint index = -1;
        glUniform1i(indexLocation, index);
        EXPECT_GL_NO_ERROR();

        const GLfloat coords[] = {-1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f};

        GLint coordLocation = glGetAttribLocation(program, "a_position");
        ASSERT_NE(-1, coordLocation);
        GLuint coordBuffer;
        glGenBuffers(1, &coordBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, coordBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(coordLocation);
        glVertexAttribPointer(coordLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        EXPECT_GL_NO_ERROR();

        std::vector<angle::Vector4> refValues(3, angle::Vector4(1.0f, 1.0f, 1.0f, 1.0f));
        std::vector<GLuint> buffers(3, 0);
        glGenBuffers(3, &buffers[0]);

        for (int bufNdx = 0; bufNdx < 3; ++bufNdx)
        {
            glBindBuffer(GL_UNIFORM_BUFFER, buffers[bufNdx]);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(angle::Vector4), &(refValues[bufNdx]),
                         GL_STATIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, bufNdx, buffers[bufNdx]);
            EXPECT_GL_NO_ERROR();
        }

        glDeleteProgram(program);

        GLuint indices[] = {0, 1, 2, 2, 3, 0};
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);
        EXPECT_GL_NO_ERROR();

        glDisableVertexAttribArray(coordLocation);
        coordLocation = 0;
        glDeleteBuffers(1, &coordBuffer);

        glDeleteBuffers(3, &buffers[0]);
        buffers.clear();

        glUseProgram(0);

        // When command buffers are submitted to GPU, if robustness is working properly, the
        // fragment shader will not suffer from write out-of-bounds issue, which resulted in context
        // reset and context loss.
        glFinish();

        EXPECT_GL_NO_ERROR();
    }

  protected:
    EGLDisplay mDisplay = EGL_NO_DISPLAY;
    EGLSurface mWindow  = EGL_NO_SURFACE;
    bool mInitialized   = false;

  private:
    EGLContext mContext = EGL_NO_CONTEXT;
    EGLConfig mConfig   = 0;
    OSWindow *mOSWindow = nullptr;
};

class EGLRobustnessTestES31 : public EGLRobustnessTest
{};

// Check glGetGraphicsResetStatusEXT returns GL_NO_ERROR if we did nothing
TEST_P(EGLRobustnessTest, NoErrorByDefault)
{
    ANGLE_SKIP_TEST_IF(!mInitialized);
    ASSERT_TRUE(glGetGraphicsResetStatusEXT() == GL_NO_ERROR);
}

// Checks that the application gets no loss with NO_RESET_NOTIFICATION
TEST_P(EGLRobustnessTest, DISABLED_NoResetNotification)
{
    ANGLE_SKIP_TEST_IF(!mInitialized);
    createContext(EGL_NO_RESET_NOTIFICATION_EXT);

    if (!IsWindows())
    {
        std::cout << "Test disabled on non Windows platforms because drivers can't recover. "
                  << "See " << __FILE__ << ":" << __LINE__ << std::endl;
        return;
    }
    std::cout << "Causing a GPU reset, brace for impact." << std::endl;

    forceContextReset();
    ASSERT_TRUE(glGetGraphicsResetStatusEXT() == GL_NO_ERROR);
}

// Checks that resetting the ANGLE display allows to get rid of the context loss.
// Also checks that the application gets notified of the loss of the display.
// We coalesce both tests to reduce the number of TDRs done on Windows: by default
// having more than 5 TDRs in a minute will cause Windows to disable the GPU until
// the computer is rebooted.
TEST_P(EGLRobustnessTest, DISABLED_ResettingDisplayWorks)
{
    // Note that on Windows the OpenGL driver fails hard (popup that closes the application)
    // on a TDR caused by D3D. Don't run D3D tests at the same time as the OpenGL tests.
    ANGLE_SKIP_TEST_IF(IsWindows() && isGLRenderer());
    ANGLE_SKIP_TEST_IF(!mInitialized);

    createContext(EGL_LOSE_CONTEXT_ON_RESET_EXT);

    if (!IsWindows())
    {
        std::cout << "Test disabled on non Windows platforms because drivers can't recover. "
                  << "See " << __FILE__ << ":" << __LINE__ << std::endl;
        return;
    }
    std::cout << "Causing a GPU reset, brace for impact." << std::endl;

    forceContextReset();
    ASSERT_TRUE(glGetGraphicsResetStatusEXT() != GL_NO_ERROR);

    recreateTestFixture();
    ASSERT_TRUE(glGetGraphicsResetStatusEXT() == GL_NO_ERROR);
}

// Test to reproduce the crash when running
// dEQP-EGL.functional.robustness.reset_context.shaders.out_of_bounds.reset_status.writes.uniform_block.fragment
// on Pixel 6
// The test requires ES3.1 conformant in order to compile shader successfully.
// Limit the test to run on Vulkan backend only, as other backend doesn't support 3.1 yet.
TEST_P(EGLRobustnessTestES31, ContextResetOnInvalidLocalShaderVariableAccess)
{
    ANGLE_SKIP_TEST_IF(!mInitialized);

    ANGLE_SKIP_TEST_IF(
        !IsEGLDisplayExtensionEnabled(mDisplay, "EGL_KHR_create_context") ||
        !IsEGLDisplayExtensionEnabled(mDisplay, "EGL_EXT_create_context_robustness"));

    createRobustContext(true);
    invalidShaderLocalVariableAccess();
}

// Test to ensure shader local variable write out of bound won't crash
// when the context has robustness enabled, and EGL_NO_RESET_NOTIFICATION_EXT
// is set as the value for attribute EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEFY_EXT
TEST_P(EGLRobustnessTestES31, ContextNoResetOnInvalidLocalShaderVariableAccess)
{
    ANGLE_SKIP_TEST_IF(!mInitialized);
    ANGLE_SKIP_TEST_IF(
        !IsEGLDisplayExtensionEnabled(mDisplay, "EGL_KHR_create_context") ||
        !IsEGLDisplayExtensionEnabled(mDisplay, "EGL_EXT_create_context_robustness"));

    createRobustContext(false);
    invalidShaderLocalVariableAccess();
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EGLRobustnessTest);
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EGLRobustnessTestES31);
ANGLE_INSTANTIATE_TEST(EGLRobustnessTest,
                       WithNoFixture(ES2_VULKAN()),
                       WithNoFixture(ES2_D3D9()),
                       WithNoFixture(ES2_D3D11()),
                       WithNoFixture(ES2_OPENGL()));
ANGLE_INSTANTIATE_TEST(EGLRobustnessTestES31, WithNoFixture(ES31_VULKAN()));
