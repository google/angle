//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SurfaceImpl.h: Implementation methods of egl::Surface

#ifndef LIBANGLE_RENDERER_SURFACEIMPL_H_
#define LIBANGLE_RENDERER_SURFACEIMPL_H_

#include "common/angleutils.h"
#include "libANGLE/Error.h"

namespace egl
{
class Display;
struct Config;
}

namespace rx
{

class SurfaceImpl
{
  public:
    SurfaceImpl(egl::Display *display, const egl::Config *config,
                EGLint fixedSize, EGLint postSubBufferSupported, EGLenum textureFormat,
                EGLenum textureType);
    virtual ~SurfaceImpl();

    virtual egl::Error initialize() = 0;
    virtual egl::Error swap() = 0;
    virtual egl::Error postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height) = 0;
    virtual egl::Error querySurfacePointerANGLE(EGLint attribute, void **value) = 0;
    virtual egl::Error bindTexImage(EGLint buffer) = 0;
    virtual egl::Error releaseTexImage(EGLint buffer) = 0;
    virtual void setSwapInterval(EGLint interval) = 0;

    // width and height can change with client window resizing
    virtual EGLint getWidth() const = 0;
    virtual EGLint getHeight() const = 0;

    const egl::Config *getConfig() const { return mConfig; }
    EGLint isFixedSize() const { return mFixedSize; }
    EGLenum getFormat() const;
    EGLint isPostSubBufferSupported() const { return mPostSubBufferSupported; }
    EGLenum getTextureFormat() const { return mTextureFormat; }
    EGLenum getTextureTarget() const { return mTextureTarget; }

  protected:
   // Useful for mocking
    SurfaceImpl()
        : mDisplay(nullptr),
          mConfig(nullptr),
          mFixedSize(0),
          mPostSubBufferSupported(0),
          mTextureFormat(EGL_NONE),
          mTextureTarget(EGL_NONE)
    {}

    egl::Display *const mDisplay;
    const egl::Config *mConfig;    // EGL config surface was created with

    EGLint mFixedSize;
    EGLint mPostSubBufferSupported;
//  EGLint horizontalResolution;   // Horizontal dot pitch
//  EGLint verticalResolution;     // Vertical dot pitch
//  EGLBoolean largestPBuffer;     // If true, create largest pbuffer possible
//  EGLBoolean mipmapTexture;      // True if texture has mipmaps
//  EGLint mipmapLevel;            // Mipmap level to render to
//  EGLenum multisampleResolve;    // Multisample resolve behavior
    EGLenum mTextureFormat;        // Format of texture: RGB, RGBA, or no texture
    EGLenum mTextureTarget;        // Type of texture: 2D or no texture
//  EGLenum vgAlphaFormat;         // Alpha format for OpenVG
//  EGLenum vgColorSpace;          // Color space for OpenVG

  private:
    DISALLOW_COPY_AND_ASSIGN(SurfaceImpl);
};

}

#endif // LIBANGLE_RENDERER_SURFACEIMPL_H_

