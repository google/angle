//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BlendFuncExtendedTest
//   Test EXT_blend_func_extended

#include "test_utils/ANGLETest.h"

#include "shader_utils.h"

#include <algorithm>
#include <cmath>
#include <fstream>

using namespace angle;

namespace
{

// Partial implementation of weight function for GLES 2 blend equation that
// is dual-source aware.
template <int factor, int index>
float Weight(const float /*dst*/[4], const float src[4], const float src1[4])
{
    if (factor == GL_SRC_COLOR)
        return src[index];
    if (factor == GL_SRC_ALPHA)
        return src[3];
    if (factor == GL_SRC1_COLOR_EXT)
        return src1[index];
    if (factor == GL_SRC1_ALPHA_EXT)
        return src1[3];
    if (factor == GL_ONE_MINUS_SRC1_COLOR_EXT)
        return 1.0f - src1[index];
    if (factor == GL_ONE_MINUS_SRC1_ALPHA_EXT)
        return 1.0f - src1[3];
    return 0.0f;
}

GLubyte ScaleChannel(float weight)
{
    return static_cast<GLubyte>(std::floor(std::max(0.0f, std::min(1.0f, weight)) * 255.0f));
}

// Implementation of GLES 2 blend equation that is dual-source aware.
template <int RGBs, int RGBd, int As, int Ad>
void BlendEquationFuncAdd(const float dst[4],
                          const float src[4],
                          const float src1[4],
                          angle::GLColor *result)
{
    float r[4];
    r[0] = src[0] * Weight<RGBs, 0>(dst, src, src1) + dst[0] * Weight<RGBd, 0>(dst, src, src1);
    r[1] = src[1] * Weight<RGBs, 1>(dst, src, src1) + dst[1] * Weight<RGBd, 1>(dst, src, src1);
    r[2] = src[2] * Weight<RGBs, 2>(dst, src, src1) + dst[2] * Weight<RGBd, 2>(dst, src, src1);
    r[3] = src[3] * Weight<As, 3>(dst, src, src1) + dst[3] * Weight<Ad, 3>(dst, src, src1);

    result->R = ScaleChannel(r[0]);
    result->G = ScaleChannel(r[1]);
    result->B = ScaleChannel(r[2]);
    result->A = ScaleChannel(r[3]);
}

void CheckPixels(GLint x,
                 GLint y,
                 GLsizei width,
                 GLsizei height,
                 GLint tolerance,
                 const angle::GLColor &color)
{
    for (GLint yy = 0; yy < height; ++yy)
    {
        for (GLint xx = 0; xx < width; ++xx)
        {
            const auto px = x + xx;
            const auto py = y + yy;
            EXPECT_PIXEL_COLOR_NEAR(px, py, color, 1);
        }
    }
}

const GLuint kWidth  = 100;
const GLuint kHeight = 100;

class EXTBlendFuncExtendedTest : public ANGLETest
{
};

class EXTBlendFuncExtendedDrawTest : public ANGLETest
{
  protected:
    EXTBlendFuncExtendedDrawTest() : mProgram(0)
    {
        setWindowWidth(kWidth);
        setWindowHeight(kHeight);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        glGenBuffers(1, &mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);

        static const float vertices[] = {
            1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        ASSERT_GL_NO_ERROR();
    }

    void TearDown() override
    {
        glDeleteBuffers(1, &mVBO);
        if (mProgram)
        {
            glDeleteProgram(mProgram);
        }

        ASSERT_GL_NO_ERROR();

        ANGLETest::TearDown();
    }

    void makeProgram(const char *vertSource, const char *fragSource)
    {
        mProgram = CompileProgram(vertSource, fragSource);

        ASSERT_NE(0u, mProgram);
    }

    void drawTest()
    {
        glUseProgram(mProgram);

        GLint position = glGetAttribLocation(mProgram, "position");
        GLint src0     = glGetUniformLocation(mProgram, "src0");
        GLint src1     = glGetUniformLocation(mProgram, "src1");
        ASSERT_GL_NO_ERROR();

        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glEnableVertexAttribArray(position);
        glVertexAttribPointer(position, 2, GL_FLOAT, GL_FALSE, 0, 0);
        ASSERT_GL_NO_ERROR();

        static const float kDst[4]  = {0.5f, 0.5f, 0.5f, 0.5f};
        static const float kSrc0[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        static const float kSrc1[4] = {0.3f, 0.6f, 0.9f, 0.7f};

        glUniform4f(src0, kSrc0[0], kSrc0[1], kSrc0[2], kSrc0[3]);
        glUniform4f(src1, kSrc1[0], kSrc1[1], kSrc1[2], kSrc1[3]);
        ASSERT_GL_NO_ERROR();

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glViewport(0, 0, kWidth, kHeight);
        glClearColor(kDst[0], kDst[1], kDst[2], kDst[3]);
        ASSERT_GL_NO_ERROR();

        {
            glBlendFuncSeparate(GL_SRC1_COLOR_EXT, GL_SRC_ALPHA, GL_ONE_MINUS_SRC1_COLOR_EXT,
                                GL_ONE_MINUS_SRC1_ALPHA_EXT);

            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            ASSERT_GL_NO_ERROR();

            // verify
            angle::GLColor color;
            BlendEquationFuncAdd<GL_SRC1_COLOR_EXT, GL_SRC_ALPHA, GL_ONE_MINUS_SRC1_COLOR_EXT,
                                 GL_ONE_MINUS_SRC1_ALPHA_EXT>(kDst, kSrc0, kSrc1, &color);

            CheckPixels(kWidth / 4, (3 * kHeight) / 4, 1, 1, 1, color);
            CheckPixels(kWidth - 1, 0, 1, 1, 1, color);
        }

        {
            glBlendFuncSeparate(GL_ONE_MINUS_SRC1_COLOR_EXT, GL_ONE_MINUS_SRC_ALPHA,
                                GL_ONE_MINUS_SRC_COLOR, GL_SRC1_ALPHA_EXT);

            glClear(GL_COLOR_BUFFER_BIT);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            ASSERT_GL_NO_ERROR();

            // verify
            angle::GLColor color;
            BlendEquationFuncAdd<GL_ONE_MINUS_SRC1_COLOR_EXT, GL_ONE_MINUS_SRC_ALPHA,
                                 GL_ONE_MINUS_SRC_COLOR, GL_SRC1_ALPHA_EXT>(kDst, kSrc0, kSrc1,
                                                                            &color);

            CheckPixels(kWidth / 4, (3 * kHeight) / 4, 1, 1, 1, color);
            CheckPixels(kWidth - 1, 0, 1, 1, 1, color);
        }
    }

    GLuint mVBO;
    GLuint mProgram;
};

}  // namespace

// Test EXT_blend_func_extended related gets.
TEST_P(EXTBlendFuncExtendedTest, TestMaxDualSourceDrawBuffers)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_blend_func_extended"));

