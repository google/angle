//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"

#include "platform/FeaturesVk.h"
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
        setConfigStencilBits(8);
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

class ClearTest : public ClearTestBase
{
  protected:
    void MaskedScissoredColorDepthStencilClear(bool mask,
                                               bool scissor,
                                               bool clearDepth,
                                               bool clearStencil);

    bool mHasDepth   = true;
    bool mHasStencil = true;
};

class ClearTestES3 : public ClearTestBase
{
};

class ClearTestRGB : public ANGLETest
{
  protected:
    ClearTestRGB()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
    }
};

class ScissoredClearTest : public ClearTest
{
};

class VulkanClearTest : public ClearTest
{
  protected:
    void SetUp() override
    {
        ANGLETest::SetUp();

        glBindTexture(GL_TEXTURE_2D, mColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWindowWidth(), getWindowHeight(), 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);

        // Setup Color/Stencil FBO with a stencil format that's emulated with packed depth/stencil.
        glBindFramebuffer(GL_FRAMEBUFFER, mColorStencilFBO);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorTexture,
                               0);
        glBindRenderbuffer(GL_RENDERBUFFER, mStencilTexture);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, getWindowWidth(),
                              getWindowHeight());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  mStencilTexture);

        ASSERT_GL_NO_ERROR();

        // Note: GL_DEPTH_COMPONENT24 is not allowed in GLES2.
        if (getClientMajorVersion() >= 3)
        {
            // Setup Color/Depth FBO with a depth format that's emulated with packed depth/stencil.
            glBindFramebuffer(GL_FRAMEBUFFER, mColorDepthFBO);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   mColorTexture, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, mDepthTexture);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, getWindowWidth(),
                                  getWindowHeight());
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                      mDepthTexture);
        }

        ASSERT_GL_NO_ERROR();
    }

    void TearDown() override { ANGLETest::TearDown(); }

    void bindColorStencilFBO()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mColorStencilFBO);
        mHasDepth = false;
    }

    void bindColorDepthFBO()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mColorDepthFBO);
        mHasStencil = false;
    }

    // Override a feature to force emulation of stencil-only and depth-only formats with a packed
    // depth/stencil format
    void overrideFeaturesVk(FeaturesVk *featuresVk) override
    {
        featuresVk->forceFallbackFormat = true;
    }

  private:
    GLFramebuffer mColorStencilFBO;
    GLFramebuffer mColorDepthFBO;
    GLTexture mColorTexture;
    GLRenderbuffer mDepthTexture;
    GLRenderbuffer mStencilTexture;
};

