//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "common/unsafe_buffers.h"
#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

class ViewportTest : public ANGLETest<>
{
  protected:
    ViewportTest()
    {
        setWindowWidth(512);
        setWindowHeight(512);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);

        mProgram = 0;
    }

    void runNonScissoredTest()
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        runTest();
    }

    void runScissoredTest()
    {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_SCISSOR_TEST);
        glScissor(0, getWindowHeight() / 2, getWindowWidth(), getWindowHeight() / 2);

        runTest();
    }

    void runTest()
    {
        // Firstly ensure that no errors have been hit.
        EXPECT_GL_NO_ERROR();

        std::array<GLint, 4> viewportSize;
        glGetIntegerv(GL_VIEWPORT, viewportSize.data());

        // Clear to green. Might be a scissored clear, if scissorSize != window size
        glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw a red quad centered in the middle of the viewport, with dimensions 25% of the size
        // of the viewport.
        drawQuad(mProgram, essl1_shaders::PositionAttrib(), 0.5f, 0.25f);

        GLint centerViewportX = viewportSize[0] + (viewportSize[2] / 2);
        GLint centerViewportY = viewportSize[1] + (viewportSize[3] / 2);

        GLint redQuadLeftSideX   = viewportSize[0] + viewportSize[2] * 3 / 8;
        GLint redQuadRightSideX  = viewportSize[0] + viewportSize[2] * 5 / 8;
        GLint redQuadTopSideY    = viewportSize[1] + viewportSize[3] * 3 / 8;
        GLint redQuadBottomSideY = viewportSize[1] + viewportSize[3] * 5 / 8;

        // The midpoint of the viewport should be red.
        checkPixel(centerViewportX, centerViewportY, true);

        // Pixels just inside the red quad should be red.
        checkPixel(redQuadLeftSideX, redQuadTopSideY, true);
        checkPixel(redQuadLeftSideX, redQuadBottomSideY - 1, true);
        checkPixel(redQuadRightSideX - 1, redQuadTopSideY, true);
        checkPixel(redQuadRightSideX - 1, redQuadBottomSideY - 1, true);

        // Pixels just outside the red quad shouldn't be red.
        checkPixel(redQuadLeftSideX - 1, redQuadTopSideY - 1, false);
        checkPixel(redQuadLeftSideX - 1, redQuadBottomSideY, false);
        checkPixel(redQuadRightSideX, redQuadTopSideY - 1, false);
        checkPixel(redQuadRightSideX, redQuadBottomSideY, false);

        // Pixels just within the viewport shouldn't be red.
        checkPixel(viewportSize[0], viewportSize[1], false);
        checkPixel(viewportSize[0], viewportSize[1] + viewportSize[3] - 1, false);
        checkPixel(viewportSize[0] + viewportSize[2] - 1, viewportSize[1], false);
        checkPixel(viewportSize[0] + viewportSize[2] - 1, viewportSize[1] + viewportSize[3] - 1,
                   false);
    }

    void checkPixel(GLint x, GLint y, GLboolean renderedRed)
    {
        // By default, expect the pixel to be black.
        GLint expectedRedChannel   = 0;
        GLint expectedGreenChannel = 0;

        GLint scissorSize[4];
        glGetIntegerv(GL_SCISSOR_BOX, scissorSize);

        EXPECT_GL_NO_ERROR();

        if (scissorSize[0] <= x && x < scissorSize[0] + scissorSize[2] && scissorSize[1] <= y &&
            y < scissorSize[1] + scissorSize[3])
        {
            // If the pixel lies within the scissor rect, then it should have been cleared to green.
            // If we rendered a red square on top of it, then the pixel should be red (the green
            // channel will have been reset to 0).
            expectedRedChannel   = renderedRed ? 255 : 0;
            expectedGreenChannel = renderedRed ? 0 : 255;
        }

        // If the pixel is within the bounds of the window, then we check it. Otherwise we skip it.
        if (0 <= x && x < getWindowWidth() && 0 <= y && y < getWindowHeight())
        {
            EXPECT_PIXEL_EQ(x, y, expectedRedChannel, expectedGreenChannel, 0, 255);
        }
    }

    void testSetUp() override
    {
        mProgram = CompileProgram(essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        glUseProgram(mProgram);

        glClearColor(0, 0, 0, 1);
        glClearDepthf(0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Call glViewport and glScissor with default parameters.
        glScissor(0, 0, getWindowWidth(), getWindowHeight());
        glViewport(0, 0, getWindowWidth(), getWindowHeight());

        glDisable(GL_DEPTH_TEST);
    }

    void testTearDown() override { glDeleteProgram(mProgram); }

    GLuint mProgram;
};

TEST_P(ViewportTest, QuarterWindow)
{
    glViewport(0, 0, getWindowWidth() / 4, getWindowHeight() / 4);

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, QuarterWindowCentered)
{
    glViewport(getWindowWidth() * 3 / 8, getWindowHeight() * 3 / 8, getWindowWidth() / 4,
               getWindowHeight() / 4);

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, FullWindow)
{
    glViewport(0, 0, getWindowWidth(), getWindowHeight());

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, FullWindowOffCenter)
{
    glViewport(-getWindowWidth() / 2, getWindowHeight() / 2, getWindowWidth(), getWindowHeight());

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, DoubleWindow)
{
    glViewport(0, 0, getWindowWidth() * 2, getWindowHeight() * 2);

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, DoubleWindowCentered)
{
    glViewport(-getWindowWidth() / 2, -getWindowHeight() / 2, getWindowWidth() * 2,
               getWindowHeight() * 2);

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, DoubleWindowOffCenter)
{
    glViewport(-getWindowWidth() * 3 / 4, getWindowHeight() * 3 / 4, getWindowWidth(),
               getWindowHeight());

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, TripleWindow)
{
    glViewport(0, 0, getWindowWidth() * 3, getWindowHeight() * 3);

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, TripleWindowCentered)
{
    glViewport(-getWindowWidth(), -getWindowHeight(), getWindowWidth() * 3, getWindowHeight() * 3);

    runNonScissoredTest();

    runScissoredTest();
}

TEST_P(ViewportTest, TripleWindowOffCenter)
{
    glViewport(-getWindowWidth() * 3 / 2, -getWindowHeight() * 3 / 2, getWindowWidth() * 3,
               getWindowHeight() * 3);

    runNonScissoredTest();

    runScissoredTest();
}

// Test line rendering with a non-standard viewport.
TEST_P(ViewportTest, DrawLineWithViewport)
{
    // We assume in the test the width and height are equal and we are tracing
    // the line from bottom left to top right. Verify that all pixels along that line
    // have been traced with green.
    ASSERT_EQ(getWindowWidth(), getWindowHeight());

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glUseProgram(program);

    std::vector<Vector3> vertices = {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};

    const GLint positionLocation = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    GLBuffer vertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(positionLocation);

    // Set the viewport.
    GLint quarterWidth  = getWindowWidth() / 4;
    GLint quarterHeight = getWindowHeight() / 4;
    glViewport(quarterWidth, quarterHeight, quarterWidth, quarterHeight);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

    glDisableVertexAttribArray(positionLocation);

    ASSERT_GL_NO_ERROR();

    for (GLint x = quarterWidth; x < getWindowWidth() / 2; x++)
    {
        EXPECT_PIXEL_COLOR_EQ(x, x, GLColor::green);
    }
}

// Test line rendering with an overly large viewport.
TEST_P(ViewportTest, DrawLineWithLargeViewport)
{
    // We assume in the test the width and height are equal and we are tracing
    // the line from bottom left to top right. Verify that all pixels along that line
    // have been traced with green.
    ASSERT_EQ(getWindowWidth(), getWindowHeight());

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glUseProgram(program);

    std::vector<Vector3> vertices = {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};

    const GLint positionLocation = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    GLBuffer vertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(positionLocation);

    // Set the viewport.
    glViewport(0, 0, getWindowWidth() * 2, getWindowHeight() * 2);

    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

    glDisableVertexAttribArray(positionLocation);

    ASSERT_GL_NO_ERROR();

    for (GLint x = 0; x < getWindowWidth(); x++)
    {
        EXPECT_PIXEL_COLOR_EQ(x, x, GLColor::green);
    }
}

// Test very large viewport sizes so sanitizers can verify there is no undefined behaviour
TEST_P(ViewportTest, Overflow)
{
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glUseProgram(program);

    std::vector<Vector3> vertices = {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}};

    const GLint positionLocation = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    GLBuffer vertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(),
                 GL_STATIC_DRAW);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(positionLocation);

    constexpr int kMaxSize = std::numeric_limits<int>::max();
    constexpr std::array<std::array<int, 4>, 4> kTestViewportSizes = {{
        {
            kMaxSize,
            kMaxSize,
            1,
            1,
        },
        {
            0,
            0,
            kMaxSize,
            kMaxSize,
        },
        {
            1,
            1,
            kMaxSize,
            kMaxSize,
        },
        {
            kMaxSize,
            kMaxSize,
            kMaxSize,
            kMaxSize,
        },
    }};

    for (const auto &viewportSize : kTestViewportSizes)
    {
        // Set the viewport.
        glViewport(viewportSize[0], viewportSize[1], viewportSize[2], viewportSize[3]);

        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertices.size()));

        glDisableVertexAttribArray(positionLocation);

        ASSERT_GL_NO_ERROR();
    }
}