    GLint maxDualSourceDrawBuffers = 0;
    glGetIntegerv(GL_MAX_DUAL_SOURCE_DRAW_BUFFERS_EXT, &maxDualSourceDrawBuffers);
    EXPECT_GT(maxDualSourceDrawBuffers, 0);

    ASSERT_GL_NO_ERROR();
}

// Test a shader with EXT_blend_func_extended and gl_SecondaryFragColorEXT.
// Outputs to primary color buffer using primary and secondary colors.
TEST_P(EXTBlendFuncExtendedDrawTest, FragColor)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_blend_func_extended"));

    const char *kVertexShader =
        "attribute vec4 position;\n"
        "void main() {\n"
        "  gl_Position = position;\n"
        "}\n";

    const char *kFragColorShader =
        "#extension GL_EXT_blend_func_extended : require\n"
        "precision mediump float;\n"
        "uniform vec4 src0;\n"
        "uniform vec4 src1;\n"
        "void main() {\n"
        "  gl_FragColor = src0;\n"
        "  gl_SecondaryFragColorEXT = src1;\n"
        "}\n";

    makeProgram(kVertexShader, kFragColorShader);

    drawTest();
}

// Test a shader with EXT_blend_func_extended and gl_FragData.
// Outputs to a color buffer using primary and secondary frag data.
TEST_P(EXTBlendFuncExtendedDrawTest, FragData)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_blend_func_extended"));

    const char *kVertexShader =
        "attribute vec4 position;\n"
        "void main() {\n"
        "  gl_Position = position;\n"
        "}\n";

    const char *kFragColorShader =
        "#extension GL_EXT_blend_func_extended : require\n"
        "precision mediump float;\n"
        "uniform vec4 src0;\n"
        "uniform vec4 src1;\n"
        "void main() {\n"
        "  gl_FragData[0] = src0;\n"
        "  gl_SecondaryFragDataEXT[0] = src1;\n"
        "}\n";

    makeProgram(kVertexShader, kFragColorShader);

    drawTest();
}

ANGLE_INSTANTIATE_TEST(EXTBlendFuncExtendedTest,
                       ES2_OPENGL(),
                       ES2_OPENGLES(),
                       ES3_OPENGL(),
                       ES3_OPENGLES());
ANGLE_INSTANTIATE_TEST(EXTBlendFuncExtendedDrawTest, ES2_OPENGL());