// Test clearing the default framebuffer
TEST_P(ClearTest, DefaultFramebuffer)
{
    glClearColor(0.25f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_PIXEL_NEAR(0, 0, 64, 128, 128, 128, 1.0);
}

// Test clearing the RGB default framebuffer and verify that the alpha channel is not cleared
TEST_P(ClearTestRGB, DefaultFramebufferRGB)
{
    glClearColor(0.25f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_PIXEL_NEAR(0, 0, 64, 128, 128, 255, 1.0);
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

// Test to validate that we can go from an RGBA framebuffer attachment, to an RGB one and still
// have a correct behavior after.
TEST_P(ClearTest, ChangeFramebufferAttachmentFromRGBAtoRGB)
{
    // http://anglebug.com/2689
    ANGLE_SKIP_TEST_IF(IsD3D9() || IsD3D11() || (IsOzone() && IsOpenGLES()));
    ANGLE_SKIP_TEST_IF(IsOSX() && (IsNVIDIA() || IsIntel()) && IsDesktopOpenGL());
    ANGLE_SKIP_TEST_IF(IsAndroid() && IsAdreno() && IsOpenGLES());

    ANGLE_GL_PROGRAM(program, angle::essl1_shaders::vs::Simple(),
                     angle::essl1_shaders::fs::UniformColor());
    setupQuadVertexBuffer(0.5f, 1.0f);
    glUseProgram(program);
    GLint positionLocation = glGetAttribLocation(program, angle::essl1_shaders::PositionAttrib());
    ASSERT_NE(positionLocation, -1);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(positionLocation);

    GLint colorUniformLocation =
        glGetUniformLocation(program, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 1.0f, 1.0f, 1.0f, 0.5f);

    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLTexture texture;
    glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWindowWidth(), getWindowHeight(), 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // So far so good, we have an RGBA framebuffer that we've cleared to 0.5 everywhere.
    EXPECT_PIXEL_NEAR(0, 0, 128, 0, 128, 128, 1.0);

    // In the Vulkan backend, RGB textures are emulated with an RGBA texture format
    // underneath and we keep a special mask to know that we shouldn't touch the alpha
    // channel when we have that emulated texture. This test exists to validate that
    // this mask gets updated correctly when the framebuffer attachment changes.
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getWindowWidth(), getWindowHeight(), 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    ASSERT_GL_NO_ERROR();

    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_RECT_EQ(0, 0, getWindowWidth(), getWindowHeight(), GLColor::magenta);
}

// Test clearing a RGB8 Framebuffer with a color mask.
TEST_P(ClearTest, RGB8WithMaskFramebuffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, mFBOs[0]);

    GLTexture texture;

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getWindowWidth(), getWindowHeight(), 0, GL_RGB,
                 GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

    glClearColor(0.2f, 0.4f, 0.6f, 0.8f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Since there's no alpha, we expect to get 255 back instead of the clear value (204).
    EXPECT_PIXEL_NEAR(0, 0, 51, 102, 153, 255, 1.0);

    glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
    glClearColor(0.1f, 0.3f, 0.5f, 0.7f);
    glClear(GL_COLOR_BUFFER_BIT);

    // The blue channel was masked so its value should be unchanged.
    EXPECT_PIXEL_NEAR(0, 0, 26, 77, 153, 255, 1.0);

    // Restore default.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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

// Regression test for a bug where "glClearDepthf"'s argument was not clamped
// In GLES 2 they where declared as GLclampf and the behaviour is the same in GLES 3.2
TEST_P(ClearTest, ClearIsClamped)
{
    glClearDepthf(5.0f);

    GLfloat clear_depth;
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clear_depth);
    EXPECT_EQ(1.0f, clear_depth);
}

// Regression test for a bug where "glDepthRangef"'s arguments were not clamped
// In GLES 2 they where declared as GLclampf and the behaviour is the same in GLES 3.2
TEST_P(ClearTest, DepthRangefIsClamped)
{
    glDepthRangef(1.1f, -4.0f);

    GLfloat depth_range[2];
    glGetFloatv(GL_DEPTH_RANGE, depth_range);
    EXPECT_EQ(1.0f, depth_range[0]);
    EXPECT_EQ(0.0f, depth_range[1]);
}

// Requires ES3
// This tests a bug where in a masked clear when calling "ClearBuffer", we would
// mistakenly clear every channel (including the masked-out ones)
TEST_P(ClearTestES3, MaskedClearBufferBug)
{
    unsigned char pixelData[] = {255, 255, 255, 255};

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

    float clearValue[]   = {0, 0.5f, 0.5f, 1.0f};
    GLenum drawBuffers[] = {GL_NONE, GL_COLOR_ATTACHMENT1};
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

    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers);

    float clearValues1[] = {0.0f, 1.0f, 0.0f, 1.0f};
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

            int testN = cellX * cellSize + cellY * backFBOSize * cellSize + backFBOSize + 1;
            GLColor actualColor = pixelData[testN];
            EXPECT_NEAR(expectedColor.R, actualColor.R, 1);
            EXPECT_NEAR(expectedColor.G, actualColor.G, 1);
            EXPECT_NEAR(expectedColor.B, actualColor.B, 1);
            EXPECT_NEAR(expectedColor.A, actualColor.A, 1);
        }
    }

    ASSERT_GL_NO_ERROR();
}

