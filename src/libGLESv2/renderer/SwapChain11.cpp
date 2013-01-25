//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
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

#include "libGLESv2/renderer/shaders/compiled/passthrough11vs.h"
#include "libGLESv2/renderer/shaders/compiled/passthrough11ps.h"

namespace rx
{

struct QuadVertex
{
    float x, y;
    float u, v;
};

static void setVertex(QuadVertex* vertex, float x, float y, float u, float v)
{
    vertex->x = x;
    vertex->y = y;
    vertex->u = u;
    vertex->v = v;
}

SwapChain11::SwapChain11(Renderer11 *renderer, HWND window, HANDLE shareHandle,
                         GLenum backBufferFormat, GLenum depthBufferFormat)
    : mRenderer(renderer), SwapChain(window, shareHandle, backBufferFormat, depthBufferFormat)
{
    mSwapChain = NULL;
    mBackBufferTexture = NULL;
    mBackBufferRTView = NULL;
    mOffscreenTexture = NULL;
    mOffscreenRTView = NULL;
    mOffscreenSRView = NULL;
    mDepthStencilTexture = NULL;
    mDepthStencilDSView = NULL;
    mQuadVB = NULL;
    mPassThroughSampler = NULL;
    mPassThroughIL = NULL;
    mPassThroughVS = NULL;
    mPassThroughPS = NULL;
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

    if (mBackBufferTexture)
    {
        mBackBufferTexture->Release();
        mBackBufferTexture = NULL;
    }

    if (mBackBufferRTView)
    {
        mBackBufferRTView->Release();
        mBackBufferRTView = NULL;
    }

    if (mOffscreenTexture)
    {
        mOffscreenTexture->Release();
        mOffscreenTexture = NULL;
    }

    if (mOffscreenRTView)
    {
        mOffscreenRTView->Release();
        mOffscreenRTView = NULL;
    }

    if (mOffscreenSRView)
    {
        mOffscreenSRView->Release();
        mOffscreenSRView = NULL;
    }

    if (mDepthStencilTexture)
    {
        mDepthStencilTexture->Release();
        mDepthStencilTexture = NULL;
    }

    if (mDepthStencilDSView)
    {
        mDepthStencilDSView->Release();
        mDepthStencilDSView = NULL;
    }

    if (mQuadVB)
    {
        mQuadVB->Release();
        mQuadVB = NULL;
    }

    if (mPassThroughSampler)
    {
        mPassThroughSampler->Release();
        mPassThroughSampler = NULL;
    }

    if (mPassThroughIL)
    {
        mPassThroughIL->Release();
        mPassThroughIL = NULL;
    }

    if (mPassThroughVS)
    {
        mPassThroughVS->Release();
        mPassThroughVS = NULL;
    }

    if (mPassThroughPS)
    {
        mPassThroughPS->Release();
        mPassThroughPS = NULL;
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

    if (mBackBufferTexture)
    {
        mBackBufferTexture->Release();
        mBackBufferTexture = NULL;
    }

    if (mBackBufferRTView)
    {
        mBackBufferRTView->Release();
        mBackBufferRTView = NULL;
    }

    if (mOffscreenTexture)
    {
        mOffscreenTexture->Release();
        mOffscreenTexture = NULL;
    }

    if (mOffscreenRTView)   // TODO: Preserve the render target content
    {
        mOffscreenRTView->Release();
        mOffscreenRTView = NULL;
    }

    if (mOffscreenSRView)
    {
        mOffscreenSRView->Release();
        mOffscreenSRView = NULL;
    }

    if (mDepthStencilTexture)
    {
        mDepthStencilTexture->Release();
        mDepthStencilTexture = NULL;
    }

    if (mDepthStencilDSView)
    {
        mDepthStencilDSView->Release();
        mDepthStencilDSView = NULL;
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
    offscreenTextureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    offscreenTextureDesc.CPUAccessFlags = 0;
    offscreenTextureDesc.MiscFlags = 0;   // D3D11_RESOURCE_MISC_SHARED

    HRESULT result = device->CreateTexture2D(&offscreenTextureDesc, NULL, &mOffscreenTexture);
    d3d11::SetDebugName(mOffscreenTexture, "Offscreen texture");

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
        
    result = device->CreateRenderTargetView(mOffscreenTexture, NULL, &mOffscreenRTView);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mOffscreenRTView, "Offscreen render target");

    result = device->CreateShaderResourceView(mOffscreenTexture, NULL, &mOffscreenSRView);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mOffscreenSRView, "Offscreen shader resource");

    if (mWindow)
    {
        IDXGIFactory *factory = mRenderer->getDxgiFactory();

        DXGI_SWAP_CHAIN_DESC swapChainDesc = {0};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.BufferDesc.Format = gl_d3d11::ConvertRenderbufferFormat(mBackBufferFormat);
        swapChainDesc.BufferDesc.Width = backbufferWidth;
        swapChainDesc.BufferDesc.Height = backbufferHeight;
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

        result = mSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&mBackBufferTexture);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mBackBufferTexture, "Back buffer texture");

