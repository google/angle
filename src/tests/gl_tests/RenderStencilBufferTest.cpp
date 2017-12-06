//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RenderStencilBufferTest:
//   Reproduce driver bug on Intel windows and mac when rendering with stencil
//   buffer enabled, depth buffer disabled and large viewport.

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

class RenderStencilBufferTest : public ANGLETest
{
  protected:
    RenderStencilBufferTest() : mProgram(0)
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
        setWebGLCompatibilityEnabled(true);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        const std::string vertexShaderSource =
            R"(attribute vec4 position;
            void main()
            {
                gl_Position = position;
            })";

        const std::string fragmentShaderSource =
            R"(precision mediump float;
            uniform vec4 u_draw_color;
            void main()
            {
                gl_FragColor = u_draw_color;
            })";

        mProgram = CompileProgram(vertexShaderSource, fragmentShaderSource);
        ASSERT_NE(0u, mProgram);

        glUseProgram(mProgram);

        GLint positionLoc = glGetAttribLocation(mProgram, "position");
        ASSERT_NE(-1, positionLoc);

        setupQuadVertexBuffer(1.0f, 1.0f);
        glEnableVertexAttribArray(positionLoc);
        glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        GLint colorLoc = glGetUniformLocation(mProgram, "u_draw_color");
        ASSERT_NE(-1, colorLoc);
        glUniform4f(colorLoc, 1.0f, 0.0f, 0.0f, 1.0f);
        ASSERT_GL_NO_ERROR();

        glEnable(GL_STENCIL_TEST);
    }

    void TearDown() override
    {
        glDisable(GL_STENCIL_TEST);
        if (mProgram != 0)
            glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    GLuint mProgram;
};

// This test reproduce driver bug on Intel windows platforms on driver version
// from 4815 to 4877.
// When rendering with Stencil buffer enabled and depth buffer disabled, and
// large viewport will lead to memory leak and driver crash. And the pixel
// result is a random value.
TEST_P(RenderStencilBufferTest, DrawWithLargeViewport)
{
    ANGLE_SKIP_TEST_IF(IsIntel() && IsOSX());

    // The iteration is to reproduce memory leak when rendering several times.
    for (int i = 0; i < 10; ++i)
    {
        // Create offscreen fbo and its color attachment and depth stencil attachment.
        GLTexture framebufferColorTexture;
        glBindTexture(GL_TEXTURE_2D, framebufferColorTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, getWindowWidth(), getWindowHeight());

        ASSERT_GL_NO_ERROR();

        GLTexture framebufferStencilTexture;
        glBindTexture(GL_TEXTURE_2D, framebufferStencilTexture);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH24_STENCIL8, getWindowWidth(), getWindowHeight());

        ASSERT_GL_NO_ERROR();

        GLFramebuffer fb;
        glBindFramebuffer(GL_FRAMEBUFFER, fb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               framebufferColorTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                               framebufferStencilTexture, 0);

        EXPECT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));
        ASSERT_GL_NO_ERROR();

        glEnable(GL_STENCIL_TEST);
        glDisable(GL_DEPTH_TEST);

        GLint kStencilRef = 4;
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, kStencilRef, 0xFF);

        glViewport(0, 0, 16384, 16384);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        ASSERT_GL_NO_ERROR();
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);

        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
        EXPECT_GL_NO_ERROR();
    }
}

ANGLE_INSTANTIATE_TEST(RenderStencilBufferTest, ES3_D3D11(), ES3_OPENGL());
