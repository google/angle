//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayD3D.h: D3D implementation of egl::Display

#ifndef LIBANGLE_RENDERER_D3D_DISPLAYD3D_H_
#define LIBANGLE_RENDERER_D3D_DISPLAYD3D_H_

#include "libANGLE/renderer/DisplayImpl.h"

namespace rx
{
class RendererD3D;

class DisplayD3D : public DisplayImpl
{
  public:
    DisplayD3D(rx::RendererD3D *renderer);
    SurfaceImpl *createWindowSurface(egl::Display *display, const egl::Config *config,
                                     EGLNativeWindowType window, EGLint fixedSize,
                                     EGLint width, EGLint height, EGLint postSubBufferSupported) override;
    SurfaceImpl *createOffscreenSurface(egl::Display *display, const egl::Config *config,
                                        EGLClientBuffer shareHandle, EGLint width, EGLint height,
                                        EGLenum textureFormat, EGLenum textureTarget) override;
    egl::Error restoreLostDevice() override;

    bool isValidNativeWindow(EGLNativeWindowType window) const override;

  private:
    DISALLOW_COPY_AND_ASSIGN(DisplayD3D);

    void generateExtensions(egl::DisplayExtensions *outExtensions) const override;

    rx::RendererD3D *mRenderer;
};

}

#endif // LIBANGLE_RENDERER_D3D_DISPLAYD3D_H_
