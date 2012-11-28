//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain11.h: Defines a back-end specific class for the D3D11 swap chain.

#ifndef LIBGLESV2_RENDERER_SWAPCHAIN11_H_
#define LIBGLESV2_RENDERER_SWAPCHAIN11_H_

#include <d3d11.h>

#include "common/angleutils.h"
#include "libGLESv2/renderer/SwapChain.h"

namespace rx
{
class Renderer11;

class SwapChain11 : public SwapChain
{
  public:
    SwapChain11(Renderer11 *renderer, HWND window, HANDLE shareHandle,
                GLenum backBufferFormat, GLenum depthBufferFormat);
    virtual ~SwapChain11();

    virtual EGLint reset(EGLint backbufferWidth, EGLint backbufferHeight, EGLint swapInterval);
    virtual EGLint swapRect(EGLint x, EGLint y, EGLint width, EGLint height);

    virtual ID3D11RenderTargetView *getRenderTarget();
    virtual ID3D11DepthStencilView *getDepthStencil();
    virtual ID3D11Texture2D *getOffscreenTexture();

  private:
    DISALLOW_COPY_AND_ASSIGN(SwapChain11);

    void release();

    Renderer11 *mRenderer;
    EGLint mHeight;
    EGLint mWidth;

    IDXGISwapChain *mSwapChain;
    ID3D11Texture2D *mBackBuffer;
    ID3D11RenderTargetView *mBackBufferView;
    ID3D11RenderTargetView *mRenderTargetView;
    ID3D11Texture2D *mDepthStencil;
    ID3D11DepthStencilView *mDepthStencilView;
    ID3D11Texture2D *mOffscreenTexture;
};

}
#endif // LIBGLESV2_RENDERER_SWAPCHAIN11_H_
