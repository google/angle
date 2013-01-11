//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget11.cpp: Implements a DX11-specific wrapper for ID3D11View pointers
// retained by Renderbuffers.

#include "libGLESv2/renderer/RenderTarget11.h"
#include "libGLESv2/renderer/Renderer11.h"

#include "libGLESv2/renderer/renderer11_utils.h"
#include "libGLESv2/main.h"

namespace rx
{

RenderTarget11::RenderTarget11(Renderer *renderer, ID3D11RenderTargetView *view, GLsizei width, GLsizei height)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mRenderTarget = view;
    mDepthStencil = NULL;

    if (mRenderTarget)
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc;
        view->GetDesc(&desc);

        mWidth = width;
        mHeight = height;
        
        mInternalFormat = d3d11_gl::ConvertRenderbufferFormat(desc.Format);
        mActualFormat = d3d11_gl::ConvertRenderbufferFormat(desc.Format);
        mSamples = 1; // TEMP?
    }
}

RenderTarget11::RenderTarget11(Renderer *renderer, ID3D11DepthStencilView *view, GLsizei width, GLsizei height)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mRenderTarget = NULL;
    mDepthStencil = view;

    if (mDepthStencil)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        view->GetDesc(&desc);

        mWidth = width;
        mHeight = height;
        
        mInternalFormat = d3d11_gl::ConvertRenderbufferFormat(desc.Format);
        mActualFormat = d3d11_gl::ConvertRenderbufferFormat(desc.Format);
        mSamples = 1; // TEMP?
    }
}

RenderTarget11::RenderTarget11(Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples, bool depth)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mRenderTarget = NULL;
    mDepthStencil = NULL;

    DXGI_FORMAT requestedFormat = gl_d3d11::ConvertRenderbufferFormat(format);
    int supportedSamples = 1; // TODO - Multisample support query

    if (supportedSamples == -1)
    {
        error(GL_OUT_OF_MEMORY);

        return;
    }

    HRESULT result = D3DERR_INVALIDCALL;
    
    if (width > 0 && height > 0)
    {
        // Create texture resource
        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width; 
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = requestedFormat;
        desc.SampleDesc.Count = supportedSamples;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.BindFlags = (depth ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET);
        ID3D11Texture2D *rtTexture = NULL;

        ID3D11Device *device = mRenderer->getDevice();
        HRESULT result = device->CreateTexture2D(&desc, NULL, &rtTexture);

        if (SUCCEEDED(result))
        {
            if (depth)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
                dsvDesc.Format = requestedFormat;
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;
                result = device->CreateDepthStencilView(rtTexture, &dsvDesc, &mDepthStencil);
            }
            else
            {   
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = requestedFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;
                result = device->CreateRenderTargetView(rtTexture, &rtvDesc, &mRenderTarget);
            }

            rtTexture->Release();
        }

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            error(GL_OUT_OF_MEMORY);

            return;
        }

        ASSERT(SUCCEEDED(result));
    }

    mWidth = width;
    mHeight = height;
    mInternalFormat = format;
    mSamples = supportedSamples;
    mActualFormat = format;
}

RenderTarget11::~RenderTarget11()
{
    if (mRenderTarget)
    {
        mRenderTarget->Release();
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
    }
}

RenderTarget11 *RenderTarget11::makeRenderTarget11(RenderTarget *target)
{
    ASSERT(dynamic_cast<rx::RenderTarget11*>(target) != NULL);
    return static_cast<rx::RenderTarget11*>(target);
}

// Adds reference, caller must call Release
ID3D11RenderTargetView *RenderTarget11::getRenderTargetView() const
{
    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    return mRenderTarget;
}

// Adds reference, caller must call Release
ID3D11DepthStencilView *RenderTarget11::getDepthStencilView() const
{
    if (mDepthStencil)
    {
        mDepthStencil->AddRef();
    }

    return mDepthStencil;
}

}