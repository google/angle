//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"

#include "random_utils.h"
#include "shader_utils.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

Vector4 RandomVec4(int seed, float minValue, float maxValue)
{
    RNG rng(seed);
    srand(seed);
    return Vector4(
        rng.randomFloatBetween(minValue, maxValue), rng.randomFloatBetween(minValue, maxValue),
        rng.randomFloatBetween(minValue, maxValue), rng.randomFloatBetween(minValue, maxValue));
}

GLColor Vec4ToColor(const Vector4 &vec)
{
    GLColor color;
    color.R = static_cast<uint8_t>(vec.x() * 255.0f);
    color.G = static_cast<uint8_t>(vec.y() * 255.0f);
    color.B = static_cast<uint8_t>(vec.z() * 255.0f);
    color.A = static_cast<uint8_t>(vec.w() * 255.0f);
    return color;
};

class ClearTestBase : public ANGLETest
{
  protected:
    ClearTestBase()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        mFBOs.resize(2, 0);
        glGenFramebuffers(2, mFBOs.data());

        ASSERT_GL_NO_ERROR();
    }

    void TearDown() override
    {
        if (!mFBOs.empty())
        {
            glDeleteFramebuffers(static_cast<GLsizei>(mFBOs.size()), mFBOs.data());
        }

        if (!mTextures.empty())
        {
            glDeleteTextures(static_cast<GLsizei>(mTextures.size()), mTextures.data());
        }

        ANGLETest::TearDown();
    }

    std::vector<GLuint> mFBOs;
    std::vector<GLuint> mTextures;
};

class ClearTest : public ClearTestBase {};
class ClearTestES3 : public ClearTestBase {};

// Test clearing the default framebuffer
TEST_P(ClearTest, DefaultFramebuffer)
{
    glClearColor(0.25f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_PIXEL_NEAR(0, 0, 64, 128, 128, 128, 1.0);
}

// Test clearing a RGBA8 Framebuffer
TEST_P(ClearTest, RGBA8Framebuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLTexture texture;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWindowWidth(), getWindowHeight(), 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

    EXPECT_PIXEL_NEAR(0, 0, 128, 128, 128, 128, 1.0);
}

TEST_P(ClearTest, ClearIssue)
{
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(0.0, 1.0, 0.0, 1.0);
    glClearDepthf(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    EXPECT_GL_NO_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLRenderbuffer rbo;
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB565, 16, 16);

    EXPECT_GL_NO_ERROR();

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);

    EXPECT_GL_NO_ERROR();

    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    EXPECT_GL_NO_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ANGLE_GL_PROGRAM(blueProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(blueProgram, essl1_shaders::PositionAttrib(), 0.5f);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
}

// Requires ES3
// This tests a bug where in a masked clear when calling "ClearBuffer", we would
// mistakenly clear every channel (including the masked-out ones)
TEST_P(ClearTestES3, MaskedClearBufferBug)
{
    unsigned char pixelData[] = { 255, 255, 255, 255 };

    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLTexture textures[2];

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);

    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelData);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textures[1], 0);

    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(0, 0, 255, 255, 255, 255);

    float clearValue[] = { 0, 0.5f, 0.5f, 1.0f };
    GLenum drawBuffers[] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);
    glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
    glClearBufferfv(GL_COLOR, 1, clearValue);

    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(0, 0, 255, 255, 255, 255);

    glReadBuffer(GL_COLOR_ATTACHMENT1);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_NEAR(0, 0, 0, 127, 255, 255, 1);
}

TEST_P(ClearTestES3, BadFBOSerialBug)
{
    // First make a simple framebuffer, and clear it to green
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLTexture textures[2];

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, getWindowWidth(), getWindowHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, drawBuffers);

    float clearValues1[] = { 0.0f, 1.0f, 0.0f, 1.0f };
    glClearBufferfv(GL_COLOR, 0, clearValues1);

    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);

    // Next make a second framebuffer, and draw it to red
    // (Triggers bad applied render target serial)
    GLFramebuffer fbo2;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo2);
    ASSERT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, getWindowWidth(), getWindowHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);

    glDrawBuffers(1, drawBuffers);

    ANGLE_GL_PROGRAM(blueProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(blueProgram, essl1_shaders::PositionAttrib(), 0.5f);

    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    // Check that the first framebuffer is still green.
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
}

// Test that SRGB framebuffers clear to the linearized clear color
TEST_P(ClearTestES3, SRGBClear)
{
    // First make a simple framebuffer, and clear it
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLTexture texture;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_SRGB8_ALPHA8, getWindowWidth(), getWindowHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

    EXPECT_PIXEL_NEAR(0, 0, 188, 188, 188, 128, 1.0);
}