// Test that viewport dimensions are clamped to implementation-defined maximums
TEST_P(ViewportTest, ClampOnStore)
{
    GLint maxDims[2];
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxDims);
    ASSERT_GT(maxDims[0], 0);
    ASSERT_GT(maxDims[1], 0);

    GLint viewport[4];
    glViewport(0, 0, 1, 1);
    ASSERT_GL_NO_ERROR();
    glGetIntegerv(GL_VIEWPORT, viewport);
    ASSERT_EQ(viewport[0], 0);
    ASSERT_EQ(viewport[1], 0);
    ASSERT_EQ(viewport[2], 1);
    ASSERT_EQ(viewport[3], 1);

    glViewport(0, 0, -2, 2);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);
    glGetIntegerv(GL_VIEWPORT, viewport);
    ASSERT_EQ(viewport[0], 0);
    ASSERT_EQ(viewport[1], 0);
    ASSERT_EQ(viewport[2], 1);
    ASSERT_EQ(viewport[3], 1);

    glViewport(0, 0, 3, -3);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);
    glGetIntegerv(GL_VIEWPORT, viewport);
    ASSERT_EQ(viewport[0], 0);
    ASSERT_EQ(viewport[1], 0);
    ASSERT_EQ(viewport[2], 1);
    ASSERT_EQ(viewport[3], 1);

    glViewport(0, 0, maxDims[0], maxDims[1]);
    ASSERT_GL_NO_ERROR();
    glGetIntegerv(GL_VIEWPORT, viewport);
    ASSERT_EQ(viewport[0], 0);
    ASSERT_EQ(viewport[1], 0);
    ASSERT_EQ(viewport[2], maxDims[0]);
    ASSERT_EQ(viewport[3], maxDims[1]);

    glViewport(0, 0, maxDims[0] + 1, maxDims[1] + 1);
    ASSERT_GL_NO_ERROR();
    glGetIntegerv(GL_VIEWPORT, viewport);
    ASSERT_EQ(viewport[0], 0);
    ASSERT_EQ(viewport[1], 0);
    ASSERT_EQ(viewport[2], maxDims[0]);
    ASSERT_EQ(viewport[3], maxDims[1]);
}

