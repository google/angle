#include "ANGLETest.h"

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(TextureTest, ES2_D3D9, ES2_D3D11);

template<typename T>
class TextureTest : public ANGLETest
{
protected:
    TextureTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
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

            uniform vec2 textureScale;

            void main()
            {
                gl_Position = vec4(position.xy * textureScale, 0.0, 1.0);
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
        ASSERT_NE(-1, mTexture2DUniformLocation);

        mTextureScaleUniformLocation = glGetUniformLocation(m2DProgram, "textureScale");
        ASSERT_NE(-1, mTextureScaleUniformLocation);

        glUseProgram(m2DProgram);
        glUniform2f(mTextureScaleUniformLocation, 1.0f, 1.0f);
        glUseProgram(0);
        ASSERT_GL_NO_ERROR();
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
    GLint mTextureScaleUniformLocation;
};

TYPED_TEST(TextureTest, NegativeAPISubImage)
{
    glBindTexture(GL_TEXTURE_2D, mTexture2D);
    EXPECT_GL_ERROR(GL_NO_ERROR);

    const GLubyte *pixels[20] = { 0 };
    glTexSubImage2D(GL_TEXTURE_2D, 0, 1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

TYPED_TEST(TextureTest, ZeroSizedUploads)
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
TYPED_TEST(TextureTest, CubeMapBug)
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

// Copy of a test in conformance/textures/texture-mips, to test generate mipmaps
TYPED_TEST(TextureTest, MipmapsTwice)
{
    int px = getWindowWidth() / 2;
    int py = getWindowHeight() / 2;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture2D);

    // Fill with red
    std::vector<GLubyte> pixels(4 * 16 * 16);
    for (size_t pixelId = 0; pixelId < 16 * 16; ++pixelId)
    {
        pixels[pixelId * 4 + 0] = 255;
        pixels[pixelId * 4 + 1] = 0;
        pixels[pixelId * 4 + 2] = 0;
        pixels[pixelId * 4 + 3] = 255;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUseProgram(m2DProgram);
    glUniform1i(mTexture2DUniformLocation, 0);
    glUniform2f(mTextureScaleUniformLocation, 0.0625f, 0.0625f);
    drawQuad(m2DProgram, "position", 0.5f);
    EXPECT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(px, py, 255, 0, 0, 255);

    // Fill with blue
    for (size_t pixelId = 0; pixelId < 16 * 16; ++pixelId)
    {
        pixels[pixelId * 4 + 0] = 0;
        pixels[pixelId * 4 + 1] = 0;
        pixels[pixelId * 4 + 2] = 255;
        pixels[pixelId * 4 + 3] = 255;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    // Fill with green
    for (size_t pixelId = 0; pixelId < 16 * 16; ++pixelId)
    {
        pixels[pixelId * 4 + 0] = 0;
        pixels[pixelId * 4 + 1] = 255;
        pixels[pixelId * 4 + 2] = 0;
        pixels[pixelId * 4 + 3] = 255;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);

    drawQuad(m2DProgram, "position", 0.5f);

    EXPECT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(px, py, 0, 255, 0, 255);
}

// Test creating a FBO with a cube map render target, to test an ANGLE bug
// https://code.google.com/p/angleproject/issues/detail?id=849
TYPED_TEST(TextureTest, CubeMapFBO)
{
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glBindTexture(GL_TEXTURE_CUBE_MAP, mTextureCube);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, mTextureCube, 0);

    EXPECT_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glDeleteFramebuffers(1, &fbo);

    EXPECT_GL_NO_ERROR();
}
