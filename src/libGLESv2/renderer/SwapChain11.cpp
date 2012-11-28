//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChain11.cpp: Implements a back-end specific class for the D3D11 swap chain.

#include "libGLESv2/renderer/SwapChain11.h"

#include "common/debug.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/renderer/renderer11_utils.h"
#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/main.h"

namespace rx
{

SwapChain11::SwapChain11(Renderer11 *renderer, HWND window, HANDLE shareHandle,
                         GLenum backBufferFormat, GLenum depthBufferFormat)
    : mRenderer(renderer), SwapChain(window, shareHandle, backBufferFormat, depthBufferFormat)
{
    mSwapChain = NULL;
    mBackBuffer = NULL;
    mBackBufferView = NULL;
    mRenderTargetView = NULL;
    mDepthStencil = NULL;
    mDepthStencilView = NULL;
    mOffscreenTexture = NULL;
    mWidth = -1;
    mHeight = -1;
}

SwapChain11::~SwapChain11()
{
    release();
}

void SwapChain11::release()
{
    if (mSwapChain)
    {
        mSwapChain->Release();
        mSwapChain = NULL;
    }

    if (mBackBuffer)
    {
        mBackBuffer->Release();
        mBackBuffer = NULL;
    }

    if (mBackBufferView)
    {
        mBackBufferView->Release();
        mBackBufferView = NULL;
    }
    
    if (mRenderTargetView)
    {
        mRenderTargetView->Release();
        mRenderTargetView = NULL;
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
        mDepthStencil = NULL;
    }

    if (mDepthStencilView)
    {
        mDepthStencilView->Release();
        mDepthStencilView = NULL;
    }

    if (mOffscreenTexture)
    {
        mOffscreenTexture->Release();
        mOffscreenTexture = NULL;
    }

    if (mWindow)
        mShareHandle = NULL;
}

EGLint SwapChain11::reset(int backbufferWidth, int backbufferHeight, EGLint swapInterval)
{
    ID3D11Device *device = mRenderer->getDevice();

    if (device == NULL)
    {
        return EGL_BAD_ACCESS;
    }

    // Release specific resources to free up memory for the new render target, while the
    // old render target still exists for the purpose of preserving its contents.
    if (mSwapChain)
    {
        mSwapChain->Release();
        mSwapChain = NULL;
    }

    if (mBackBuffer)
    {
        mBackBuffer->Release();
        mBackBuffer = NULL;
    }

    if (mBackBufferView)
    {
        mBackBufferView->Release();
        mBackBufferView = NULL;
    }

    if (mRenderTargetView)   // TODO: Preserve the render target content
    {
        mRenderTargetView->Release();
        mRenderTargetView = NULL;
    }

    if (mOffscreenTexture)
    {
        mOffscreenTexture->Release();
        mOffscreenTexture = NULL;
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
        mDepthStencil = NULL;
    }

    if (mDepthStencilView)
    {
        mDepthStencilView->Release();
        mDepthStencilView = NULL;
    }

    HANDLE *pShareHandle = NULL;
    if (!mWindow && mRenderer->getShareHandleSupport())
    {
        pShareHandle = &mShareHandle;
    }

    D3D11_TEXTURE2D_DESC offscreenTextureDesc = {0};
    offscreenTextureDesc.Width = backbufferWidth;
    offscreenTextureDesc.Height = backbufferHeight;
    offscreenTextureDesc.Format = gl_d3d11::ConvertRenderbufferFormat(mBackBufferFormat);
    offscreenTextureDesc.MipLevels = 1;
    offscreenTextureDesc.ArraySize = 1;
    offscreenTextureDesc.SampleDesc.Count = 1;
    offscreenTextureDesc.SampleDesc.Quality = 0;
    offscreenTextureDesc.Usage = D3D11_USAGE_DEFAULT;
    offscreenTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    offscreenTextureDesc.CPUAccessFlags = 0;
    offscreenTextureDesc.MiscFlags = 0;   // D3D11_RESOURCE_MISC_SHARED

    HRESULT result = device->CreateTexture2D(&offscreenTextureDesc, NULL, &mOffscreenTexture);

    if (FAILED(result))
    {
        ERR("Could not create offscreen texture: %08lX", result);
        release();

        if (isDeviceLostError(result))
        {
            return EGL_CONTEXT_LOST;
        }
        else
        {
            return EGL_BAD_ALLOC;
        }
    }
        
    result = device->CreateRenderTargetView(mOffscreenTexture, NULL, &mRenderTargetView);
    ASSERT(SUCCEEDED(result));

    if (mWindow)
    {
        IDXGIFactory *factory = mRenderer->getDxgiFactory();

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {0};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Format = gl_d3d11::ConvertRenderbufferFormat(mBackBufferFormat);
        swapChainDesc.BufferDesc.Width = 1;
        swapChainDesc.BufferDesc.Height = 1;
        swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
        swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.Flags = 0;
        swapChainDesc.OutputWindow = mWindow;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Windowed = TRUE;

        result = factory->CreateSwapChain(device, &swapChainDesc, &mSwapChain);

        if (FAILED(result))
        {
            ERR("Could not create additional swap chains or offscreen surfaces: %08lX", result);
            release();

            if (isDeviceLostError(result))
            {
                return EGL_CONTEXT_LOST;
            }
            else
            {
                return EGL_BAD_ALLOC;
            }
        }

        result = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&mBackBuffer);
        ASSERT(SUCCEEDED(result));

        result = device->CreateRenderTargetView(mBackBuffer, NULL, &mBackBufferView);
        ASSERT(SUCCEEDED(result));
    }

