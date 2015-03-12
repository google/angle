//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "gtest/gtest.h"

template <int clientVersion>
class EGLQueryContextTest : public testing::Test
{
  public:
    virtual void SetUp()
    {
        PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
        EXPECT_TRUE(eglGetPlatformDisplayEXT != NULL);

        EGLint dispattrs[] =
        {
            EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
            EGL_NONE
        };
        mDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_ANGLE_ANGLE, EGL_DEFAULT_DISPLAY, dispattrs);
        EXPECT_TRUE(mDisplay != EGL_NO_DISPLAY);
        EXPECT_TRUE(eglInitialize(mDisplay, NULL, NULL) != EGL_FALSE);

        EGLint ncfg;
        EGLint cfgattrs[] =
        {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_RENDERABLE_TYPE, clientVersion == 3 ? EGL_OPENGL_ES3_BIT : EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
            EGL_NONE
        };
        EXPECT_TRUE(eglChooseConfig(mDisplay, cfgattrs, &mConfig, 1, &ncfg) != EGL_FALSE);
        EXPECT_TRUE(ncfg == 1);

        EGLint ctxattrs[] =
        {
            EGL_CONTEXT_CLIENT_VERSION, clientVersion,
            EGL_NONE
        };
        mContext = eglCreateContext(mDisplay, mConfig, NULL, ctxattrs);
        EXPECT_TRUE(mContext != EGL_NO_CONTEXT);

        EGLint surfattrs[] =
        {
            EGL_WIDTH, 16,
            EGL_HEIGHT, 16,
            EGL_NONE
        };
        mSurface = eglCreatePbufferSurface(mDisplay, mConfig, surfattrs);
        EXPECT_TRUE(mSurface != EGL_NO_SURFACE);
    }

    virtual void TearDown()
    {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(mDisplay, mContext);
        eglDestroySurface(mDisplay, mSurface);
        eglTerminate(mDisplay);
    }

    EGLDisplay mDisplay;
    EGLConfig mConfig;
    EGLContext mContext;
    EGLSurface mSurface;
};

typedef EGLQueryContextTest<2> EGLQueryContextTestES2;

typedef EGLQueryContextTest<3> EGLQueryContextTestES3;


TEST_F(EGLQueryContextTestES2, GetConfigID)
{
    EGLint configId, contextConfigId;
    EXPECT_TRUE(eglGetConfigAttrib(mDisplay, mConfig, EGL_CONFIG_ID, &configId) != EGL_FALSE);
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_CONFIG_ID, &contextConfigId) != EGL_FALSE);
    EXPECT_TRUE(configId == contextConfigId);
}

TEST_F(EGLQueryContextTestES2, GetClientType)
{
    EGLint clientType;
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_CONTEXT_CLIENT_TYPE, &clientType) != EGL_FALSE);
    EXPECT_TRUE(clientType == EGL_OPENGL_ES_API);
}

TEST_F(EGLQueryContextTestES2, GetClientVersion)
{
    EGLint clientVersion;
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_CONTEXT_CLIENT_VERSION, &clientVersion) != EGL_FALSE);
    EXPECT_TRUE(clientVersion == 2);
}

TEST_F(EGLQueryContextTestES2, GetRenderBufferNoSurface)
{
    EGLint renderBuffer;
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_RENDER_BUFFER, &renderBuffer) != EGL_FALSE);
    EXPECT_TRUE(renderBuffer == EGL_NONE);
}

TEST_F(EGLQueryContextTestES2, GetRenderBufferBoundSurface)
{
    EGLint renderBuffer, contextRenderBuffer;
    EXPECT_TRUE(eglQuerySurface(mDisplay, mSurface, EGL_RENDER_BUFFER, &renderBuffer) != EGL_FALSE);
    EXPECT_TRUE(eglMakeCurrent(mDisplay, mSurface, mSurface, mContext) != EGL_FALSE);
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_RENDER_BUFFER, &contextRenderBuffer) != EGL_FALSE);
    EXPECT_TRUE(renderBuffer == contextRenderBuffer);
}

TEST_F(EGLQueryContextTestES2, BadDisplay)
{
    EGLint val;
    EXPECT_TRUE(eglQueryContext(EGL_NO_DISPLAY, mContext, EGL_CONTEXT_CLIENT_TYPE, &val) == EGL_FALSE);
    EXPECT_TRUE(eglGetError() == EGL_BAD_DISPLAY);
}

TEST_F(EGLQueryContextTestES2, NotInitialized)
{
    EGLint val;
    TearDown();
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_CONTEXT_CLIENT_TYPE, &val) == EGL_FALSE);
    EXPECT_TRUE(eglGetError() == EGL_NOT_INITIALIZED);

    mDisplay = EGL_NO_DISPLAY;
    mSurface = EGL_NO_SURFACE;
    mContext = EGL_NO_CONTEXT;
}

TEST_F(EGLQueryContextTestES2, BadContext)
{
    EGLint val;
    EXPECT_TRUE(eglQueryContext(mDisplay, EGL_NO_CONTEXT, EGL_CONTEXT_CLIENT_TYPE, &val) == EGL_FALSE);
    EXPECT_TRUE(eglGetError() == EGL_BAD_CONTEXT);
}

TEST_F(EGLQueryContextTestES2, BadAttribute)
{
    EGLint val;
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_HEIGHT, &val) == EGL_FALSE);
    EXPECT_TRUE(eglGetError() == EGL_BAD_ATTRIBUTE);
}

TEST_F(EGLQueryContextTestES3, GetClientVersion)
{
    EGLint clientVersion;
    EXPECT_TRUE(eglQueryContext(mDisplay, mContext, EGL_CONTEXT_CLIENT_VERSION, &clientVersion) != EGL_FALSE);
    EXPECT_TRUE(clientVersion == 3);
}
