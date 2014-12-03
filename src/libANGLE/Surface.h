//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Surface.h: Defines the egl::Surface class, representing a drawing surface
// such as the client area of a window, including any back buffers.
// Implements EGLSurface and related functionality. [EGL 1.4] section 2.2 page 3.

#ifndef LIBANGLE_SURFACE_H_
#define LIBANGLE_SURFACE_H_

#include "common/angleutils.h"
#include "libANGLE/Error.h"

#include <EGL/egl.h>

namespace gl
{
class Texture2D;
}

namespace rx
{
class SurfaceImpl;
}

namespace egl
{
class Display;
class Config;

class Surface final
{
  public:
    Surface(rx::SurfaceImpl *impl);
    ~Surface();

    rx::SurfaceImpl *getImplementation() const { return mImplementation; }

    Error initialize();
    Error swap();
    Error postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height);
    Error querySurfacePointerANGLE(EGLint attribute, void **value);
    Error bindTexImage(gl::Texture2D *texture, EGLint buffer);
    Error releaseTexImage(EGLint buffer);

    EGLNativeWindowType getWindowHandle() const;

    EGLint isPostSubBufferSupported() const;

    void setSwapInterval(EGLint interval);

    EGLint getConfigID() const;

    // width and height can change with client window resizing
    EGLint getWidth() const;
    EGLint getHeight() const;
    EGLint getPixelAspectRatio() const;
    EGLenum getRenderBuffer() const;
    EGLenum getSwapBehavior() const;
    EGLenum getTextureFormat() const;
    EGLenum getTextureTarget() const;
    EGLenum getFormat() const;

    gl::Texture2D *getBoundTexture() const { return mTexture; }

    EGLint isFixedSize() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Surface);

    rx::SurfaceImpl *mImplementation;

    EGLint mPixelAspectRatio;      // Display aspect ratio
    EGLenum mRenderBuffer;         // Render buffer
    EGLenum mSwapBehavior;         // Buffer swap behavior

    gl::Texture2D *mTexture;
};

}

#endif   // LIBANGLE_SURFACE_H_