        result = device->CreateRenderTargetView(mBackBufferTexture, NULL, &mBackBufferRTView);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mBackBufferRTView, "Back buffer render target");
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

        result = device->CreateTexture2D(&depthStencilDesc, NULL, &mDepthStencilTexture);
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
        d3d11::SetDebugName(mDepthStencilTexture, "Depth stencil texture");

        result = device->CreateDepthStencilView(mDepthStencilTexture, NULL, &mDepthStencilDSView);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mDepthStencilDSView, "Depth stencil view");
    }

    D3D11_BUFFER_DESC vbDesc;
    vbDesc.ByteWidth = sizeof(QuadVertex) * 4;
    vbDesc.Usage = D3D11_USAGE_DYNAMIC;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbDesc.MiscFlags = 0;
    vbDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&vbDesc, NULL, &mQuadVB);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mQuadVB, "Swap chain quad vertex buffer");

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 0;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = device->CreateSamplerState(&samplerDesc, &mPassThroughSampler);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mPassThroughSampler, "Swap chain pass through sampler");

    D3D11_INPUT_ELEMENT_DESC quadLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    result = device->CreateInputLayout(quadLayout, 2, g_VS_Passthrough, sizeof(g_VS_Passthrough), &mPassThroughIL);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mPassThroughIL, "Swap chain pass through layout");

    result = device->CreateVertexShader(g_VS_Passthrough, sizeof(g_VS_Passthrough), NULL, &mPassThroughVS);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mPassThroughVS, "Swap chain pass through vertex shader");

    result = device->CreatePixelShader(g_PS_Passthrough, sizeof(g_PS_Passthrough), NULL, &mPassThroughPS);
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mPassThroughPS, "Swap chain pass through pixel shader");

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
    ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();

    // Set vertices
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT result = deviceContext->Map(mQuadVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        return EGL_BAD_ACCESS;
    }

    QuadVertex *vertices = static_cast<QuadVertex*>(mappedResource.pData);

    // Create a quad in homogeneous coordinates
    float x1 = (x / mWidth) * 2.0f - 1.0f;
    float y1 = ((mHeight - y - height) / mHeight) * 2.0f - 1.0f;
    float x2 = ((x + width) / mWidth) * 2.0f - 1.0f;
    float y2 = ((mHeight - y) / mHeight) * 2.0f - 1.0f;

    float u1 = x / float(mWidth);
    float v1 = y / float(mHeight);
    float u2 = (x + width) / float(mWidth);
    float v2 = (y + height) / float(mHeight);

    setVertex(&vertices[0], x1, y1, u1, v1);
    setVertex(&vertices[1], x1, y2, u1, v2);
    setVertex(&vertices[2], x2, y1, u2, v1);
    setVertex(&vertices[3], x2, y2, u2, v2);

    deviceContext->Unmap(mQuadVB, 0);

    static UINT stride = sizeof(QuadVertex);
    static UINT startIdx = 0;
    deviceContext->IASetVertexBuffers(0, 1, &mQuadVB, &stride, &startIdx);

    // Apply state
    deviceContext->OMSetDepthStencilState(NULL, 0xFFFFFFFF);

    static const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    deviceContext->OMSetBlendState(NULL, blendFactor, 0xFFFFFFF);

    deviceContext->RSSetState(NULL);

    // Apply shaders
    deviceContext->IASetInputLayout(mPassThroughIL);
    deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    deviceContext->VSSetShader(mPassThroughVS, NULL, 0);
    deviceContext->PSSetShader(mPassThroughPS, NULL, 0);

    // Apply render targets
    deviceContext->OMSetRenderTargets(1, &mBackBufferRTView, NULL);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = mWidth;
    viewport.Height = mHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    deviceContext->RSSetViewports(1, &viewport);

    // Apply textures
    deviceContext->PSSetShaderResources(0, 1, &mOffscreenSRView);
    deviceContext->PSSetSamplers(0, 1, &mPassThroughSampler);

    // Draw
    deviceContext->Draw(4, 0);
    mSwapChain->Present(0, 0);

    // Unbind
    static ID3D11ShaderResourceView *const nullSRV = NULL;
    deviceContext->PSSetShaderResources(0, 1, &nullSRV);

    static ID3D11RenderTargetView *const nullRTV = NULL;
    deviceContext->OMSetRenderTargets(1, &nullRTV, NULL);

    mRenderer->markAllStateDirty();

    return EGL_SUCCESS;
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

// Increments refcount on view.
// caller must Release() the returned view
ID3D11RenderTargetView *SwapChain11::getRenderTarget()
{
    if (mOffscreenRTView)
    {
        mOffscreenRTView->AddRef();
    }

    return mOffscreenRTView;
}

// Increments refcount on view.
// caller must Release() the returned view
ID3D11ShaderResourceView *SwapChain11::getRenderTargetShaderResource()
{
    if (mOffscreenSRView)
    {
        mOffscreenSRView->AddRef();
    }

    return mOffscreenSRView;
}

// Increments refcount on view.
// caller must Release() the returned view
ID3D11DepthStencilView *SwapChain11::getDepthStencil()
{
    if (mDepthStencilDSView)
    {
        mDepthStencilDSView->AddRef();
    }

    return mDepthStencilDSView;
}

SwapChain11 *SwapChain11::makeSwapChain11(SwapChain *swapChain)
{
    ASSERT(dynamic_cast<rx::SwapChain11*>(swapChain) != NULL);
    return static_cast<rx::SwapChain11*>(swapChain);
}

}
