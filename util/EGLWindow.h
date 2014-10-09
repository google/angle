//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UTIL_EGLWINDOW_H_
#define UTIL_EGLWINDOW_H_

#define GL_GLEXT_PROTOTYPES

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <string>
#include <list>
#include <cstdint>
#include <memory>

#include "shared_utils.h"

class OSWindow;

class EGLWindow
{
  public:
    EGLWindow(size_t width, size_t height,
              EGLint glesMajorVersion,
              EGLint requestedRenderer);

    ~EGLWindow();

    void setClientVersion(EGLint glesMajorVersion) { mClientVersion = glesMajorVersion; }
    void setWidth(size_t width) { mWidth = width; }
    void setHeight(size_t height) { mHeight = height; }
    void setConfigRedBits(int bits) { mRedBits = bits; }
    void setConfigGreenBits(int bits) { mGreenBits = bits; }
    void setConfigBlueBits(int bits) { mBlueBits = bits; }
    void setConfigAlphaBits(int bits) { mAlphaBits = bits; }
    void setConfigDepthBits(int bits) { mDepthBits = bits; }
    void setConfigStencilBits(int bits) { mStencilBits = bits; }
    void setMultisample(bool multisample) { mMultisample = multisample; }
    void setSwapInterval(EGLint swapInterval) { mSwapInterval = swapInterval; }

    void swap();

    GLuint getClientVersion() const { return mClientVersion; }
    EGLint getRequestedRenderer() const { return mRequestedRenderer; }
    EGLConfig getConfig() const;
    EGLDisplay getDisplay() const;
    EGLSurface getSurface() const;
    EGLContext getContext() const;
    size_t getWidth() const { return mWidth; }
    size_t getHeight() const { return mHeight; }
    int getConfigRedBits() const { return mRedBits; }
    int getConfigGreenBits() const { return mGreenBits; }
    int getConfigBlueBits() const { return mBlueBits; }
    int getConfigAlphaBits() const { return mAlphaBits; }
    int getConfigDepthBits() const { return mDepthBits; }
    int getConfigStencilBits() const { return mStencilBits; }
    bool isMultisample() const { return mMultisample; }
    EGLint getSwapInterval() const { return mSwapInterval; }

    bool initializeGL(OSWindow *osWindow);
    void destroyGL();

  private:
    DISALLOW_COPY_AND_ASSIGN(EGLWindow);

    EGLConfig mConfig;
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;

    GLuint mClientVersion;
    EGLint mRequestedRenderer;
    size_t mWidth;
    size_t mHeight;
    int mRedBits;
    int mGreenBits;
    int mBlueBits;
    int mAlphaBits;
    int mDepthBits;
    int mStencilBits;
    bool mMultisample;
    EGLint mSwapInterval;
};

#endif // UTIL_EGLWINDOW_H_
