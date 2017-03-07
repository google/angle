//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RobustResourceInitTest: Tests for GL_ANGLE_robust_resource_initialization.

#include "test_utils/ANGLETest.h"

#include "test_utils/gl_raii.h"

namespace angle
{

class RobustResourceInitTest : public ANGLETest
{
  protected:
    RobustResourceInitTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);

        // Defer context init until the test body.
        setDeferContextInit(true);
        setRobustResourceInit(true);
    }
};

// Context creation should fail if EGL_ANGLE_create_context_robust_resource_initialization
// is not available, and succeed otherwise.
TEST_P(RobustResourceInitTest, ExtensionInit)
{
    EGLDisplay display = getEGLWindow()->getDisplay();
    ASSERT_TRUE(display != EGL_NO_DISPLAY);

    if (eglDisplayExtensionEnabled(display,
                                   "EGL_ANGLE_create_context_robust_resource_initialization"))
    {
        // Context creation shold succeed with robust resource init enabled.
        EXPECT_TRUE(getEGLWindow()->initializeContext());

        // Robust resource init extension should be available.
        EXPECT_TRUE(extensionEnabled("GL_ANGLE_robust_resource_initialization"));

        // Querying the state value should return true.
        GLboolean enabled = 0;
        glGetBooleanv(GL_CONTEXT_ROBUST_RESOURCE_INITIALIZATION_ANGLE, &enabled);
        EXPECT_GL_NO_ERROR();
        EXPECT_GL_TRUE(enabled);

        EXPECT_GL_TRUE(glIsEnabled(GL_CONTEXT_ROBUST_RESOURCE_INITIALIZATION_ANGLE));
    }
    else
    {
        // Context creation should fail with robust resource init enabled.
        EXPECT_FALSE(getEGLWindow()->initializeContext());

        // Context creation should succeed with robust resource init disabled.
        setRobustResourceInit(false);
        ASSERT_TRUE(getEGLWindow()->initializeGL(GetOSWindow()));

        // If context extension string exposed, check queries.
        if (extensionEnabled("GL_ANGLE_robust_resource_initialization"))
        {
            GLboolean enabled = 0;
            glGetBooleanv(GL_CONTEXT_ROBUST_RESOURCE_INITIALIZATION_ANGLE, &enabled);
            EXPECT_GL_FALSE(enabled);

            EXPECT_GL_FALSE(glIsEnabled(GL_CONTEXT_ROBUST_RESOURCE_INITIALIZATION_ANGLE));
            EXPECT_GL_NO_ERROR();
        }
        else
        {
            // Querying robust resource init should return INVALID_ENUM.
            GLboolean enabled = 0;
            glGetBooleanv(GL_CONTEXT_ROBUST_RESOURCE_INITIALIZATION_ANGLE, &enabled);
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
        }
    }
}

// Test queries on a normal, non-robust enabled context.
TEST_P(RobustResourceInitTest, QueriesOnNonRobustContext)
{
    EGLDisplay display = getEGLWindow()->getDisplay();
    ASSERT_TRUE(display != EGL_NO_DISPLAY);

    if (!eglDisplayExtensionEnabled(display,
                                    "EGL_ANGLE_create_context_robust_resource_initialization"))
    {
        return;
    }

    setRobustResourceInit(false);
    EXPECT_TRUE(getEGLWindow()->initializeContext());

    // If context extension string exposed, check queries.
    ASSERT_TRUE(extensionEnabled("GL_ANGLE_robust_resource_initialization"));

    // Querying robust resource init should return INVALID_ENUM.
    GLboolean enabled = 0;
    glGetBooleanv(GL_CONTEXT_ROBUST_RESOURCE_INITIALIZATION_ANGLE, &enabled);
    EXPECT_GL_FALSE(enabled);

    EXPECT_GL_FALSE(glIsEnabled(GL_CONTEXT_ROBUST_RESOURCE_INITIALIZATION_ANGLE));
    EXPECT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST(RobustResourceInitTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES3_OPENGL(),
                       ES2_OPENGLES(),
                       ES3_OPENGLES());
}  // namespace
