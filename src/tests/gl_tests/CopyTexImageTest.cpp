//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

namespace angle
{

class CopyTexImageTest : public ANGLETest
{
  protected:
    CopyTexImageTest()
    {
        setWindowWidth(32);
        setWindowHeight(32);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        constexpr char kVS[] =
            "precision highp float;\n"
            "attribute vec4 position;\n"
            "varying vec2 texcoord;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    gl_Position = position;\n"
            "    texcoord = (position.xy * 0.5) + 0.5;\n"
            "}\n";

        constexpr char kFS[] =
            "precision highp float;\n"
            "uniform sampler2D tex;\n"
            "varying vec2 texcoord;\n"
            "\n"
            "void main()\n"
            "{\n"
            "    gl_FragColor = texture2D(tex, texcoord);\n"
            "}\n";

        mTextureProgram = CompileProgram(kVS, kFS);
        if (mTextureProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        mTextureUniformLocation = glGetUniformLocation(mTextureProgram, "tex");

        ASSERT_GL_NO_ERROR();
    }

    void TearDown() override
    {
        glDeleteProgram(mTextureProgram);

        ANGLETest::TearDown();
    }

    void initializeResources(GLenum format, GLenum type)
    {
        for (size_t i = 0; i < kFboCount; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, mFboTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, format, kFboSizes[i], kFboSizes[i], 0, format, type,
                         nullptr);

            // Disable mipmapping
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glBindFramebuffer(GL_FRAMEBUFFER, mFbos[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   mFboTextures[i], 0);

            glClearColor(kFboColors[i][0], kFboColors[i][1], kFboColors[i][2], kFboColors[i][3]);
            glClear(GL_COLOR_BUFFER_BIT);
        }

        ASSERT_GL_NO_ERROR();
    }

    void verifyResults(GLuint texture,
                       GLubyte data[4],
                       GLint fboSize,
                       GLint xs,
                       GLint ys,
                       GLint xe,
                       GLint ye)
    {
        glViewport(0, 0, fboSize, fboSize);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Draw a quad with the target texture
        glUseProgram(mTextureProgram);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(mTextureUniformLocation, 0);

        drawQuad(mTextureProgram, "position", 0.5f);

        // Expect that the rendered quad has the same color as the source texture
        EXPECT_PIXEL_NEAR(xs, ys, data[0], data[1], data[2], data[3], 1.0);
        EXPECT_PIXEL_NEAR(xs, ye - 1, data[0], data[1], data[2], data[3], 1.0);
        EXPECT_PIXEL_NEAR(xe - 1, ys, data[0], data[1], data[2], data[3], 1.0);
        EXPECT_PIXEL_NEAR(xe - 1, ye - 1, data[0], data[1], data[2], data[3], 1.0);
        EXPECT_PIXEL_NEAR((xs + xe) / 2, (ys + ye) / 2, data[0], data[1], data[2], data[3], 1.0);
    }

    void runCopyTexImageTest(GLenum format, GLubyte expected[3][4])
    {
        GLTexture tex;
        glBindTexture(GL_TEXTURE_2D, tex);

        // Disable mipmapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Perform the copy multiple times.
        //
        // - The first time, a new texture is created
        // - The second time, as the fbo size is the same as previous, the texture storage is not
        //   recreated.
        // - The third time, the fbo size is different, so a new texture is created.
        for (size_t i = 0; i < kFboCount; ++i)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, mFbos[i]);

            glCopyTexImage2D(GL_TEXTURE_2D, 0, format, 0, 0, kFboSizes[i], kFboSizes[i], 0);
            ASSERT_GL_NO_ERROR();

            verifyResults(tex, expected[i], kFboSizes[i], 0, 0, kFboSizes[i], kFboSizes[i]);
        }
    }

    void runCopyTexSubImageTest(GLenum format, GLubyte expected[3][4])
    {
        GLTexture tex;
        glBindTexture(GL_TEXTURE_2D, tex);

        // Disable mipmapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        // Create the texture with copy of the first fbo.
        glBindFramebuffer(GL_FRAMEBUFFER, mFbos[0]);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, format, 0, 0, kFboSizes[0], kFboSizes[0], 0);
        ASSERT_GL_NO_ERROR();

        verifyResults(tex, expected[0], kFboSizes[0], 0, 0, kFboSizes[0], kFboSizes[0]);

        // Make sure out-of-bound writes to the texture return invalid value.
        glBindFramebuffer(GL_FRAMEBUFFER, mFbos[1]);

