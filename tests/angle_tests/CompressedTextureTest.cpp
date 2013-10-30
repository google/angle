#include "ANGLETest.h"
#include "media/stanfordbunny.inl"
#include "media/stanforddragon.inl"

class CompressedTextureTest : public ANGLETest
{
protected:
    CompressedTextureTest()
    {
        setWindowWidth(512);
        setWindowHeight(512);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const std::string vsSource = SHADER_SOURCE
        (
            precision highp float;
            attribute vec4 position;
            varying vec2 texcoord;

            void main()
            {
                gl_Position = position;
                texcoord = (position.xy * 0.5) + 0.5;
                texcoord.y = 1.0 - texcoord.y;
            }
        );

        const std::string textureFSSource = SHADER_SOURCE
        (
            precision highp float;
            uniform sampler2D tex;
            varying vec2 texcoord;

            void main()
            {
                gl_FragColor = texture2D(tex, texcoord);
            }
        );

        mTextureProgram = compileProgram(vsSource, textureFSSource);
        if (mTextureProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        mTextureUniformLocation = glGetUniformLocation(mTextureProgram, "tex");

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteProgram(mTextureProgram);

        ANGLETest::TearDown();
    }

    GLuint mTextureProgram;
    GLint mTextureUniformLocation;
};

TEST_F(CompressedTextureTest, compressed_tex_image)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_EXT_texture_compression_dxt1"))
    {
        return;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 0, 1U), std::max(stanfordbunnyHeight >> 0, 1U), 0, sizeof(stanfordbunny_0), stanfordbunny_0);
    glCompressedTexImage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 1, 1U), std::max(stanfordbunnyHeight >> 1, 1U), 0, sizeof(stanfordbunny_1), stanfordbunny_1);
    glCompressedTexImage2D(GL_TEXTURE_2D, 2, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 2, 1U), std::max(stanfordbunnyHeight >> 2, 1U), 0, sizeof(stanfordbunny_2), stanfordbunny_2);
    glCompressedTexImage2D(GL_TEXTURE_2D, 3, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 3, 1U), std::max(stanfordbunnyHeight >> 3, 1U), 0, sizeof(stanfordbunny_3), stanfordbunny_3);
    glCompressedTexImage2D(GL_TEXTURE_2D, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 4, 1U), std::max(stanfordbunnyHeight >> 4, 1U), 0, sizeof(stanfordbunny_4), stanfordbunny_4);
    glCompressedTexImage2D(GL_TEXTURE_2D, 5, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 5, 1U), std::max(stanfordbunnyHeight >> 5, 1U), 0, sizeof(stanfordbunny_5), stanfordbunny_5);
    glCompressedTexImage2D(GL_TEXTURE_2D, 6, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 6, 1U), std::max(stanfordbunnyHeight >> 6, 1U), 0, sizeof(stanfordbunny_6), stanfordbunny_6);
    glCompressedTexImage2D(GL_TEXTURE_2D, 7, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 7, 1U), std::max(stanfordbunnyHeight >> 7, 1U), 0, sizeof(stanfordbunny_7), stanfordbunny_7);
    glCompressedTexImage2D(GL_TEXTURE_2D, 8, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 8, 1U), std::max(stanfordbunnyHeight >> 8, 1U), 0, sizeof(stanfordbunny_8), stanfordbunny_8);
    glCompressedTexImage2D(GL_TEXTURE_2D, 9, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, std::max(stanfordbunnyWidth >> 9, 1U), std::max(stanfordbunnyHeight >> 9, 1U), 0, sizeof(stanfordbunny_9), stanfordbunny_9);

    EXPECT_GL_NO_ERROR();

    glUseProgram(mTextureProgram);
    glUniform1i(mTextureUniformLocation, 0);

    drawQuad(mTextureProgram, "position", 0.5f);

    EXPECT_GL_NO_ERROR();

    glDeleteTextures(1, &texture);

    EXPECT_GL_NO_ERROR();
}

TEST_F(CompressedTextureTest, compressed_tex_storage)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_EXT_texture_compression_dxt1"))
    {
        return;
    }

    if (getClientVersion() < 3 && (!extensionEnabled("GL_EXT_texture_storage") || !extensionEnabled("GL_OES_rgb8_rgba8")))
    {
        return;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if (getClientVersion() < 3)
    {
        glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, stanforddragonWidth, stanforddragonHeight);
    }
    else
    {
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, stanforddragonWidth, stanforddragonHeight);
    }
    EXPECT_GL_NO_ERROR();

    glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, stanforddragonWidth, stanforddragonHeight, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, sizeof(stanforddragon), stanforddragon);

    EXPECT_GL_NO_ERROR();

    glUseProgram(mTextureProgram);
    glUniform1i(mTextureUniformLocation, 0);

    drawQuad(mTextureProgram, "position", 0.5f);

    EXPECT_GL_NO_ERROR();

    glDeleteTextures(1, &texture);

    EXPECT_GL_NO_ERROR();
}
