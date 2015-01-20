//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Surface.cpp: Implements the egl::Surface class, representing a drawing surface
// such as the client area of a window, including any back buffers.
// Implements EGLSurface and related functionality. [EGL 1.4] section 2.2 page 3.

#include "libANGLE/Surface.h"

#include "libANGLE/Config.h"
#include "libANGLE/Texture.h"
#include "libANGLE/renderer/SurfaceImpl.h"

namespace egl
{

Surface::Surface(rx::SurfaceImpl *impl)
    : mImplementation(impl),
      // FIXME: Determine actual pixel aspect ratio
      mPixelAspectRatio(static_cast<EGLint>(1.0 * EGL_DISPLAY_SCALING)),
      mRenderBuffer(EGL_BACK_BUFFER),
      mSwapBehavior(EGL_BUFFER_PRESERVED),
      mTexture(NULL)
{
}

Surface::~Surface()
{
    if (mTexture)
    {
        if (mImplementation)
        {
            mImplementation->releaseTexImage(mTexture->id());
        }
        mTexture->releaseTexImage();
        mTexture = NULL;
    }

    SafeDelete(mImplementation);
}

EGLNativeWindowType Surface::getWindowHandle() const
{
    return mImplementation->getWindowHandle();
}

Error Surface::swap()
{
    return mImplementation->swap();
}

Error Surface::postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height)
{
    return mImplementation->postSubBuffer(x, y, width, height);
}

Error Surface::querySurfacePointerANGLE(EGLint attribute, void **value)
{
    return mImplementation->querySurfacePointerANGLE(attribute, value);
}

EGLint Surface::isPostSubBufferSupported() const
{
    return mImplementation->isPostSubBufferSupported();
}

void Surface::setSwapInterval(EGLint interval)
{
    mImplementation->setSwapInterval(interval);
}

EGLint Surface::getConfigID() const
{
    return mImplementation->getConfig()->configID;
}

const Config *Surface::getConfig() const
{
    return mImplementation->getConfig();
}

EGLint Surface::getPixelAspectRatio() const
{
    return mPixelAspectRatio;
}

EGLenum Surface::getRenderBuffer() const
{
    return mRenderBuffer;
}

EGLenum Surface::getSwapBehavior() const
{
    return mSwapBehavior;
}

EGLenum Surface::getTextureFormat() const
{
    return mImplementation->getTextureFormat();
}

EGLenum Surface::getTextureTarget() const
{
    return mImplementation->getTextureTarget();
}

EGLint Surface::isFixedSize() const
{
    return mImplementation->isFixedSize();
}

EGLenum Surface::getFormat() const
{
    return mImplementation->getFormat();
}

EGLint Surface::getWidth() const
{
    return mImplementation->getWidth();
}

EGLint Surface::getHeight() const
{
    return mImplementation->getHeight();
}

Error Surface::bindTexImage(gl::Texture *texture, EGLint buffer)
{
    ASSERT(!mTexture);

    texture->bindTexImage(this);
    mTexture = texture;
    return mImplementation->bindTexImage(buffer);
}

Error Surface::releaseTexImage(EGLint buffer)
{
    ASSERT(mTexture);
    gl::Texture *boundTexture = mTexture;
    mTexture = NULL;

    boundTexture->releaseTexImage();
    return mImplementation->releaseTexImage(buffer);
}

}