        // xoffset < 0 and yoffset < 0
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, -1, -1, 0, 0, kFboSizes[0], kFboSizes[0]);
        ASSERT_GL_ERROR(GL_INVALID_VALUE);

        // xoffset + width > w and yoffset + height > h
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 0, 0, kFboSizes[0], kFboSizes[0]);
        ASSERT_GL_ERROR(GL_INVALID_VALUE);

        // Copy the second fbo over a portion of the image.
        GLint offset = kFboSizes[0] / 2;
        GLint extent = kFboSizes[0] - offset;

        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, offset, offset, kFboSizes[1] / 2, kFboSizes[1] / 2,
                            extent, extent);
        ASSERT_GL_NO_ERROR();

        verifyResults(tex, expected[1], kFboSizes[0], offset, offset, kFboSizes[0], kFboSizes[0]);

        // The rest of the image should be untouched
        verifyResults(tex, expected[0], kFboSizes[0], 0, 0, offset, offset);
        verifyResults(tex, expected[0], kFboSizes[0], offset, 0, kFboSizes[0], offset);
        verifyResults(tex, expected[0], kFboSizes[0], 0, offset, offset, kFboSizes[0]);

        // Copy the third fbo over another portion of the image.
        glBindFramebuffer(GL_FRAMEBUFFER, mFbos[2]);

        offset = kFboSizes[0] / 4;
        extent = kFboSizes[0] - offset;

        // While width and height are set as 3/4 of the size, the fbo offset is given such that
        // after clipping, width and height are effectively 1/2 of the size.
        GLint srcOffset       = kFboSizes[2] - kFboSizes[0] / 2;
        GLint effectiveExtent = kFboSizes[0] / 2;

        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, offset, offset, srcOffset, srcOffset, extent, extent);
        ASSERT_GL_NO_ERROR();

        verifyResults(tex, expected[2], kFboSizes[0], offset, offset, effectiveExtent,
                      effectiveExtent);

        // The rest of the image should be untouched
        verifyResults(tex, expected[1], kFboSizes[0], offset + effectiveExtent, kFboSizes[0] / 2,
                      kFboSizes[0], kFboSizes[0]);
        verifyResults(tex, expected[1], kFboSizes[0], kFboSizes[0] / 2, offset + effectiveExtent,
                      kFboSizes[0], kFboSizes[0]);

        verifyResults(tex, expected[0], kFboSizes[0], 0, 0, kFboSizes[0], offset);
        verifyResults(tex, expected[0], kFboSizes[0], 0, 0, offset, kFboSizes[0]);
        verifyResults(tex, expected[0], kFboSizes[0], offset + effectiveExtent, 0, kFboSizes[0],
                      kFboSizes[0] / 2);
        verifyResults(tex, expected[0], kFboSizes[0], 0, offset + effectiveExtent, kFboSizes[0] / 2,
                      kFboSizes[0]);
    }

    GLuint mTextureProgram;
    GLint mTextureUniformLocation;

    static constexpr uint32_t kFboCount = 3;
    GLFramebuffer mFbos[kFboCount];
    GLTexture mFboTextures[kFboCount];

    static constexpr uint32_t kFboSizes[kFboCount]    = {16, 16, 32};
    static constexpr GLfloat kFboColors[kFboCount][4] = {{0.25f, 1.0f, 0.75f, 0.5f},
                                                         {1.0f, 0.75f, 0.5f, 0.25f},
                                                         {0.5f, 0.25f, 1.0f, 0.75f}};
};

// Until C++17, need to redundantly declare the constexpr members outside the class (only the
// arrays, because the others are already const-propagated and not needed by the linker).
constexpr uint32_t CopyTexImageTest::kFboSizes[];
constexpr GLfloat CopyTexImageTest::kFboColors[][4];

TEST_P(CopyTexImageTest, RGBAToRGB)
{
    GLubyte expected[3][4] = {
        {64, 255, 191, 255},
        {255, 191, 127, 255},
        {127, 64, 255, 255},
    };

    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);
    runCopyTexImageTest(GL_RGB, expected);
}

TEST_P(CopyTexImageTest, RGBAToL)
{
    GLubyte expected[3][4] = {
        {64, 64, 64, 255},
        {255, 255, 255, 255},
        {127, 127, 127, 255},
    };

    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);
    runCopyTexImageTest(GL_LUMINANCE, expected);
}

TEST_P(CopyTexImageTest, RGBToL)
{
    GLubyte expected[3][4] = {
        {64, 64, 64, 255},
        {255, 255, 255, 255},
        {127, 127, 127, 255},
    };

    initializeResources(GL_RGB, GL_UNSIGNED_BYTE);
    runCopyTexImageTest(GL_LUMINANCE, expected);
}

