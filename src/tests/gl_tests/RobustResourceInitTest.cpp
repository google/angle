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

    bool hasEGLExtension()
    {
        EGLDisplay display = getEGLWindow()->getDisplay();
        ASSERT(display != EGL_NO_DISPLAY);

        return (eglDisplayExtensionEnabled(
            display, "EGL_ANGLE_create_context_robust_resource_initialization"));
    }

    bool setup()
    {
        if (!hasEGLExtension())
        {
            return false;
        }

        if (!getEGLWindow()->initializeContext())
        {
            EXPECT_TRUE(false);
            return false;
        }

        return true;
    }
};

// Context creation should fail if EGL_ANGLE_create_context_robust_resource_initialization
// is not available, and succeed otherwise.
TEST_P(RobustResourceInitTest, ExtensionInit)
{
    if (hasEGLExtension())
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

    if (!hasEGLExtension())
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

// Tests that buffers start zero-filled if the data pointer is null.
TEST_P(RobustResourceInitTest, BufferData)
{
    if (!setup())
    {
        return;
    }

    GLBuffer buffer;
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, getWindowWidth() * getWindowHeight() * sizeof(GLfloat), nullptr,
                 GL_STATIC_DRAW);

    const std::string &vertexShader =
        "attribute vec2 position;\n"
        "attribute float testValue;\n"
        "varying vec4 colorOut;\n"
        "void main() {\n"
        "    gl_Position = vec4(position, 0, 1);\n"
        "    colorOut = testValue == 0.0 ? vec4(0, 1, 0, 1) : vec4(1, 0, 0, 1);\n"
        "}";
    const std::string &fragmentShader =
        "varying mediump vec4 colorOut;\n"
        "void main() {\n"
        "    gl_FragColor = colorOut;\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertexShader, fragmentShader);

    GLint testValueLoc = glGetAttribLocation(program.get(), "testValue");
    ASSERT_NE(-1, testValueLoc);

    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(testValueLoc, 1, GL_FLOAT, GL_FALSE, 4, nullptr);
    glEnableVertexAttribArray(testValueLoc);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    drawQuad(program.get(), "position", 0.5f);

    ASSERT_GL_NO_ERROR();

    std::vector<GLColor> expected(getWindowWidth() * getWindowHeight(), GLColor::green);
    std::vector<GLColor> actual(getWindowWidth() * getWindowHeight());
    glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE,
                 actual.data());
    EXPECT_EQ(expected, actual);
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
