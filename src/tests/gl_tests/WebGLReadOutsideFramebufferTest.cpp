//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WebGLReadOutsideFramebufferTest.cpp : Test functions which read the framebuffer (readPixels,
// copyTexSubImage2D, copyTexImage2D) on areas outside the framebuffer.

#include "test_utils/ANGLETest.h"

#include "test_utils/gl_raii.h"

namespace
{

class PixelRect
{
  public:
    PixelRect(int width, int height) : mWidth(width), mHeight(height), mData(width * height) {}

    // Set each pixel to a different color consisting of the x,y position and a given tag.
    // Making each pixel a different means any misplaced pixel will cause a failure.
    // Encoding the position proved valuable in debugging.
    void fill(unsigned tag)
    {
        for (int x = 0; x < mWidth; ++x)
        {
            for (int y = 0; y < mHeight; ++y)
            {
                mData[x + y * mWidth] = angle::GLColor(x + (y << 8) + (tag << 16));
            }
        }
    }

    void setPixel(GLubyte x, GLubyte y, GLubyte z, GLubyte w)
    {
        mData[x + y * mWidth] = angle::GLColor(x, y, z, w);
    }

    void toTexture2D(GLuint texid) const
    {
        glBindTexture(GL_TEXTURE_2D, texid);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWidth, mHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                     mData.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    void readFB(int x, int y)
    {
        glReadPixels(x, y, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, mData.data());
    }

    // Read pixels from 'other' into 'this' from position (x,y).
    // Pixels outside 'other' are untouched or zeroed according to 'zeroOutside.'
    void readPixelRect(const PixelRect &other, int x, int y, bool zeroOutside)
    {
        for (int i = 0; i < mWidth; ++i)
        {
            for (int j = 0; j < mHeight; ++j)
            {
                angle::GLColor *dest = &mData[i + j * mWidth];
                if (!other.getPixel(x + i, y + j, dest) && zeroOutside)
                {
                    *dest = angle::GLColor(0);
                }
            }
        }
    }

    bool getPixel(int x, int y, angle::GLColor *colorOut) const
    {
        if (0 <= x && x < mWidth && 0 <= y && y < mHeight)
        {
            *colorOut = mData[x + y * mWidth];
            return true;
        }
        return false;
    }

    void compare(const PixelRect &expected) const
    {
        for (int i = 0; i < mWidth * mHeight; ++i)
            ASSERT_EQ(expected.mData[i], mData[i]);
    }

  private:
    int mWidth, mHeight;
    std::vector<angle::GLColor> mData;
};

}  // namespace

namespace angle
{

class WebGLReadOutsideFramebufferTest : public ANGLETest
{
  public:
    // Read framebuffer to 'pixelsOut' via glReadPixels.
    void TestReadPixels(int x, int y, PixelRect *pixelsOut) { pixelsOut->readFB(x, y); }

    // Read framebuffer to 'pixelsOut' via glCopyTexSubImage2D.
    void TestCopyTexSubImage2D(int x, int y, PixelRect *pixelsOut)
    {
        // Init texture with given pixels.
        GLTexture destTexture;
        pixelsOut->toTexture2D(destTexture.get());

        // Read framebuffer -> texture -> 'pixelsOut'
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, x, y, kReadWidth, kReadHeight);
        readTexture(kReadWidth, kReadHeight, pixelsOut);
    }

    // Read framebuffer to 'pixelsOut' via glCopyTexImage2D.
    void TestCopyTexImage2D(int x, int y, PixelRect *pixelsOut)
    {
        // Init texture with given pixels.
        GLTexture destTexture;
        pixelsOut->toTexture2D(destTexture.get());

        // Read framebuffer -> texture -> 'pixelsOut'
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, kReadWidth, kReadHeight, 0);
        readTexture(kReadWidth, kReadHeight, pixelsOut);
    }

  protected:
    static constexpr int kFbWidth    = 128;
    static constexpr int kFbHeight   = 128;
    static constexpr int kReadWidth  = 4;
    static constexpr int kReadHeight = 4;

    // Tag the framebuffer pixels differently than the initial read buffer pixels, so we know for
    // sure which pixels are changed by reading.
    static constexpr GLuint fbTag   = 0x1122;
    static constexpr GLuint readTag = 0xaabb;

