//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChainD3D.h: Defines a back-end specific class that hides the details of the
// implementation-specific swapchain.

#ifndef LIBANGLE_RENDERER_D3D_SWAPCHAIND3D_H_
#define LIBANGLE_RENDERER_D3D_SWAPCHAIND3D_H_

// TODO: move SwapChain to be d3d only
#include "libANGLE/renderer/d3d/d3d11/NativeWindow.h"

#include "common/angleutils.h"
#include "common/platform.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

#if !defined(ANGLE_FORCE_VSYNC_OFF)
#define ANGLE_FORCE_VSYNC_OFF 0
#endif

namespace rx
{

class SwapChainD3D
{
  public:
    SwapChainD3D(rx::NativeWindow nativeWindow, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat)
        : mNativeWindow(nativeWindow), mShareHandle(shareHandle), mBackBufferFormat(backBufferFormat), mDepthBufferFormat(depthBufferFormat)
    {
    }

    virtual ~SwapChainD3D() {};

    virtual EGLint resize(EGLint backbufferWidth, EGLint backbufferSize) = 0;
    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval) = 0;
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height) = 0;
    virtual void recreate() = 0;

    GLenum GetBackBufferInternalFormat() const { return mBackBufferFormat; }
    GLenum GetDepthBufferInternalFormat() const { return mDepthBufferFormat; }

    virtual HANDLE getShareHandle() {return mShareHandle;};

  protected:
    rx::NativeWindow mNativeWindow;  // Handler for the Window that the surface is created for.
    const GLenum mBackBufferFormat;
    const GLenum mDepthBufferFormat;

    HANDLE mShareHandle;

  private:
    DISALLOW_COPY_AND_ASSIGN(SwapChainD3D);
};

}
#endif // LIBANGLE_RENDERER_D3D_SWAPCHAIND3D_H_