TEST_P(CopyTexImageTest, RGBAToLA)
{
    GLubyte expected[3][4] = {
        {64, 64, 64, 127},
        {255, 255, 255, 64},
        {127, 127, 127, 191},
    };

    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);
    runCopyTexImageTest(GL_LUMINANCE_ALPHA, expected);
}

TEST_P(CopyTexImageTest, RGBAToA)
{
    GLubyte expected[3][4] = {
        {0, 0, 0, 127},
        {0, 0, 0, 64},
        {0, 0, 0, 191},
    };

    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);
    runCopyTexImageTest(GL_ALPHA, expected);
}

TEST_P(CopyTexImageTest, SubImageRGBAToRGB)
{
    GLubyte expected[3][4] = {
        {64, 255, 191, 255},
        {255, 191, 127, 255},
        {127, 64, 255, 255},
    };

    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);
    runCopyTexSubImageTest(GL_RGB, expected);
}

TEST_P(CopyTexImageTest, SubImageRGBAToL)
{
    GLubyte expected[3][4] = {
        {64, 64, 64, 255},
        {255, 255, 255, 255},
        {127, 127, 127, 255},
    };

    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);
    runCopyTexSubImageTest(GL_LUMINANCE, expected);
}

TEST_P(CopyTexImageTest, SubImageRGBAToLA)
{
    GLubyte expected[3][4] = {
        {64, 64, 64, 127},
        {255, 255, 255, 64},
        {127, 127, 127, 191},
    };

    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);
    runCopyTexSubImageTest(GL_LUMINANCE_ALPHA, expected);
}

TEST_P(CopyTexImageTest, SubImageRGBToL)
{
    GLubyte expected[3][4] = {
        {64, 64, 64, 255},
        {255, 255, 255, 255},
        {127, 127, 127, 255},
    };

    initializeResources(GL_RGB, GL_UNSIGNED_BYTE);
    runCopyTexSubImageTest(GL_LUMINANCE, expected);
}

// Read default framebuffer with glCopyTexImage2D().
TEST_P(CopyTexImageTest, DefaultFramebuffer)
{
    // Seems to be a bug in Mesa with the GLX back end: cannot read framebuffer until we draw to it.
    // glCopyTexImage2D() below will fail without this clear.
    glClear(GL_COLOR_BUFFER_BIT);

    const GLint w = getWindowWidth(), h = getWindowHeight();
    GLTexture tex;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, w, h, 0);
    EXPECT_GL_NO_ERROR();
}

// Read default framebuffer with glCopyTexSubImage2D().
TEST_P(CopyTexImageTest, SubDefaultFramebuffer)
{
    // Seems to be a bug in Mesa with the GLX back end: cannot read framebuffer until we draw to it.
    // glCopyTexSubImage2D() below will fail without this clear.
    glClear(GL_COLOR_BUFFER_BIT);

    const GLint w = getWindowWidth(), h = getWindowHeight();
    GLTexture tex;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
    EXPECT_GL_NO_ERROR();
}

// specialization of CopyTexImageTest is added so that some tests can be explicitly run with an ES3
// context
class CopyTexImageTestES3 : public CopyTexImageTest
{};

//  The test verifies that glCopyTexSubImage2D generates a GL_INVALID_OPERATION error
//  when the read buffer is GL_NONE.
//  Reference: GLES 3.0.4, Section 3.8.5 Alternate Texture Image Specification Commands
TEST_P(CopyTexImageTestES3, ReadBufferIsNone)
{
    initializeResources(GL_RGBA, GL_UNSIGNED_BYTE);

    GLTexture tex;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, mFbos[0]);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kFboSizes[0], kFboSizes[0], 0);

    glReadBuffer(GL_NONE);

    EXPECT_GL_NO_ERROR();
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 4, 4);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST(CopyTexImageTest,
                       ES2_D3D9(),
                       ES2_D3D11(EGL_EXPERIMENTAL_PRESENT_PATH_COPY_ANGLE),
                       ES2_D3D11(EGL_EXPERIMENTAL_PRESENT_PATH_FAST_ANGLE),
                       ES2_OPENGL(),
                       ES2_OPENGL(3, 3),
                       ES2_OPENGLES(),
                       ES2_VULKAN());

ANGLE_INSTANTIATE_TEST(CopyTexImageTestES3, ES3_D3D11(), ES3_OPENGL(), ES3_OPENGLES());
}  // namespace angle