class ViewportDepthRangeTest : public ANGLETest<>
{
  protected:
    ViewportDepthRangeTest()
    {
        setWindowWidth(64);
        setWindowHeight(64);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    void testSetUp() override
    {
        mProgram = CompileProgram(essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
        ASSERT_NE(0u, mProgram);
        glUseProgram(mProgram);
        mColorLocation = glGetUniformLocation(mProgram, essl1_shaders::ColorUniform());
        ASSERT_NE(-1, mColorLocation);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0, 0, 0, 1);
    }

    void testTearDown() override { glDeleteProgram(mProgram); }

    void setDepthRange(GLfloat nearPlane, GLfloat farPlane)
    {
        glDepthRangef(nearPlane, farPlane);
        glDepthFunc(nearPlane > farPlane ? GL_GREATER : GL_LESS);
        glClearDepthf(nearPlane > farPlane ? 0.0f : 1.0f);
    }

    void checkDepthRange(GLfloat nearPlane, GLfloat farPlane)
    {
        GLfloat range[2];
        glGetFloatv(GL_DEPTH_RANGE, range);
        EXPECT_FLOAT_EQ(nearPlane, range[0]);
        EXPECT_FLOAT_EQ(farPlane, range[1]);

        // Probe around NDC z = -0.5 against a fixed depth reference.
        glClearDepthf(nearPlane + (farPlane - nearPlane) * 0.25f);
        glDepthMask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthMask(GL_FALSE);
        glUniform4f(mColorLocation, 0, 1, 0, 1);
        drawQuad(mProgram, essl1_shaders::PositionAttrib(), -0.51f);
        glUniform4f(mColorLocation, 1, 0, 0, 1);
        drawQuad(mProgram, essl1_shaders::PositionAttrib(), -0.49f);
        glDepthMask(GL_TRUE);
        ASSERT_GL_NO_ERROR();
        EXPECT_PIXEL_COLOR_EQ(32, 32, GLColor::green);
    }

    void allocateAttachments(GLuint color, GLuint depth, GLsizei size)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, color);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA4, size, size);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size, size);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    }

    static constexpr std::array<std::array<GLfloat, 2>, 4> kDepthRanges = {
        {{1.0f, 0.0f}, {0.75f, 0.25f}, {0.0f, 1.0f}, {0.25f, 0.75f}}};

    GLuint mProgram      = 0;
    GLint mColorLocation = -1;
};

