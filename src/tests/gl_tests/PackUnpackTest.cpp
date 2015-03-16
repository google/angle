//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// PackUnpackTest:
//   Tests the corrrectness of opengl 4.1 emulation of pack/unpack built-in functions.
//

#include "test_utils/ANGLETest.h"

using namespace angle;

namespace
{

class PackUnpackTest : public ANGLETest
{
  protected:
    PackUnpackTest()
    {
        setWindowWidth(16);
        setWindowHeight(16);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        // Vertex Shader source
        const std::string vs = SHADER_SOURCE
        (   #version 300 es\n
            precision mediump float;
            in vec4 position;

            void main()
            {
                gl_Position = position;
            }
        );

        // Fragment Shader source
        const std::string sNormFS = SHADER_SOURCE
        (   #version 300 es\n
            precision mediump float;
            uniform mediump vec2 v;
            layout(location = 0) out mediump vec4 fragColor;

            void main()
            {
                uint u = packSnorm2x16(v);
                vec2 r = unpackSnorm2x16(u);
                if (r.x < 0.0) r.x = 1.0 + r.x;
                if (r.y < 0.0) r.y = 1.0 + r.y;
                fragColor = vec4(r, 0.0, 1.0);
            }
        );

        // Fragment Shader source
        const std::string halfFS = SHADER_SOURCE
        (   #version 300 es\n
            precision mediump float;
            uniform mediump vec2 v;
            layout(location = 0) out mediump vec4 fragColor;

             void main()
             {
                 uint u = packHalf2x16(v);
                 vec2 r = unpackHalf2x16(u);
                 if (r.x < 0.0) r.x = 1.0 + r.x;
                 if (r.y < 0.0) r.y = 1.0 + r.y;
                 fragColor = vec4(r, 0.0, 1.0);
             }
        );

        mSNormProgram = CompileProgram(vs, sNormFS);
        mHalfProgram = CompileProgram(vs, halfFS);
        if (mSNormProgram == 0 || mHalfProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        glGenFramebuffers(1, &mOffscreenFramebuffer);
        glGenTextures(1, &mOffscreenTexture2D);
    }

    void TearDown() override
    {
        glDeleteTextures(1, &mOffscreenTexture2D);
        glDeleteFramebuffers(1, &mOffscreenFramebuffer);
        glDeleteProgram(mSNormProgram);
        glDeleteProgram(mHalfProgram);

        ANGLETest::TearDown();
    }

    double computeOutput(float input)
    {
        if (input <= -1.0)
            return 0;
        else if (input >= 1.0)
            return 255;
        else if (input < -6.10E-05 && input > -1.0)
            return 255 * (1.0 + input);
        else
            return 255 * input;
    }

    void compareBeforeAfter(GLuint program, float input1, float input2)
    {
        glBindTexture(GL_TEXTURE_2D, mOffscreenTexture2D);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWindowWidth(), getWindowHeight(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, mOffscreenFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mOffscreenTexture2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, mOffscreenFramebuffer);
        glViewport(0, 0, 16, 16);
        const GLfloat color[] = { 1.0f, 1.0f, 0.0f, 1.0f };
        glClearBufferfv(GL_COLOR, 0, color);

        GLfloat vertexLocations[] =
        {
            -1.0f, 1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
        };

        GLint positionLocation = glGetAttribLocation(program, "position");
        GLint vec2Location = glGetUniformLocation(program, "v");
        glUseProgram(program);
        glUniform2f(vec2Location, input1, input2);
        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, vertexLocations);
        glEnableVertexAttribArray(positionLocation);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glUseProgram(0);

        ASSERT_GL_NO_ERROR();
        EXPECT_PIXEL_NEAR(8, 8, computeOutput(input1), computeOutput(input2), 0, 255, 1.0);
    }

    GLuint mSNormProgram;
    GLuint mHalfProgram;
    GLuint mOffscreenFramebuffer;
    GLuint mOffscreenTexture2D;
};

// Test the correctness of packSnorm2x16 and unpackSnorm2x16 functions calculating normal floating numbers.
TEST_P(PackUnpackTest, PackUnpackSnormNormal)
{
    compareBeforeAfter(mSNormProgram, 0.5f, -0.2f);
    compareBeforeAfter(mSNormProgram, -0.35f, 0.75f);
    compareBeforeAfter(mSNormProgram, 0.00392f, -0.99215f);
    compareBeforeAfter(mSNormProgram, 1.0f, -0.00392f);
}

// Test the correctness of packHalf2x16 and unpackHalf2x16 functions calculating normal floating numbers.
TEST_P(PackUnpackTest, PackUnpackHalfNormal)
{
    compareBeforeAfter(mHalfProgram, 0.5f, -0.2f);
    compareBeforeAfter(mHalfProgram, -0.35f, 0.75f);
    compareBeforeAfter(mHalfProgram, 0.00392f, -0.99215f);
    compareBeforeAfter(mHalfProgram, 1.0f, -0.00392f);
}

// Test the correctness of packSnorm2x16 and unpackSnorm2x16 functions calculating subnormal floating numbers.
TEST_P(PackUnpackTest, PackUnpackSnormSubnormal)
{
    compareBeforeAfter(mSNormProgram, 0.00001f, -0.00001f);
}

// Test the correctness of packHalf2x16 and unpackHalf2x16 functions calculating subnormal floating numbers.
TEST_P(PackUnpackTest, PackUnpackHalfSubnormal)
{
    compareBeforeAfter(mHalfProgram, 0.00001f, -0.00001f);
}

// Test the correctness of packSnorm2x16 and unpackSnorm2x16 functions calculating zero floating numbers.
TEST_P(PackUnpackTest, PackUnpackSnormZero)
{
    compareBeforeAfter(mSNormProgram, 0.00000f, -0.00000f);
}

// Test the correctness of packHalf2x16 and unpackHalf2x16 functions calculating zero floating numbers.
TEST_P(PackUnpackTest, PackUnpackHalfZero)
{
    compareBeforeAfter(mHalfProgram, 0.00000f, -0.00000f);
}

// Test the correctness of packSnorm2x16 and unpackSnorm2x16 functions calculating overflow floating numbers.
TEST_P(PackUnpackTest, PackUnpackSnormOverflow)
{
    compareBeforeAfter(mHalfProgram, 67000.0f, -67000.0f);
}

// Test the correctness of packHalf2x16 and unpackHalf2x16 functions calculating overflow floating numbers.
TEST_P(PackUnpackTest, PackUnpackHalfOverflow)
{
    compareBeforeAfter(mHalfProgram, 67000.0f, -67000.0f);
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_INSTANTIATE_TEST(PackUnpackTest, ES3_OPENGL());

}