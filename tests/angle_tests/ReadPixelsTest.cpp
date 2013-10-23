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
    std::vector<GLubyte> pixels(pixelsWidth * pixelsHeight * 4);

    glReadPixels(-pixelsWidth / 2, -pixelsHeight / 2, pixelsWidth, pixelsHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    EXPECT_GL_NO_ERROR();

    for (int y = pixelsHeight / 2; y < pixelsHeight; y++)
    {
        for (int x = pixelsWidth / 2; x < pixelsWidth; x++)
        {
            const GLubyte* pixel = pixels.data() + ((y * pixelsWidth + x) * 4);

            // Expect that all pixels which fell within the framebuffer are red
            EXPECT_EQ(pixel[0], 255);
            EXPECT_EQ(pixel[1], 0);
            EXPECT_EQ(pixel[2], 0);
            EXPECT_EQ(pixel[3], 255);
        }
    }
}