// Preserve depth range when only the viewport is dirty.
TEST_P(ViewportDepthRangeTest, ViewportChangeOnly)
{
    GLint positionLocation = glGetAttribLocation(mProgram, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);
    setupQuadVertexBuffer(-0.5f, 1.0f);
    glEnableVertexAttribArray(positionLocation);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glUniform4f(mColorLocation, 0, 0.5f, 0, 1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    for (const auto &range : kDepthRanges)
    {
        SCOPED_TRACE(::testing::Message() << range[0] << ", " << range[1]);
        for (const auto &viewport : {std::array<GLint, 4>{0, 0, 63, 64}, {1, 1, 63, 63}})
        {
            SCOPED_TRACE(::testing::Message() << viewport[0] << ", " << viewport[1]);
            glViewport(0, 0, 64, 64);
            setDepthRange(range[0], range[1]);
            glDepthFunc(range[0] > range[1] ? GL_GEQUAL : GL_LEQUAL);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Both draws must pass; additive blending yields full green.
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            ASSERT_GL_NO_ERROR();
            EXPECT_PIXEL_COLOR_EQ(32, 32, GLColor::green);
        }
    }
}

// Preserve depth range across viewport changes.
TEST_P(ViewportDepthRangeTest, ViewportChanges)
{
    for (const auto &range : kDepthRanges)
    {
        SCOPED_TRACE(::testing::Message() << range[0] << ", " << range[1]);
        glViewport(0, 0, 64, 64);
        setDepthRange(range[0], range[1]);
        checkDepthRange(range[0], range[1]);
        glViewport(0, 0, 63, 64);
        checkDepthRange(range[0], range[1]);
        glViewport(1, 0, 63, 64);
        checkDepthRange(range[0], range[1]);
        glViewport(1, 1, 63, 63);
        // An unchanged range must not dirty state.
        glDepthRangef(range[0], range[1]);
        checkDepthRange(range[0], range[1]);
        glViewport(0, 0, 64, 64);
        checkDepthRange(range[0], range[1]);
    }
}

// Preserve depth range across framebuffer switches.
TEST_P(ViewportDepthRangeTest, FramebufferChanges)
{
    GLFramebuffer framebuffers[2];
    GLRenderbuffer colors[2];
    GLRenderbuffer depths[2];
    for (size_t index = 0; index < 2; ++index)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[index]);
        allocateAttachments(colors[index], depths[index], 64 + static_cast<GLsizei>(index));
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, 64, 64);
    for (const auto &range : kDepthRanges)
    {
        SCOPED_TRACE(::testing::Message() << range[0] << ", " << range[1]);
        setDepthRange(range[0], range[1]);
        checkDepthRange(range[0], range[1]);
        for (size_t index : {0u, 1u, 0u})
        {
            glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[index]);
            checkDepthRange(range[0], range[1]);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        checkDepthRange(range[0], range[1]);
    }
}

// Preserve depth range when attachments resize.
TEST_P(ViewportDepthRangeTest, FramebufferResize)
{
    GLFramebuffer framebuffer;
    GLRenderbuffer color;
    GLRenderbuffer depth;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    allocateAttachments(color, depth, 64);
    glViewport(0, 0, 64, 64);
    for (const auto &range : kDepthRanges)
    {
        SCOPED_TRACE(::testing::Message() << range[0] << ", " << range[1]);
        setDepthRange(range[0], range[1]);
        checkDepthRange(range[0], range[1]);
        for (GLsizei size : {65, 64})
        {
            allocateAttachments(color, depth, size);
            checkDepthRange(range[0], range[1]);
        }
    }
}

ANGLE_INSTANTIATE_TEST_ES2_AND_ES3(ViewportDepthRangeTest);

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST_ES2_AND(ViewportTest, ES2_D3D11_PRESENT_PATH_FAST());

// This test suite is not instantiated on some OSes.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ViewportTest);

}  // namespace
