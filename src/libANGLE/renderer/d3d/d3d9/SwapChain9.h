//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain9.h: Defines a back-end specific class for the D3D9 swap chain.

#ifndef LIBANGLE_RENDERER_D3D_D3D9_SWAPCHAIN9_H_
#define LIBANGLE_RENDERER_D3D_D3D9_SWAPCHAIN9_H_

#include "libANGLE/renderer/d3d/SwapChainD3D.h"

#include "common/angleutils.h"

namespace rx
{
class Renderer9;

class SwapChain9 : public SwapChainD3D
{
  public:
    SwapChain9(Renderer9 *renderer, NativeWindow nativeWindow, HANDLE shareHandle,
               GLenum backBufferFormat, GLenum depthBufferFormat);
    virtual ~SwapChain9();

    EGLint resize(EGLint backbufferWidth, EGLint backbufferHeight);
    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval);
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height);
    virtual void recreate();

    virtual IDirect3DSurface9 *getRenderTarget();
    virtual IDirect3DSurface9 *getDepthStencil();
    virtual IDirect3DTexture9 *getOffscreenTexture();

    EGLint getWidth() const { return mWidth; }
    EGLint getHeight() const { return mHeight; }

    static SwapChain9 *makeSwapChain9(SwapChainD3D *swapChain);

  private:
    DISALLOW_COPY_AND_ASSIGN(SwapChain9);

    void release();

    Renderer9 *mRenderer;
    EGLint mHeight;
    EGLint mWidth;
    EGLint mSwapInterval;

    IDirect3DSwapChain9 *mSwapChain;
    IDirect3DSurface9 *mBackBuffer;
    IDirect3DSurface9 *mRenderTarget;
    IDirect3DSurface9 *mDepthStencil;
    IDirect3DTexture9* mOffscreenTexture;
};

}
#endif // LIBANGLE_RENDERER_D3D_D3D9_SWAPCHAIN9_H_
