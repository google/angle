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