void ClearTest::MaskedScissoredColorDepthStencilClear(bool mask,
                                                      bool scissor,
                                                      bool clearDepth,
                                                      bool clearStencil)
{
    // Flaky on Android Nexus 5x, possible driver bug.
    // TODO(jmadill): Re-enable when possible. http://anglebug.com/2548
    ANGLE_SKIP_TEST_IF(IsOpenGLES() && IsAndroid());

    const int w     = getWindowWidth();
    const int h     = getWindowHeight();
    const int whalf = w >> 1;
    const int hhalf = h >> 1;

    // Clear to a random color, 0.9 depth and 0x00 stencil
    Vector4 color1(0.1f, 0.2f, 0.3f, 0.4f);
    GLColor color1RGB(color1);

    glClearColor(color1[0], color1[1], color1[2], color1[3]);
    glClearDepthf(0.9f);
    glClearStencil(0x00);
    glClear(GL_COLOR_BUFFER_BIT | (clearDepth ? GL_DEPTH_BUFFER_BIT : 0) |
            (clearStencil ? GL_STENCIL_BUFFER_BIT : 0));
    ASSERT_GL_NO_ERROR();

    // Verify color was cleared correctly.
    EXPECT_PIXEL_COLOR_NEAR(0, 0, color1RGB, 1);

    if (scissor)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(whalf / 2, hhalf / 2, whalf, hhalf);
    }

    // Use color and stencil masks to clear to a second color, 0.5 depth and 0x59 stencil.
    Vector4 color2(0.2f, 0.4f, 0.6f, 0.8f);
    GLColor color2RGB(color2);
    glClearColor(color2[0], color2[1], color2[2], color2[3]);
    glClearDepthf(0.5f);
    glClearStencil(0xFF);
    if (mask)
    {
        glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
        glDepthMask(GL_FALSE);
        glStencilMask(0x59);
    }
    glClear(GL_COLOR_BUFFER_BIT | (clearDepth ? GL_DEPTH_BUFFER_BIT : 0) |
            (clearStencil ? GL_STENCIL_BUFFER_BIT : 0));
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0xFF);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_SCISSOR_TEST);
    ASSERT_GL_NO_ERROR();

    // Verify second clear mask worked as expected.
    GLColor color2MaskedRGB(color2RGB[0], color1RGB[1], color2RGB[2], color1RGB[3]);

    GLColor expectedCenterColorRGB = mask ? color2MaskedRGB : color2RGB;
    GLColor expectedCornerColorRGB = scissor ? color1RGB : expectedCenterColorRGB;

    EXPECT_PIXEL_COLOR_NEAR(whalf, hhalf, expectedCenterColorRGB, 1);

    EXPECT_PIXEL_COLOR_NEAR(0, 0, expectedCornerColorRGB, 1);
    EXPECT_PIXEL_COLOR_NEAR(w - 1, 0, expectedCornerColorRGB, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, h - 1, expectedCornerColorRGB, 1);
    EXPECT_PIXEL_COLOR_NEAR(w - 1, h - 1, expectedCornerColorRGB, 1);

    // If there is depth, but depth is not asked to be cleared, the depth buffer contains garbage,
    // so no particular behavior can be expected.
    if (clearDepth || !mHasDepth)
    {
        // We use a small shader to verify depth.
        ANGLE_GL_PROGRAM(depthTestProgram, essl1_shaders::vs::Passthrough(),
                         essl1_shaders::fs::Blue());
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(mask ? GL_GREATER : GL_EQUAL);
        // - If depth is cleared, but it's masked, 0.9 should be in the depth buffer.
        // - If depth is cleared, but it's not masked, 0.5 should be in the depth buffer.
        // - If depth is not cleared, the if above ensures there is no depth buffer at all,
        //   which means depth test will always pass.
        drawQuad(depthTestProgram, essl1_shaders::PositionAttrib(), mask ? 1.0f : 0.0f);
        glDisable(GL_DEPTH_TEST);
        ASSERT_GL_NO_ERROR();

        // Either way, we expect blue to be written to the center.
        expectedCenterColorRGB = GLColor::blue;
        // If there is no depth, depth test always passes so the whole image must be blue.  Same if
        // depth write is masked.
        expectedCornerColorRGB =
            mHasDepth && scissor && !mask ? expectedCornerColorRGB : GLColor::blue;

        EXPECT_PIXEL_COLOR_NEAR(whalf, hhalf, expectedCenterColorRGB, 1);

        EXPECT_PIXEL_COLOR_NEAR(0, 0, expectedCornerColorRGB, 1);
        EXPECT_PIXEL_COLOR_NEAR(w - 1, 0, expectedCornerColorRGB, 1);
        EXPECT_PIXEL_COLOR_NEAR(0, h - 1, expectedCornerColorRGB, 1);
        EXPECT_PIXEL_COLOR_NEAR(w - 1, h - 1, expectedCornerColorRGB, 1);
    }

    // If there is stencil, but it's not asked to be cleared, there is similarly no expectation.
    if (clearStencil || !mHasStencil)
    {
        // And another small shader to verify stencil.
        ANGLE_GL_PROGRAM(stencilTestProgram, essl1_shaders::vs::Passthrough(),
                         essl1_shaders::fs::Green());
        glEnable(GL_STENCIL_TEST);
        // - If stencil is cleared, but it's masked, 0x59 should be in the stencil buffer.
        // - If stencil is cleared, but it's not masked, 0xFF should be in the stencil buffer.
        // - If stencil is not cleared, the if above ensures there is no stencil buffer at all,
        //   which means stencil test will always pass.
        glStencilFunc(GL_EQUAL, mask ? 0x59 : 0xFF, 0xFF);
        drawQuad(stencilTestProgram, essl1_shaders::PositionAttrib(), 0.0f);
        glDisable(GL_STENCIL_TEST);
        ASSERT_GL_NO_ERROR();

        // Either way, we expect green to be written to the center.
        expectedCenterColorRGB = GLColor::green;
        // If there is no stencil, stencil test always passes so the whole image must be green.
        expectedCornerColorRGB = mHasStencil && scissor ? expectedCornerColorRGB : GLColor::green;

        EXPECT_PIXEL_COLOR_NEAR(whalf, hhalf, expectedCenterColorRGB, 1);

        EXPECT_PIXEL_COLOR_NEAR(0, 0, expectedCornerColorRGB, 1);
        EXPECT_PIXEL_COLOR_NEAR(w - 1, 0, expectedCornerColorRGB, 1);
        EXPECT_PIXEL_COLOR_NEAR(0, h - 1, expectedCornerColorRGB, 1);
        EXPECT_PIXEL_COLOR_NEAR(w - 1, h - 1, expectedCornerColorRGB, 1);
    }
}