// Test that framebuffers with mixed SRGB/Linear attachments clear to the correct color for each
// attachment
TEST_P(ClearTestES3, MixedSRGBClear)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLTexture textures[2];

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_SRGB8_ALPHA8, getWindowWidth(), getWindowHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);

    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, getWindowWidth(), getWindowHeight());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, textures[1], 0);

    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, drawBuffers);

    // Clear both textures
    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, 0, 0);

    // Check value of texture0
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);
    EXPECT_PIXEL_NEAR(0, 0, 188, 188, 188, 128, 1.0);

    // Check value of texture1
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[1], 0);
    EXPECT_PIXEL_NEAR(0, 0, 128, 128, 128, 128, 1.0);
}

// This test covers a D3D11 bug where calling ClearRenderTargetView sometimes wouldn't sync
// before a draw call. The test draws small quads to a larger FBO (the default back buffer).
// Before each blit to the back buffer it clears the quad to a certain color using
// ClearBufferfv to give a solid color. The sync problem goes away if we insert a call to
// flush or finish after ClearBufferfv or each draw.
TEST_P(ClearTestES3, RepeatedClear)
{
    const std::string &vertexSource =
        "#version 300 es\n"
        "in highp vec2 position;\n"
        "out highp vec2 v_coord;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = vec4(position, 0, 1);\n"
        "    vec2 texCoord = (position * 0.5) + 0.5;\n"
        "    v_coord = texCoord;\n"
        "}\n";

    const std::string &fragmentSource =
        "#version 300 es\n"
        "in highp vec2 v_coord;\n"
        "out highp vec4 color;\n"
        "uniform sampler2D tex;\n"
        "void main()\n"
        "{\n"
        "    color = texture(tex, v_coord);\n"
        "}\n";

    ANGLE_GL_PROGRAM(program, vertexSource, fragmentSource);

    mTextures.resize(1, 0);
    glGenTextures(1, mTextures.data());

    GLenum format           = GL_RGBA8;
    const int numRowsCols   = 3;
    const int cellSize      = 32;
    const int fboSize       = cellSize;
    const int backFBOSize   = cellSize * numRowsCols;
    const float fmtValueMin = 0.0f;
    const float fmtValueMax = 1.0f;

    glBindTexture(GL_TEXTURE_2D, mTextures[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, format, fboSize, fboSize);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextures[0], 0);
    ASSERT_GL_NO_ERROR();

    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));

    // larger fbo bound -- clear to transparent black
    glUseProgram(program);
    GLint uniLoc = glGetUniformLocation(program, "tex");
    ASSERT_NE(-1, uniLoc);
    glUniform1i(uniLoc, 0);
    glBindTexture(GL_TEXTURE_2D, mTextures[0]);

    GLint positionLocation = glGetAttribLocation(program, "position");
    ASSERT_NE(-1, positionLocation);

    glUseProgram(program);

    for (int cellY = 0; cellY < numRowsCols; cellY++)
    {
        for (int cellX = 0; cellX < numRowsCols; cellX++)
        {
            int seed            = cellX + cellY * numRowsCols;
            const Vector4 color = RandomVec4(seed, fmtValueMin, fmtValueMax);

            glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);
            glClearBufferfv(GL_COLOR, 0, color.data());

            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Method 1: Set viewport and draw full-viewport quad
            glViewport(cellX * cellSize, cellY * cellSize, cellSize, cellSize);
            drawQuad(program, "position", 0.5f);

            // Uncommenting the glFinish call seems to make the test pass.
            // glFinish();
        }
    }

    std::vector<GLColor> pixelData(backFBOSize * backFBOSize);
    glReadPixels(0, 0, backFBOSize, backFBOSize, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());

    for (int cellY = 0; cellY < numRowsCols; cellY++)
    {
        for (int cellX = 0; cellX < numRowsCols; cellX++)
        {
            int seed              = cellX + cellY * numRowsCols;
            const Vector4 color   = RandomVec4(seed, fmtValueMin, fmtValueMax);
            GLColor expectedColor = Vec4ToColor(color);

            int testN           = cellX * cellSize + cellY * backFBOSize * cellSize + backFBOSize + 1;
            GLColor actualColor = pixelData[testN];
            EXPECT_NEAR(expectedColor.R, actualColor.R, 1);
            EXPECT_NEAR(expectedColor.G, actualColor.G, 1);
            EXPECT_NEAR(expectedColor.B, actualColor.B, 1);
            EXPECT_NEAR(expectedColor.A, actualColor.A, 1);
        }
    }

    ASSERT_GL_NO_ERROR();
}

