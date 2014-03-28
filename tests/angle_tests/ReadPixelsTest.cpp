#include "ANGLETest.h"

class ReadPixelsTest : public ANGLETest
{
protected:
    ReadPixelsTest()
    {
        setWindowWidth(32);
        setWindowHeight(32);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

TEST_F(ReadPixelsTest, out_of_bounds)
{
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_GL_NO_ERROR();

    GLsizei pixelsWidth = 32;
    GLsizei pixelsHeight = 32;
    GLint offset = 16;
    std::vector<GLubyte> pixels((pixelsWidth + offset) * (pixelsHeight + offset) * 4);

    glReadPixels(-offset, -offset, pixelsWidth + offset, pixelsHeight + offset, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    EXPECT_GL_NO_ERROR();

    for (int y = pixelsHeight / 2; y < pixelsHeight; y++)
    {
        for (int x = pixelsWidth / 2; x < pixelsWidth; x++)
        {
            const GLubyte* pixel = pixels.data() + ((y * (pixelsWidth + offset) + x) * 4);
            unsigned int r = static_cast<unsigned int>(pixel[0]);
            unsigned int g = static_cast<unsigned int>(pixel[1]);
            unsigned int b = static_cast<unsigned int>(pixel[2]);
            unsigned int a = static_cast<unsigned int>(pixel[3]);

            // Expect that all pixels which fell within the framebuffer are red
            EXPECT_EQ(255, r);
            EXPECT_EQ(0,   g);
            EXPECT_EQ(0,   b);
            EXPECT_EQ(255, a);
        }
    }
}