// Tests combined color+depth+stencil clears.
TEST_P(ClearTest, MaskedColorAndDepthClear)
{
    MaskedScissoredColorDepthStencilClear(true, false, true, false);
}

TEST_P(ClearTest, MaskedColorAndStencilClear)
{
    MaskedScissoredColorDepthStencilClear(true, false, false, true);
}

TEST_P(ClearTest, MaskedColorAndDepthAndStencilClear)
{
    MaskedScissoredColorDepthStencilClear(true, false, true, true);
}

// Simple scissored clear.
TEST_P(ScissoredClearTest, BasicScissoredColorClear)
{
    MaskedScissoredColorDepthStencilClear(false, true, false, false);
}

// Simple scissored masked clear.
TEST_P(ScissoredClearTest, MaskedScissoredColorClear)
{
    MaskedScissoredColorDepthStencilClear(true, true, false, false);
}

// Tests combined color+depth+stencil scissored clears.
TEST_P(ScissoredClearTest, ScissoredColorAndDepthClear)
{
    MaskedScissoredColorDepthStencilClear(false, true, true, false);
}

TEST_P(ScissoredClearTest, ScissoredColorAndStencilClear)
{
    MaskedScissoredColorDepthStencilClear(false, true, false, true);
}

TEST_P(ScissoredClearTest, ScissoredColorAndDepthAndStencilClear)
{
    MaskedScissoredColorDepthStencilClear(false, true, true, true);
}

