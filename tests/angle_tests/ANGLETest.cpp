#include "ANGLETest.h"

ANGLETest::ANGLETest()
    : mClientVersion(2)
    , mWidth(1280)
    , mHeight(720)
    , mRedBits(-1)
    , mGreenBits(-1)
    , mBlueBits(-1)
    , mAlphaBits(-1)
    , mDepthBits(-1)
    , mStencilBits(-1)
    , mMultisample(false)
{
}

EGLDisplay ANGLETest::mDisplay = 0;
EGLNativeWindowType ANGLETest::mNativeWindow = 0;
EGLNativeDisplayType ANGLETest::mNativeDisplay = 0;

void ANGLETest::SetUp()
{
    ReizeWindow(mWidth, mHeight);
    if (!createEGLContext())
    {
        FAIL() << "egl context creation failed.";
    }
}

void ANGLETest::TearDown()
{
    swapBuffers();
    if (!destroyEGLContext())
    {
        FAIL() << "egl context destruction failed.";
    }
}

void ANGLETest::swapBuffers()
{
    eglSwapBuffers(mDisplay, mSurface);
}

void ANGLETest::drawQuad(GLuint program, const std::string& positionAttribName, GLfloat quadDepth)
{
    GLint positionLocation = glGetAttribLocation(program, positionAttribName.c_str());

    glUseProgram(program);

    const GLfloat vertices[] =
    {
        -1.0f,  1.0f, quadDepth,
        -1.0f, -1.0f, quadDepth,
         1.0f, -1.0f, quadDepth,

        -1.0f,  1.0f, quadDepth,
         1.0f, -1.0f, quadDepth,
         1.0f,  1.0f, quadDepth,
    };

    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(positionLocation);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(positionLocation);
    glVertexAttribPointer(positionLocation, 4, GL_FLOAT, GL_FALSE, 0, NULL);

    glUseProgram(0);
}

GLuint ANGLETest::compileProgram(const std::string &vsSource, const std::string &fsSource)
{
    GLuint program = glCreateProgram();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const char *vsSourceArray[1] = { vsSource.c_str() };
    glShaderSource(vs, 1, vsSourceArray, NULL);
    glCompileShader(vs);
    glAttachShader(program, vs);
    glDeleteShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fsSourceArray[1] = { fsSource.c_str() };
    glShaderSource(fs, 1, fsSourceArray, NULL);
    glCompileShader(fs);
    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    if (!linkStatus)
    {
        glDeleteProgram(program);
        program = 0;
    }

    return program;
}

bool ANGLETest::extensionEnabled(const std::string &extName)
{
    const char* extString = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    return strstr(extString, extName.c_str()) != NULL;
}

void ANGLETest::setClientVersion(int clientVersion)
{
    mClientVersion = clientVersion;
}

void ANGLETest::setWindowWidth(int width)
{
    mWidth = width;
}

void ANGLETest::setWindowHeight(int height)
{
    mHeight = height;
}

void ANGLETest::setRedBits(int bits)
{
    mRedBits = bits;
}

void ANGLETest::setGreenBits(int bits)
{
    mGreenBits = bits;
}

void ANGLETest::setBlueBits(int bits)
{
    mBlueBits = bits;
}

void ANGLETest::setAlphaBits(int bits)
{
    mAlphaBits = bits;
}

void ANGLETest::setDepthBits(int bits)
{
    mDepthBits = bits;
}

void ANGLETest::setStencilBits(int bits)
{
    mStencilBits = bits;
}

void ANGLETest::setMultisampleEnabled(bool enabled)
{
    mMultisample = enabled;
}

int ANGLETest::getClientVersion() const
{
    return mClientVersion;
}

int ANGLETest::getWindowWidth() const
{
    return mWidth;
}

int ANGLETest::getWindowHeight() const
{
    return mHeight;
}

int ANGLETest::getRedBits() const
{
    return mRedBits;
}

int ANGLETest::getGreenBits() const
{
    return mGreenBits;
}

int ANGLETest::getBlueBits() const
{
    return mBlueBits;
}

int ANGLETest::getAlphaBits() const
{
    return mAlphaBits;
}

int ANGLETest::getDepthBits() const
{
    return mDepthBits;
}

int ANGLETest::getStencilBits() const
{
    return mStencilBits;
}

bool ANGLETest::multisampleEnabled() const
{
    return mMultisample;
}

bool ANGLETest::createEGLContext()
{
    const EGLint configAttributes[] =
    {
        EGL_RED_SIZE,       (mRedBits >= 0)     ? mRedBits     : EGL_DONT_CARE,
        EGL_GREEN_SIZE,     (mGreenBits >= 0)   ? mGreenBits   : EGL_DONT_CARE,
        EGL_BLUE_SIZE,      (mBlueBits >= 0)    ? mBlueBits    : EGL_DONT_CARE,
        EGL_ALPHA_SIZE,     (mAlphaBits >= 0)   ? mAlphaBits   : EGL_DONT_CARE,
        EGL_DEPTH_SIZE,     (mDepthBits >= 0)   ? mDepthBits   : EGL_DONT_CARE,
        EGL_STENCIL_SIZE,   (mStencilBits >= 0) ? mStencilBits : EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS, mMultisample ? 1 : 0,
        EGL_NONE
    };

    EGLint configCount;
    if (!eglChooseConfig(mDisplay, configAttributes, &mConfig, 1, &configCount) || (configCount != 1))
    {
        destroyEGLContext();
        return false;
    }

    eglGetConfigAttrib(mDisplay, mConfig, EGL_RED_SIZE, &mRedBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_GREEN_SIZE, &mGreenBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_BLUE_SIZE, &mBlueBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_ALPHA_SIZE, &mBlueBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_DEPTH_SIZE, &mDepthBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_STENCIL_SIZE, &mStencilBits);

    EGLint samples;
    eglGetConfigAttrib(mDisplay, mConfig, EGL_SAMPLE_BUFFERS, &samples);
    mMultisample = (samples != 0);

    mSurface = eglCreateWindowSurface(mDisplay, mConfig, mNativeWindow, NULL);
    if(mSurface == EGL_NO_SURFACE)
    {
        eglGetError(); // Clear error
        mSurface = eglCreateWindowSurface(mDisplay, mConfig, NULL, NULL);
    }

    if (eglGetError() != EGL_SUCCESS)
    {
        destroyEGLContext();
        return false;
    }

    EGLint contextAttibutes[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, mClientVersion,
        EGL_NONE
    };
    mContext = eglCreateContext(mDisplay, mConfig, NULL, contextAttibutes);
    if (eglGetError() != EGL_SUCCESS)
    {
        destroyEGLContext();
        return false;
    }

    eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);
    if (eglGetError() != EGL_SUCCESS)
    {
        destroyEGLContext();
        return false;
    }

    return true;
}

bool ANGLETest::destroyEGLContext()
{
    eglDestroySurface(mDisplay, mSurface);
    eglDestroyContext(mDisplay, mContext);

    return true;
}