class ScissoredClearTest : public ANGLETest
{
  public:
    ScissoredClearTest()
    {
        setWindowWidth(64);
        setWindowHeight(64);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
        setConfigStencilBits(8);
    }
};

// Simple scissored clear.
TEST_P(ScissoredClearTest, BasicScissoredColorClear)
{
    const int w     = getWindowWidth();
    const int h     = getWindowHeight();
    const int whalf = w >> 1;
    const int hhalf = h >> 1;

    // Clear whole region to red.
    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Enable scissor and clear to green.
    glEnable(GL_SCISSOR_TEST);
    glScissor(whalf / 2, hhalf / 2, whalf, whalf);
    glClearColor(0.0, 1.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Check the four corners for the original clear color, and the middle for the scissored clear
    // color.
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(w - 1, 0, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(0, h - 1, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(w - 1, h - 1, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(whalf, hhalf, GLColor::green) << "in-scissor area should be green";
}

// Tests combined scissored color+depth clear.
TEST_P(ScissoredClearTest, ScissoredColorAndDepthClear)
{
    const int w     = getWindowWidth();
    const int h     = getWindowHeight();
    const int whalf = w >> 1;
    const int hhalf = h >> 1;

    // Clear whole region to red/1.0f.
    glClearColor(1.0, 0.0, 0.0, 1.0);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable scissor and clear to green/0.5f.
    glEnable(GL_SCISSOR_TEST);
    glScissor(whalf / 2, hhalf / 2, whalf, whalf);
    glClearColor(0.0, 1.0, 0.0, 1.0);
    glClearDepthf(0.5f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Check the four corners for the original clear color, and the middle for the scissored clear
    // color.
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(w - 1, 0, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(0, h - 1, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(w - 1, h - 1, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(whalf, hhalf, GLColor::green) << "in-scissor area should be green";

    // Draw blue with depth 0.5f and depth test enabled - verify only the middle changes.
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Passthrough(), essl1_shaders::fs::Blue());

    // OpenGL uses a depth range of [-1,1] so pass in a z value of 0 to get 0.5 depth.
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.0f);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(w - 1, 0, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(0, h - 1, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(w - 1, h - 1, GLColor::red) << "out-of-scissor area should be red";
    EXPECT_PIXEL_COLOR_EQ(whalf, hhalf, GLColor::blue) << "in-scissor area should be blue";
}

// Tests combined color+depth clear.
TEST_P(ClearTest, MaskedColorAndDepthClear)
{
    // Flaky on Android Nexus 5x, possible driver bug.
    // TODO(jmadill): Re-enable when possible. http://anglebug.com/2548
    ANGLE_SKIP_TEST_IF(IsOpenGLES() && IsAndroid());

    // Clear to a random color and 1.0 depth.
    Vector4 color1(0.1f, 0.2f, 0.3f, 0.4f);
    GLColor color1RGB(color1);

    glClearColor(color1[0], color1[1], color1[2], color1[3]);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Verify color and was cleared correctly.
    EXPECT_PIXEL_COLOR_NEAR(0, 0, color1RGB, 1);

    // Use a color mask to clear to a second color and 0.5 depth.
    Vector4 color2(0.2f, 0.4f, 0.6f, 0.8f);
    GLColor color2RGB(color2);
    glClearColor(color2[0], color2[1], color2[2], color2[3]);
    glClearDepthf(0.5f);
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    ASSERT_GL_NO_ERROR();

    // Verify second clear mask worked as expected.
    GLColor color2Masked(color2RGB[0], color1RGB[1], color2RGB[2], color1RGB[3]);
    EXPECT_PIXEL_COLOR_EQ(0, 0, color2Masked);

    // We use a small shader to verify depth.
    ANGLE_GL_PROGRAM(depthTestProgram, essl1_shaders::vs::Passthrough(), essl1_shaders::fs::Blue());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    drawQuad(depthTestProgram, essl1_shaders::PositionAttrib(), 0.0f);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::blue);
    ASSERT_GL_NO_ERROR();
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
// Vulkan support disabled because of incomplete implementation.
ANGLE_INSTANTIATE_TEST(ClearTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES2_OPENGL(),
                       ES3_OPENGL(),
                       ES2_OPENGLES(),
                       ES3_OPENGLES(),
                       ES2_VULKAN());
ANGLE_INSTANTIATE_TEST(ClearTestES3, ES3_D3D11(), ES3_OPENGL(), ES3_OPENGLES());
ANGLE_INSTANTIATE_TEST(ScissoredClearTest, ES2_D3D11(), ES2_OPENGL(), ES2_VULKAN());

}  // anonymous namespace