// Tests combined color+depth+stencil scissored masked clears.
TEST_P(ScissoredClearTest, MaskedScissoredColorAndDepthClear)
{
    MaskedScissoredColorDepthStencilClear(true, true, true, false);
}

TEST_P(ScissoredClearTest, MaskedScissoredColorAndStencilClear)
{
    MaskedScissoredColorDepthStencilClear(true, true, false, true);
}

TEST_P(ScissoredClearTest, MaskedScissoredColorAndDepthAndStencilClear)
{
    MaskedScissoredColorDepthStencilClear(true, true, true, true);
}

// Tests combined color+stencil scissored masked clears for a depth-stencil-emulated
// stencil-only-type.
TEST_P(VulkanClearTest, ColorAndStencilClear)
{
    bindColorStencilFBO();
    MaskedScissoredColorDepthStencilClear(false, false, false, true);
}

TEST_P(VulkanClearTest, MaskedColorAndStencilClear)
{
    bindColorStencilFBO();
    MaskedScissoredColorDepthStencilClear(true, false, false, true);
}

TEST_P(VulkanClearTest, ScissoredColorAndStencilClear)
{
    bindColorStencilFBO();
    MaskedScissoredColorDepthStencilClear(false, true, false, true);
}

TEST_P(VulkanClearTest, MaskedScissoredColorAndStencilClear)
{
    bindColorStencilFBO();
    MaskedScissoredColorDepthStencilClear(true, true, false, true);
}

// Tests combined color+depth scissored masked clears for a depth-stencil-emulated
// depth-only-type.
TEST_P(VulkanClearTest, ColorAndDepthClear)
{
    ANGLE_SKIP_TEST_IF(getClientMajorVersion() < 3);
    bindColorDepthFBO();
    MaskedScissoredColorDepthStencilClear(false, false, true, false);
}

TEST_P(VulkanClearTest, MaskedColorAndDepthClear)
{
    ANGLE_SKIP_TEST_IF(getClientMajorVersion() < 3);
    bindColorDepthFBO();
    MaskedScissoredColorDepthStencilClear(true, false, true, false);
}

TEST_P(VulkanClearTest, ScissoredColorAndDepthClear)
{
    ANGLE_SKIP_TEST_IF(getClientMajorVersion() < 3);
    bindColorDepthFBO();
    MaskedScissoredColorDepthStencilClear(false, true, true, false);
}

TEST_P(VulkanClearTest, MaskedScissoredColorAndDepthClear)
{
    ANGLE_SKIP_TEST_IF(getClientMajorVersion() < 3);
    bindColorDepthFBO();
    MaskedScissoredColorDepthStencilClear(true, true, true, false);
}

// Test that just clearing a nonexistent drawbuffer of the default framebuffer doesn't cause an
// assert.
TEST_P(ClearTestES3, ClearBuffer1OnDefaultFramebufferNoAssert)
{
    std::vector<GLuint> testUint(4);
    glClearBufferuiv(GL_COLOR, 1, testUint.data());
    std::vector<GLint> testInt(4);
    glClearBufferiv(GL_COLOR, 1, testInt.data());
    std::vector<GLfloat> testFloat(4);
    glClearBufferfv(GL_COLOR, 1, testFloat.data());
    EXPECT_GL_NO_ERROR();
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against. Vulkan support disabled because of incomplete implementation.
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
ANGLE_INSTANTIATE_TEST(VulkanClearTest, ES2_VULKAN());

// Not all ANGLE backends support RGB backbuffers
ANGLE_INSTANTIATE_TEST(ClearTestRGB, ES2_D3D11(), ES3_D3D11(), ES2_VULKAN());

}  // anonymous namespace