    WebGLReadOutsideFramebufferTest() : mFBData(kFbWidth, kFbHeight)
    {
        setWindowWidth(kFbWidth);
        setWindowHeight(kFbHeight);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setWebGLCompatibilityEnabled(true);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        // TODO(fjhenigman): Factor out this shader and others like it in other tests, into
        // ANGLETest.
        const std::string vertexShader =
            "attribute vec3 a_position;\n"
            "varying vec2 v_texCoord;\n"
            "void main() {\n"
            "    v_texCoord = a_position.xy * 0.5 + 0.5;\n"
            "    gl_Position = vec4(a_position, 1);\n"
            "}\n";
        const std::string fragmentShader =
            "precision mediump float;\n"
            "varying vec2 v_texCoord;\n"
            "uniform sampler2D u_texture;\n"
            "void main() {\n"
            "    gl_FragColor = texture2D(u_texture, v_texCoord);\n"
            "}\n";

        mProgram = CompileProgram(vertexShader, fragmentShader);
        glUseProgram(mProgram);
        GLint uniformLoc = glGetUniformLocation(mProgram, "u_texture");
        ASSERT_NE(-1, uniformLoc);
        glUniform1i(uniformLoc, 0);

        glDisable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);

        // fill framebuffer with unique pixels
        mFBData.fill(fbTag);
        GLTexture fbTexture;
        mFBData.toTexture2D(fbTexture.get());
        drawQuad(mProgram, "a_position", 0.0f, 1.0f, true);
    }

    void TearDown() override
    {
        glDeleteProgram(mProgram);
        ANGLETest::TearDown();
    }

    using TestFunc = void (WebGLReadOutsideFramebufferTest::*)(int x, int y, PixelRect *dest);

    void Main(TestFunc testFunc, bool zeroOutside)
    {
        PixelRect actual(kReadWidth, kReadHeight);
        PixelRect expected(kReadWidth, kReadHeight);

        // Read a kReadWidth*kReadHeight rectangle of pixels from places that include:
        // - completely outside framebuffer, on all sides of it (i,j < 0 or > 2)
        // - completely inside framebuffer (i,j == 1)
        // - straddling framebuffer boundary, at each corner and side
        for (int i = -1; i < 4; ++i)
        {
            for (int j = -1; j < 4; ++j)
            {
                int x = i * kFbWidth / 2 - kReadWidth / 2;
                int y = j * kFbHeight / 2 - kReadHeight / 2;

                // Put unique pixel values into the read destinations.
                actual.fill(readTag);
                expected.readPixelRect(actual, 0, 0, false);

                // Read from framebuffer into 'actual.'
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                (this->*testFunc)(x, y, &actual);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

                // Simulate framebuffer read, into 'expected.'
                expected.readPixelRect(mFBData, x, y, zeroOutside);

                // See if they are the same.
                actual.compare(expected);
            }
        }
    }

    // Get contents of current texture by drawing it into a framebuffer then reading with
    // glReadPixels().
    void readTexture(GLsizei width, GLsizei height, PixelRect *out)
    {
        GLRenderbuffer colorBuffer;
        glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);

        GLFramebuffer fbo;
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                  colorBuffer.get());

        glViewport(0, 0, width, height);
        drawQuad(mProgram, "a_position", 0.0f, 1.0f, true);
        out->readFB(0, 0);
    }

    PixelRect mFBData;
    GLuint mProgram;
};

// TODO(fjhenigman): Enable each test as part of a CL that lets the test pass.

// Check that readPixels does not set a destination pixel when
// the corresponding source pixel is outside the framebuffer.
TEST_P(WebGLReadOutsideFramebufferTest, ReadPixels)
{
    // Main(&WebGLReadOutsideFramebufferTest::TestReadPixels, false);
}

// Check that copyTexSubImage2D does not set a destination pixel when
// the corresponding source pixel is outside the framebuffer.
TEST_P(WebGLReadOutsideFramebufferTest, CopyTexSubImage2D)
{
    if (IsOpenGL() || IsOpenGLES())
    {
        Main(&WebGLReadOutsideFramebufferTest::TestCopyTexSubImage2D, false);
    }
}

// Check that copyTexImage2D sets (0,0,0,0) for pixels outside the framebuffer.
TEST_P(WebGLReadOutsideFramebufferTest, CopyTexImage2D)
{
    // Main(&WebGLReadOutsideFramebufferTest::TestCopyTexImage2D, true);
}

ANGLE_INSTANTIATE_TEST(WebGLReadOutsideFramebufferTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES3_OPENGL(),
                       ES2_OPENGLES(),
                       ES3_OPENGLES());

}  // namespace
