//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain.h: Defines a back-end specific class that hides the details of the
// implementation-specific swapchain.

#ifndef LIBGLESV2_RENDERER_SWAPCHAIN_H_
#define LIBGLESV2_RENDERER_SWAPCHAIN_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define EGLAPI
#include <EGL/egl.h>

#include <d3d9.h>  // D3D9_REPLACE

#include "common/angleutils.h"

namespace renderer
{
class Renderer9;   // D3D9_REPLACE

class SwapChain
{
  public:
    SwapChain(Renderer9 *renderer, HWND window, HANDLE shareHandle,
              GLenum backBufferFormat, GLenum depthBufferFormat);
    virtual ~SwapChain();

    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval);
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height);

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();
    virtual IDirect3DTexture9 *getOffscreenTexture();

    HANDLE getShareHandle() { return mShareHandle; }

  private:
    DISALLOW_COPY_AND_ASSIGN(SwapChain);

    void release();

    Renderer9 *mRenderer;   // D3D9_REPLACE
    EGLint mHeight;
    EGLint mWidth;

    const HWND mWindow;            // Window that the surface is created for.
    const GLenum mBackBufferFormat;
    const GLenum mDepthBufferFormat;
    IDirect3DSwapChain9 *mSwapChain;
    IDirect3DSurface9 *mBackBuffer;
    IDirect3DSurface9 *mDepthStencil;
    IDirect3DSurface9* mRenderTarget;
    IDirect3DTexture9* mOffscreenTexture;

    HANDLE mShareHandle;
};

}
#endif // LIBGLESV2_RENDERER_SWAPCHAIN_H_
