#include "ANGLETest.h"

class TextureTest : public ANGLETest
{
protected:
    TextureTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();
        glGenTextures(1, &mTexture);

        glBindTexture(GL_TEXTURE_2D, mTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        EXPECT_GL_NO_ERROR();

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteTextures(1, &mTexture);

        ANGLETest::TearDown();
    }

    GLuint mTexture;
};

TEST_F(TextureTest, negative_api_subimage)
{
    glBindTexture(GL_TEXTURE_2D, mTexture);
    EXPECT_GL_ERROR(GL_NO_ERROR);

    const GLubyte *pixels[20] = { 0 };
    glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}
