//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef ANGLE_TESTS_ANGLE_TEST_H_
#define ANGLE_TESTS_ANGLE_TEST_H_

#include "gtest/gtest.h"

#define GL_GLEXT_PROTOTYPES

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <algorithm>

#define EXPECT_GL_ERROR(err) EXPECT_EQ((err), glGetError())
#define EXPECT_GL_NO_ERROR() EXPECT_GL_ERROR(GL_NO_ERROR)

#define ASSERT_GL_ERROR(err) ASSERT_EQ((err), glGetError())
#define ASSERT_GL_NO_ERROR() ASSERT_GL_ERROR(GL_NO_ERROR)

#define EXPECT_PIXEL_EQ(x, y, r, g, b, a) \
{ \
    GLubyte pixel[4]; \
    glReadPixels((x), (y), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel); \
    EXPECT_GL_NO_ERROR(); \
    EXPECT_EQ((r), pixel[0]); \
    EXPECT_EQ((g), pixel[1]); \
    EXPECT_EQ((b), pixel[2]); \
    EXPECT_EQ((a), pixel[3]); \
}

#define SHADER_SOURCE(...) #__VA_ARGS__

class ANGLETest : public testing::Test
{
  protected:
    ANGLETest();

  public:
    static bool InitTestWindow();
    static bool DestroyTestWindow();
    static bool ReizeWindow(int width, int height);

  protected:
    virtual void SetUp();
    virtual void TearDown();

    virtual void swapBuffers();

    static void drawQuad(GLuint program, const std::string& positionAttribName, GLfloat quadDepth);
    static GLuint compileShader(GLenum type, const std::string &source);
    static GLuint compileProgram(const std::string &vsSource, const std::string &fsSource);
    static bool extensionEnabled(const std::string &extName);

    void setClientVersion(int clientVersion);
    void setWindowWidth(int width);
    void setWindowHeight(int height);
    void setConfigRedBits(int bits);
    void setConfigGreenBits(int bits);
    void setConfigBlueBits(int bits);
    void setConfigAlphaBits(int bits);
    void setConfigDepthBits(int bits);
    void setConfigStencilBits(int bits);
    void setMultisampleEnabled(bool enabled);

    int getClientVersion() const;
    int getWindowWidth() const;
    int getWindowHeight() const;
    int getConfigRedBits() const;
    int getConfigGreenBits() const;
    int getConfigBlueBits() const;
    int getConfigAlphaBits() const;
    int getConfigDepthBits() const;
    int getConfigStencilBits() const;
    bool isMultisampleEnabled() const;

  private:
    bool createEGLContext();
    bool destroyEGLContext();

    int mClientVersion;
    int mWidth;
    int mHeight;
    int mRedBits;
    int mGreenBits;
    int mBlueBits;
    int mAlphaBits;
    int mDepthBits;
    int mStencilBits;
    bool mMultisample;

    EGLConfig mConfig;
    EGLSurface mSurface;
    EGLContext mContext;

    static EGLDisplay mDisplay;
    static EGLNativeWindowType mNativeWindow;
    static EGLNativeDisplayType mNativeDisplay;
};

class ANGLETestEnvironment : public testing::Environment
{
  public:
    virtual void SetUp();
    virtual void TearDown();
};

#endif  // ANGLE_TESTS_ANGLE_TEST_H_
