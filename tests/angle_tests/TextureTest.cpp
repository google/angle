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

        const std::string vertexShaderSource = SHADER_SOURCE
        (
            precision highp float;
            attribute vec4 position;
            varying vec2 texcoord;

            void main()
            {
                gl_Position = position;
                texcoord = (position.xy * 0.5) + 0.5;
            }
        );

        const std::string fragmentShaderSource = SHADER_SOURCE
        (
            precision highp float;
            uniform sampler2D tex;
            varying vec2 texcoord;

            void main()
            {
                gl_FragColor = texture2D(tex, texcoord);
            }
        );

        mProgram = compileProgram(vertexShaderSource, fragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        mTextureUniformLocation = glGetUniformLocation(mProgram, "tex");
    }

    virtual void TearDown()
    {
        glDeleteTextures(1, &mTexture);
        glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    GLuint mTexture;

    GLuint mProgram;
    GLint mTextureUniformLocation;
};

TEST_F(TextureTest, negative_api_subimage)
{
    glBindTexture(GL_TEXTURE_2D, mTexture);
    EXPECT_GL_ERROR(GL_NO_ERROR);

    const GLubyte *pixels[20] = { 0 };
    glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

TEST_F(TextureTest, zero_sized_uploads)
{
    glBindTexture(GL_TEXTURE_2D, mTexture);
    EXPECT_GL_ERROR(GL_NO_ERROR);

    // Use the texture first to make sure it's in video memory
    glUseProgram(mProgram);
    glUniform1i(mTextureUniformLocation, 0);
    drawQuad(mProgram, "position", 0.5f);

    const GLubyte *pixel[4] = { 0 };

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_GL_NO_ERROR();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_GL_NO_ERROR();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_GL_NO_ERROR();
}
