//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayImpl.h: Implementation methods of egl::Display

#ifndef LIBANGLE_RENDERER_DISPLAYIMPL_H_
#define LIBANGLE_RENDERER_DISPLAYIMPL_H_

#include "common/angleutils.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Error.h"

#include <set>

namespace egl
{
class Display;
class Config;
class Surface;
}

namespace rx
{
class SurfaceImpl;

class DisplayImpl
{
  public:
    DisplayImpl();
    virtual ~DisplayImpl();

    virtual SurfaceImpl *createWindowSurface(egl::Display *display, const egl::Config *config,
                                             EGLNativeWindowType window, EGLint fixedSize,
                                             EGLint width, EGLint height, EGLint postSubBufferSupported) = 0;
    virtual SurfaceImpl *createOffscreenSurface(egl::Display *display, const egl::Config *config,
                                                EGLClientBuffer shareHandle, EGLint width, EGLint height,
                                                EGLenum textureFormat, EGLenum textureTarget) = 0;
    virtual egl::Error restoreLostDevice() = 0;

    virtual bool isValidNativeWindow(EGLNativeWindowType window) const = 0;

    typedef std::set<egl::Surface*> SurfaceSet;
    const SurfaceSet &getSurfaceSet() const { return mSurfaceSet; }
    SurfaceSet &getSurfaceSet() { return mSurfaceSet; }

    void destroySurface(egl::Surface *surface);

    const egl::DisplayExtensions &getExtensions() const;

  protected:
    // Place the surface set here so it can be accessible for handling
    // context loss events. (It is shared between the Display and Impl.)
    SurfaceSet mSurfaceSet;

  private:
    DISALLOW_COPY_AND_ASSIGN(DisplayImpl);

    virtual void generateExtensions(egl::DisplayExtensions *outExtensions) const = 0;

    mutable bool mExtensionsInitialized;
    mutable egl::DisplayExtensions mExtensions;
};

}

#endif // LIBANGLE_RENDERER_DISPLAYIMPL_H_
