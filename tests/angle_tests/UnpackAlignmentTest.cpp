#include "ANGLETest.h"

#include <array>

class UnpackAlignmentTest : public ANGLETest
{
protected:
    UnpackAlignmentTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setRedBits(8);
        setGreenBits(8);
        setBlueBits(8);
        setAlphaBits(8);
        setDepthBits(24);

        mProgram = 0;
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const std::string vertexShaderSource = SHADER_SOURCE
        (
            precision highp float;
            attribute vec4 position;

            void main()
            {
                gl_Position = position;
            }
        );

        const std::string fragmentShaderSource = SHADER_SOURCE
        (
            uniform sampler2D tex;

            void main()
            {
                gl_FragColor = texture2D(tex, vec2(0.0, 1.0));
            }
        );

        mProgram = compileProgram(vertexShaderSource, fragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }
    }

    virtual void TearDown()
    {
        glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    void getPixelSize(GLenum format, GLenum type, unsigned int* size)
    {
        switch (type)
        {
          case GL_UNSIGNED_SHORT_5_5_5_1:
          case GL_UNSIGNED_SHORT_5_6_5:
          case GL_UNSIGNED_SHORT_4_4_4_4:
            *size = sizeof(GLushort);
            break;

          case GL_UNSIGNED_BYTE:
            {
                unsigned int compCount = 0;
                switch (format)
                {
                  case GL_RGBA:            compCount = 4; break;
                  case GL_RGB:             compCount = 3; break;
                  case GL_LUMINANCE_ALPHA: compCount = 2; break;
                  case GL_LUMINANCE:       compCount = 1; break;
                  case GL_ALPHA:           compCount = 1; break;
                  FAIL() << "unknown pixel format.";
                }
                *size = sizeof(GLubyte) * compCount;
            }
            break;
          default:
            FAIL() << "unknown pixel type.";
        }
    }

    bool formatHasRGB(GLenum format)
    {
        return (format != GL_ALPHA);
    }

    void testAlignment(int alignment, unsigned int offset, GLenum format, GLenum type)
    {
        static const unsigned int width = 7;
        static const unsigned int height = 2;

        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

        GLint readbackAlignment;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &readbackAlignment);
        EXPECT_EQ(alignment, readbackAlignment);

        std::array<GLubyte, 1024> buf;
        std::fill(buf.begin(), buf.end(), 0);

        unsigned int pixelSize;
        getPixelSize(format, type, &pixelSize);
        for (unsigned int i = 0; i < pixelSize; i++)
        {
            buf[offset+i] = 0xFF;
        }

        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, type, &buf[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        drawQuad(mProgram, "position", 0.5f);

        GLubyte expectedRGB = formatHasRGB(format) ? 255 : 0;
        EXPECT_PIXEL_EQ(0, 0, expectedRGB, expectedRGB, expectedRGB, 255);

        glDeleteTextures(1, &tex);
    }

    GLuint mProgram;
};

TEST_F(UnpackAlignmentTest, default_alignment)
{
    GLint defaultAlignment;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &defaultAlignment);
    EXPECT_EQ(defaultAlignment, 4);
}


TEST_F(UnpackAlignmentTest, alignment_1_rgba_ubyte)
{
    testAlignment(1, 7 * 4, GL_RGBA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_1_rgb_ubyte)
{
    testAlignment(1, 7 * 3, GL_RGB, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_1_rgba_ushort4444)
{
    testAlignment(1, 7 * 2, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
}

TEST_F(UnpackAlignmentTest, alignment_1_rgba_ushort5551)
{
    testAlignment(1, 7 * 2, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
}

TEST_F(UnpackAlignmentTest, alignment_1_rgb_ushort565)
{
    testAlignment(1, 7 * 2, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
}

TEST_F(UnpackAlignmentTest, alignment_1_la_ubyte)
{
    testAlignment(1, 7 * 2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_1_l_ubyte)
{
    testAlignment(1, 7, GL_LUMINANCE, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_1_a_ubyte)
{
    testAlignment(1, 7, GL_ALPHA, GL_UNSIGNED_BYTE);
}


TEST_F(UnpackAlignmentTest, alignment_2_rgba_ubyte)
{
    testAlignment(2, 7 * 4, GL_RGBA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_2_rgb_ubyte)
{
    testAlignment(2, 7 * 3 + 1, GL_RGB, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_2_rgba_ushort4444)
{
    testAlignment(2, 7 * 2, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
}

TEST_F(UnpackAlignmentTest, alignment_2_rgba_ushort5551)
{
    testAlignment(2, 7 * 2, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
}

TEST_F(UnpackAlignmentTest, alignment_2_rgb_ushort565)
{
    testAlignment(2, 7 * 2, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
}

TEST_F(UnpackAlignmentTest, alignment_2_la_ubyte)
{
    testAlignment(2, 7 * 2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_2_l_ubyte)
{
    testAlignment(2, 7 + 1, GL_LUMINANCE, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_2_a_ubyte)
{
    testAlignment(2, 7 + 1, GL_ALPHA, GL_UNSIGNED_BYTE);
}


TEST_F(UnpackAlignmentTest, alignment_4_rgba_ubyte)
{
    testAlignment(4, 7 * 4, GL_RGBA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_4_rgb_ubyte)
{
    testAlignment(4, 7 * 3 + 3, GL_RGB, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_4_rgba_ushort4444)
{
    testAlignment(4, 7 * 2 + 2, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
}

TEST_F(UnpackAlignmentTest, alignment_4_rgba_ushort5551)
{
    testAlignment(4, 7 * 2 + 2, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
}

TEST_F(UnpackAlignmentTest, alignment_4_rgb_ushort565)
{
    testAlignment(4, 7 * 2 + 2, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
}

TEST_F(UnpackAlignmentTest, alignment_4_la_ubyte)
{
    testAlignment(4, 7 * 2 + 2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_4_l_ubyte)
{
    testAlignment(4, 7 + 1, GL_LUMINANCE, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_4_a_ubyte)
{
    testAlignment(4, 7 + 1, GL_ALPHA, GL_UNSIGNED_BYTE);
}


TEST_F(UnpackAlignmentTest, alignment_8_rgba_ubyte)
{
    testAlignment(8, 7 * 4 + 4, GL_RGBA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_8_rgb_ubyte)
{
    testAlignment(8, 7 * 3 + 3, GL_RGB, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_8_rgba_ushort4444)
{
    testAlignment(8, 7 * 2 + 2, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
}

TEST_F(UnpackAlignmentTest, alignment_8_rgba_ushort5551)
{
    testAlignment(8, 7 * 2 + 2, GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1);
}

TEST_F(UnpackAlignmentTest, alignment_8_rgb_ushort565)
{
    testAlignment(8, 7 * 2 + 2, GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
}

TEST_F(UnpackAlignmentTest, alignment_8_la_ubyte)
{
    testAlignment(8, 7 * 2 + 2, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_8_l_ubyte)
{
    testAlignment(8, 7 + 1, GL_LUMINANCE, GL_UNSIGNED_BYTE);
}

TEST_F(UnpackAlignmentTest, alignment_8_a_ubyte)
{
    testAlignment(8, 7 + 1, GL_ALPHA, GL_UNSIGNED_BYTE);
}
