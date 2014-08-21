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

// A macro to disallow the copy constructor and operator= functions
// This must be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);               \
    void operator=(const TypeName&)

class OSWindow;

class EGLWindow
{
  public:
    EGLWindow(size_t width, size_t height,
              EGLint glesMajorVersion = 2,
              EGLint requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE);

    ~EGLWindow();

    void swap();

    EGLConfig getConfig() const;
    EGLDisplay getDisplay() const;
    EGLSurface getSurface() const;
    EGLContext getContext() const;
    size_t getWidth() const { return mWidth; }
    size_t getHeight() const { return mHeight; }

    bool initializeGL(const OSWindow *osWindow);
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
};

#endif // UTIL_EGLWINDOW_H_
