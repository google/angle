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
        glGenTextures(1, &mTexture2D);
        glGenTextures(1, &mTextureCube);

        glBindTexture(GL_TEXTURE_2D, mTexture2D);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        EXPECT_GL_NO_ERROR();

        glBindTexture(GL_TEXTURE_CUBE_MAP, mTextureCube);
        glTexStorage2DEXT(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, 1, 1);
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

        const std::string fragmentShaderSource2D = SHADER_SOURCE
        (
            precision highp float;
            uniform sampler2D tex;
            varying vec2 texcoord;

            void main()
            {
                gl_FragColor = texture2D(tex, texcoord);
            }
        );

        const std::string fragmentShaderSourceCube = SHADER_SOURCE
        (
            precision highp float;
            uniform sampler2D tex2D;
            uniform samplerCube texCube;
            varying vec2 texcoord;

            void main()
            {
                gl_FragColor = texture2D(tex2D, texcoord);
                gl_FragColor += textureCube(texCube, vec3(texcoord, 0));
            }
        );

        m2DProgram = CompileProgram(vertexShaderSource, fragmentShaderSource2D);
        mCubeProgram = CompileProgram(vertexShaderSource, fragmentShaderSourceCube);
        if (m2DProgram == 0 || mCubeProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        mTexture2DUniformLocation = glGetUniformLocation(m2DProgram, "tex");
    }

    virtual void TearDown()
    {
        glDeleteTextures(1, &mTexture2D);
        glDeleteTextures(1, &mTextureCube);
        glDeleteProgram(m2DProgram);
        glDeleteProgram(mCubeProgram);

        ANGLETest::TearDown();
    }

    GLuint mTexture2D;
    GLuint mTextureCube;

    GLuint m2DProgram;
    GLuint mCubeProgram;
    GLint mTexture2DUniformLocation;
};

TEST_F(TextureTest, NegativeAPISubImage)
{
    glBindTexture(GL_TEXTURE_2D, mTexture2D);
    EXPECT_GL_ERROR(GL_NO_ERROR);

    const GLubyte *pixels[20] = { 0 };
    glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

TEST_F(TextureTest, ZeroSizedUploads)
{
    glBindTexture(GL_TEXTURE_2D, mTexture2D);
    EXPECT_GL_ERROR(GL_NO_ERROR);

    // Use the texture first to make sure it's in video memory
    glUseProgram(m2DProgram);
    glUniform1i(mTexture2DUniformLocation, 0);
    drawQuad(m2DProgram, "position", 0.5f);

    const GLubyte *pixel[4] = { 0 };

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_GL_NO_ERROR();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_GL_NO_ERROR();

    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    EXPECT_GL_NO_ERROR();
}

// Test drawing with two texture types, to trigger an ANGLE bug in validation
TEST_F(TextureTest, CubeMapBug)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture2D);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTextureCube);
    EXPECT_GL_ERROR(GL_NO_ERROR);

    glUseProgram(mCubeProgram);
    GLint tex2DUniformLocation = glGetUniformLocation(mCubeProgram, "tex2D");
    GLint texCubeUniformLocation = glGetUniformLocation(mCubeProgram, "texCube");
    EXPECT_NE(-1, tex2DUniformLocation);
    EXPECT_NE(-1, texCubeUniformLocation);
    glUniform1i(tex2DUniformLocation, 0);
    glUniform1i(texCubeUniformLocation, 1);
    drawQuad(mCubeProgram, "position", 0.5f);
    EXPECT_GL_NO_ERROR();
}
