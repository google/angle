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

    // Tests CopyTexSubImage with floating point textures of various formats.
    void testFloatCopySubImage(int sourceImageChannels, int destImageChannels)
    {
        GLfloat sourceImageData[4][16] =
        {
            { // R
                1.0f,
                0.0f,
                0.0f,
                1.0f
            },
            { // RG
                1.0f, 0.0f,
                0.0f, 1.0f,
                0.0f, 0.0f,
                1.0f, 1.0f
            },
            { // RGB
                1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 1.0f,
                1.0f, 1.0f, 0.0f
            },
            { // RGBA
                1.0f, 0.0f, 0.0f, 1.0f,
                0.0f, 1.0f, 0.0f, 1.0f,
                0.0f, 0.0f, 1.0f, 1.0f,
                1.0f, 1.0f, 0.0f, 1.0f
            },
        };

        GLenum imageFormats[] =
        {
            GL_R32F,
            GL_RG32F,
            GL_RGB32F,
            GL_RGBA32F,
        };

        GLenum sourceUnsizedFormats[] =
        {
            GL_RED,
            GL_RG,
            GL_RGB,
            GL_RGBA,
        };

        GLuint textures[2];

        glGenTextures(2, textures);

        GLfloat *imageData = sourceImageData[sourceImageChannels - 1];
        GLenum sourceImageFormat = imageFormats[sourceImageChannels - 1];
        GLenum sourceUnsizedFormat = sourceUnsizedFormats[sourceImageChannels - 1];
        GLenum destImageFormat = imageFormats[destImageChannels - 1];

        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glTexStorage2DEXT(GL_TEXTURE_2D, 1, sourceImageFormat, 2, 2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, sourceUnsizedFormat, GL_FLOAT, imageData);

        if (sourceImageChannels < 3 && !extensionEnabled("GL_EXT_texture_rg"))
        {
            // This is not supported
            ASSERT_GL_ERROR(GL_INVALID_OPERATION);
        }
        else
        {
            ASSERT_GL_NO_ERROR();
        }

        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[0], 0);

        glBindTexture(GL_TEXTURE_2D, textures[1]);
        glTexStorage2DEXT(GL_TEXTURE_2D, 1, destImageFormat, 2, 2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 2, 2);
        ASSERT_GL_NO_ERROR();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        drawQuad(m2DProgram, "position", 0.5f);
        swapBuffers();

        int testImageChannels = std::min(sourceImageChannels, destImageChannels);

        EXPECT_PIXEL_EQ(0, 0, 255, 0, 0, 255);
        if (testImageChannels > 1)
        {
            EXPECT_PIXEL_EQ(getWindowHeight() - 1, 0, 0, 255, 0, 255);
            EXPECT_PIXEL_EQ(getWindowHeight() - 1, getWindowWidth() - 1, 255, 255, 0, 255);
            if (testImageChannels > 2)
            {
                EXPECT_PIXEL_EQ(0, getWindowWidth() - 1, 0, 0, 255, 255);
            }
        }

        glDeleteFramebuffers(1, &fbo);
        glDeleteTextures(2, textures);

        ASSERT_GL_NO_ERROR();
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

// Test that glTexSubImage2D works properly when glTexStorage2DEXT has initialized the image with a default color.
TYPED_TEST(TextureTest, TexStorage)
{
    int width = getWindowWidth();
    int height = getWindowHeight();

    GLuint tex2D;
    glGenTextures(1, &tex2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex2D);

    // Fill with red
    std::vector<GLubyte> pixels(3 * 16 * 16);
    for (size_t pixelId = 0; pixelId < 16 * 16; ++pixelId)
    {
        pixels[pixelId * 3 + 0] = 255;
        pixels[pixelId * 3 + 1] = 0;
        pixels[pixelId * 3 + 2] = 0;
    }

    // ANGLE internally uses RGBA as the DirectX format for RGB images
    // therefore glTexStorage2DEXT initializes the image to a default color to get a consistent alpha color.
    // The data is kept in a CPU-side image and the image is marked as dirty.
    glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGB8, 16, 16);

    // Initializes the color of the upper-left 8x8 pixels, leaves the other pixels untouched.
    // glTexSubImage2D should take into account that the image is dirty.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 8, 8, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUseProgram(m2DProgram);
    glUniform1i(mTexture2DUniformLocation, 0);
    glUniform2f(mTextureScaleUniformLocation, 1.f, 1.f);
    drawQuad(m2DProgram, "position", 0.5f);
    glDeleteTextures(1, &tex2D);
    EXPECT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(3*width/4, 3*height/4, 0, 0, 0, 255);
    EXPECT_PIXEL_EQ(width / 4, height / 4, 255, 0, 0, 255);
}

