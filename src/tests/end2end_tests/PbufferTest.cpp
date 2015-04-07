#include "ANGLETest.h"

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(PbufferTest, ES2_D3D9, ES2_D3D11);

template<typename T>
class PbufferTest : public ANGLETest
{
  protected:
    PbufferTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
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

        mTextureProgram = CompileProgram(vsSource, textureFSSource);
        if (mTextureProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        mTextureUniformLocation = glGetUniformLocation(mTextureProgram, "tex");

        const EGLint pBufferAttributes[] =
        {
            EGL_WIDTH, mPbufferSize,
            EGL_HEIGHT, mPbufferSize,
            EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
            EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
            EGL_NONE, EGL_NONE,
        };

        EGLWindow *window = getEGLWindow();
        mPbuffer = eglCreatePbufferSurface(window->getDisplay(), window->getConfig(), pBufferAttributes);
        ASSERT_NE(mPbuffer, EGL_NO_SURFACE);

        ASSERT_EGL_SUCCESS();
        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteProgram(mTextureProgram);

        EGLWindow *window = getEGLWindow();
        eglDestroySurface(window->getDisplay(), mPbuffer);

        ANGLETest::TearDown();
    }

    GLuint mTextureProgram;
    GLint mTextureUniformLocation;

    const size_t mPbufferSize = 32;
    EGLSurface mPbuffer;
};

// Test clearing a Pbuffer and checking the color is correct
TYPED_TEST(PbufferTest, Clearing)
{
    EGLWindow *window = getEGLWindow();

    // Clear the window surface to blue and verify
    eglMakeCurrent(window->getDisplay(), window->getSurface(), window->getSurface(), window->getContext());
    ASSERT_EGL_SUCCESS();

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(window->getWidth() / 2, window->getHeight() / 2, 0, 0, 255, 255);

    // Apply the Pbuffer and clear it to purple and verify
    eglMakeCurrent(window->getDisplay(), mPbuffer, mPbuffer, window->getContext());
    ASSERT_EGL_SUCCESS();

    glViewport(0, 0, mPbufferSize, mPbufferSize);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(mPbufferSize / 2, mPbufferSize / 2, 255, 0, 255, 255);

    // Rebind the window serfance and verify that it is still blue
    eglMakeCurrent(window->getDisplay(), window->getSurface(), window->getSurface(), window->getContext());
    ASSERT_EGL_SUCCESS();
    EXPECT_PIXEL_EQ(window->getWidth() / 2, window->getHeight() / 2, 0, 0, 255, 255);
}

// Bind the Pbuffer to a texture and verify it renders correctly
TYPED_TEST(PbufferTest, BindTexImage)
{
    EGLWindow *window = getEGLWindow();

    // Apply the Pbuffer and clear it to purple
    eglMakeCurrent(window->getDisplay(), mPbuffer, mPbuffer, window->getContext());
    ASSERT_EGL_SUCCESS();

    glViewport(0, 0, mPbufferSize, mPbufferSize);
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_EQ(mPbufferSize / 2, mPbufferSize / 2, 255, 0, 255, 255);

    // Apply the window surface
    eglMakeCurrent(window->getDisplay(), window->getSurface(), window->getSurface(), window->getContext());

    // Create a texture and bind the Pbuffer to it
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    EXPECT_GL_NO_ERROR();

    eglBindTexImage(window->getDisplay(), mPbuffer, EGL_BACK_BUFFER);
    glViewport(0, 0, window->getWidth(), window->getHeight());
    ASSERT_EGL_SUCCESS();

    // Draw a quad and verify that it is purple
    glUseProgram(mTextureProgram);
    glUniform1i(mTextureUniformLocation, 0);

    drawQuad(mTextureProgram, "position", 0.5f);
    EXPECT_GL_NO_ERROR();

    // Unbind the texture
    eglReleaseTexImage(window->getDisplay(), mPbuffer, EGL_BACK_BUFFER);
    ASSERT_EGL_SUCCESS();

    // Verify that purple was drawn
    EXPECT_PIXEL_EQ(window->getWidth() / 2, window->getHeight() / 2, 255, 0, 255, 255);

    glDeleteTextures(1, &texture);
}

// Verify that when eglBind/ReleaseTexImage are called, the texture images are freed and their
// size information is correctly updated.
TYPED_TEST(PbufferTest, TextureSizeReset)
{
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    EXPECT_GL_NO_ERROR();

    glUseProgram(mTextureProgram);
    glUniform1i(mTextureUniformLocation, 0);

    // Fill the texture with white pixels
    std::vector<GLubyte> whitePixels(mPbufferSize * mPbufferSize * 4, 255);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mPbufferSize, mPbufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &whitePixels[0]);
    EXPECT_GL_NO_ERROR();

    // Draw the white texture and verify that the pixels are correct
    drawQuad(mTextureProgram, "position", 0.5f);
    EXPECT_PIXEL_EQ(0, 0, 255, 255, 255, 255);

    // Bind the EGL surface and draw with it, results are undefined since nothing has
    // been written to it
    EGLWindow *window = getEGLWindow();
    eglBindTexImage(window->getDisplay(), mPbuffer, EGL_BACK_BUFFER);
    drawQuad(mTextureProgram, "position", 0.5f);
    EXPECT_GL_NO_ERROR();

    // Clear the back buffer to a unique color (green)
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_PIXEL_EQ(0, 0, 0, 255, 0, 255);

    // Unbind the EGL surface and try to draw with the texture again, the texture's size should
    // now be zero and incomplete so the back buffer should be black
    eglReleaseTexImage(window->getDisplay(), mPbuffer, EGL_BACK_BUFFER);
    drawQuad(mTextureProgram, "position", 0.5f);
    EXPECT_PIXEL_EQ(0, 0, 0, 0, 0, 255);
}