    if (mDepthBufferFormat != GL_NONE)
    {
        D3D11_TEXTURE2D_DESC depthStencilDesc = {0};
        depthStencilDesc.Width = backbufferWidth;
        depthStencilDesc.Height = backbufferHeight;
        depthStencilDesc.Format = gl_d3d11::ConvertRenderbufferFormat(mDepthBufferFormat);
        depthStencilDesc.MipLevels = 1;
        depthStencilDesc.ArraySize = 1;
        depthStencilDesc.SampleDesc.Count = 1;
        depthStencilDesc.SampleDesc.Quality = 0;
        depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
        depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthStencilDesc.CPUAccessFlags = 0;
        depthStencilDesc.MiscFlags = 0;

        result = device->CreateTexture2D(&depthStencilDesc, NULL, &mDepthStencil);

        if (FAILED(result))
        {
            ERR("Could not create depthstencil surface for new swap chain: 0x%08X", result);
            release();

            if (isDeviceLostError(result))
            {
                return EGL_CONTEXT_LOST;
            }
            else
            {
                return EGL_BAD_ALLOC;
            }
        }

        result = device->CreateDepthStencilView(mDepthStencil, NULL, &mDepthStencilView);
        ASSERT(SUCCEEDED(result));
    }

    mWidth = backbufferWidth;
    mHeight = backbufferHeight;

    return EGL_SUCCESS;
}

// parameters should be validated/clamped by caller
EGLint SwapChain11::swapRect(EGLint x, EGLint y, EGLint width, EGLint height)
{
    if (!mSwapChain)
    {
        return EGL_SUCCESS;
    }

    ID3D11Device *device = mRenderer->getDevice();

    // TODO
    UNIMPLEMENTED();

    return EGL_SUCCESS;
}

// Increments refcount on view.
// caller must Release() the returned view
ID3D11RenderTargetView *SwapChain11::getRenderTarget()
{
    if (mRenderTargetView)
    {
        mRenderTargetView->AddRef();
    }

    return mRenderTargetView;
}

// Increments refcount on view.
// caller must Release() the returned view
ID3D11DepthStencilView *SwapChain11::getDepthStencil()
{
    if (mDepthStencilView)
    {
        mDepthStencilView->AddRef();
    }

    return mDepthStencilView;
}

// Increments refcount on texture.
// caller must Release() the returned texture
ID3D11Texture2D *SwapChain11::getOffscreenTexture()
{
    if (mOffscreenTexture)
    {
        mOffscreenTexture->AddRef();
    }

    return mOffscreenTexture;
}

}