// Test that glTexSubImage2D combined with a PBO works properly when glTexStorage2DEXT has initialized the image with a default color.
TYPED_TEST(TextureTest, TexStorageWithPBO)
{
    if (extensionEnabled("NV_pixel_buffer_object"))
    {
        int width = getWindowWidth();
        int height = getWindowHeight();

        GLuint tex2D;
        glGenTextures(1, &tex2D);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex2D);

        // Fill with red
        std::vector<GLubyte> pixels(3 * 16 * 16);
        for (size_t pixelId = 0; pixelId < 16 * 16; ++pixelId)
        {
            pixels[pixelId * 3 + 0] = 255;
            pixels[pixelId * 3 + 1] = 0;
            pixels[pixelId * 3 + 2] = 0;
        }

        // Read 16x16 region from red backbuffer to PBO
        GLuint pbo;
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, 3 * 16 * 16, pixels.data(), GL_STATIC_DRAW);

        // ANGLE internally uses RGBA as the DirectX format for RGB images
        // therefore glTexStorage2DEXT initializes the image to a default color to get a consistent alpha color.
        // The data is kept in a CPU-side image and the image is marked as dirty.
        glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGB8, 16, 16);

        // Initializes the color of the upper-left 8x8 pixels, leaves the other pixels untouched.
        // glTexSubImage2D should take into account that the image is dirty.
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 8, 8, GL_RGB, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glUseProgram(m2DProgram);
        glUniform1i(mTexture2DUniformLocation, 0);
        glUniform2f(mTextureScaleUniformLocation, 1.f, 1.f);
        drawQuad(m2DProgram, "position", 0.5f);
        glDeleteTextures(1, &tex2D);
        glDeleteTextures(1, &pbo);
        EXPECT_GL_NO_ERROR();
        EXPECT_PIXEL_EQ(3 * width / 4, 3 * height / 4, 0, 0, 0, 255);
        EXPECT_PIXEL_EQ(width / 4, height / 4, 255, 0, 0, 255);
    }
}

// See description on testFloatCopySubImage
// TODO(jmadill): Fix sampling from unused channels on D3D9
TYPED_TEST(TextureTest, DISABLED_CopySubImageFloat_R_R)
{
    testFloatCopySubImage(1, 1);
}

// TODO(jmadill): Fix sampling from unused channels on D3D9
TYPED_TEST(TextureTest, DISABLED_CopySubImageFloat_RG_R)
{
    testFloatCopySubImage(2, 1);
}

// TODO(jmadill): Fix sampling from unused channels on D3D9
TYPED_TEST(TextureTest, DISABLED_CopySubImageFloat_RG_RG)
{
    testFloatCopySubImage(2, 2);
}

// TODO(jmadill): Fix sampling from unused channels on D3D9
TYPED_TEST(TextureTest, DISABLED_CopySubImageFloat_RGB_R)
{
    testFloatCopySubImage(3, 1);
}

// TODO(jmadill): Fix sampling from unused channels on D3D9
TYPED_TEST(TextureTest, DISABLED_CopySubImageFloat_RGB_RG)
{
    testFloatCopySubImage(3, 2);
}

TYPED_TEST(TextureTest, CopySubImageFloat_RGB_RGB)
{
    // TODO(jmadill): Figure out why this is broken on Intel D3D11
    if (isIntel() && getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
    {
        std::cout << "Test skipped on Intel D3D11." << std::endl;
        return;
    }

    testFloatCopySubImage(3, 3);
}

// TODO(jmadill): Fix sampling from unused channels on D3D9
TYPED_TEST(TextureTest, DISABLED_CopySubImageFloat_RGBA_R)
{
    testFloatCopySubImage(4, 1);
}

// TODO(jmadill): Fix sampling from unused channels on D3D9
TYPED_TEST(TextureTest, DISABLED_CopySubImageFloat_RGBA_RG)
{
    testFloatCopySubImage(4, 2);
}

TYPED_TEST(TextureTest, CopySubImageFloat_RGBA_RGB)
{
    // TODO(jmadill): Figure out why this is broken on Intel D3D11
    if (isIntel() && getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
    {
        std::cout << "Test skipped on Intel D3D11." << std::endl;
        return;
    }

    testFloatCopySubImage(4, 3);
}

TYPED_TEST(TextureTest, CopySubImageFloat_RGBA_RGBA)
{
    // TODO(jmadill): Figure out why this is broken on Intel D3D11
    if (isIntel() && getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
    {
        std::cout << "Test skipped on Intel D3D11." << std::endl;
        return;
    }

    testFloatCopySubImage(4, 4);
}
