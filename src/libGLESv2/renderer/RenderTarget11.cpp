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

static ID3D11Texture2D *getTextureResource(ID3D11View *view)
{
    ID3D11Resource *textureResource = NULL;
    view->GetResource(&textureResource);
    if (!textureResource)
    {
        return NULL;
    }

    ID3D11Texture2D *texture = NULL;
    HRESULT result = textureResource->QueryInterface(IID_ID3D11Texture2D, (void**)&texture);

    textureResource->Release();
    textureResource = NULL;

    if (FAILED(result))
    {
        return NULL;
    }

    return texture;
}

static unsigned int getRTVSubresourceIndex(ID3D11RenderTargetView *view)
{
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    view->GetDesc(&rtvDesc);

    ID3D11Texture2D *texture = getTextureResource(view);
    if (!texture)
    {
        ERR("Failed to extract the ID3D11Texture2D from the render target view.");
        return 0;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    texture->GetDesc(&texDesc);

    texture->Release();
    texture = NULL;

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;
    unsigned int mipLevels = texDesc.MipLevels;

    switch (rtvDesc.ViewDimension)
    {
      case D3D11_RTV_DIMENSION_TEXTURE1D:
        mipSlice = rtvDesc.Texture1D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
        mipSlice = rtvDesc.Texture1DArray.MipSlice;
        arraySlice = rtvDesc.Texture1DArray.FirstArraySlice;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2D:
        mipSlice = rtvDesc.Texture2D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
        mipSlice = rtvDesc.Texture2DArray.MipSlice;
        arraySlice = rtvDesc.Texture2DArray.FirstArraySlice;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2DMS:
        mipSlice = 0;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
        mipSlice = 0;
        arraySlice = rtvDesc.Texture2DMSArray.FirstArraySlice;
        break;

      case D3D11_RTV_DIMENSION_TEXTURE3D:
        mipSlice = rtvDesc.Texture3D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_RTV_DIMENSION_UNKNOWN:
      case D3D11_RTV_DIMENSION_BUFFER:
        UNIMPLEMENTED();
        break;

      default:
        UNREACHABLE();
        break;
    }

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

static unsigned int getDSVSubresourceIndex(ID3D11DepthStencilView *view)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    view->GetDesc(&dsvDesc);

    ID3D11Texture2D *texture = getTextureResource(view);
    if (!texture)
    {
        ERR("Failed to extract the ID3D11Texture2D from the depth stencil view.");
        return 0;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    texture->GetDesc(&texDesc);

    texture->Release();
    texture = NULL;

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;
    unsigned int mipLevels = texDesc.MipLevels;

    switch (dsvDesc.ViewDimension)
    {
      case D3D11_DSV_DIMENSION_TEXTURE1D:
        mipSlice = dsvDesc.Texture1D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:
        mipSlice = dsvDesc.Texture1DArray.MipSlice;
        arraySlice = dsvDesc.Texture1DArray.FirstArraySlice;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2D:
        mipSlice = dsvDesc.Texture2D.MipSlice;
        arraySlice = 0;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
        mipSlice = dsvDesc.Texture2DArray.MipSlice;
        arraySlice = dsvDesc.Texture2DArray.FirstArraySlice;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2DMS:
        mipSlice = 0;
        arraySlice = 0;
        break;

      case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
        mipSlice = 0;
        arraySlice = dsvDesc.Texture2DMSArray.FirstArraySlice;
        break;

      case D3D11_RTV_DIMENSION_UNKNOWN:
        UNIMPLEMENTED();
        break;

      default:
        UNREACHABLE();
        break;
    }

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

RenderTarget11::RenderTarget11(Renderer *renderer, ID3D11RenderTargetView *rtv, ID3D11ShaderResourceView *srv, GLsizei width, GLsizei height)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mRenderTarget = rtv;
    mDepthStencil = NULL;
    mShaderResource = srv;

    if (mRenderTarget)
    {
        D3D11_RENDER_TARGET_VIEW_DESC desc;
        mRenderTarget->GetDesc(&desc);

        mSubresourceIndex = getRTVSubresourceIndex(mRenderTarget);
        mWidth = width;
        mHeight = height;

        mInternalFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
        mActualFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
        mSamples = 1; // TEMP?
    }
}

RenderTarget11::RenderTarget11(Renderer *renderer, ID3D11DepthStencilView *dsv, ID3D11ShaderResourceView *srv, GLsizei width, GLsizei height)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mRenderTarget = NULL;
    mDepthStencil = dsv;
    mShaderResource = srv;

    if (mDepthStencil)
    {
        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        mDepthStencil->GetDesc(&desc);

        mSubresourceIndex = getDSVSubresourceIndex(mDepthStencil);
        mWidth = width;
        mHeight = height;

        mInternalFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
        mActualFormat = d3d11_gl::ConvertTextureInternalFormat(desc.Format);
        mSamples = 1; // TEMP?
    }
}

RenderTarget11::RenderTarget11(Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples, bool depth)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
    mRenderTarget = NULL;
    mDepthStencil = NULL;
    mShaderResource = NULL;

    DXGI_FORMAT requestedFormat = gl_d3d11::ConvertRenderbufferFormat(format);
    int supportedSamples = 0; // TODO - Multisample support query

    if (supportedSamples == -1)
    {
        gl::error(GL_OUT_OF_MEMORY);

        return;
    }

    HRESULT result = E_FAIL;
    
    if (width > 0 && height > 0)
    {
        // Create texture resource
        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width; 
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = requestedFormat;
        desc.SampleDesc.Count = (supportedSamples == 0 ? 1 : supportedSamples);
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.BindFlags = (depth ? D3D11_BIND_DEPTH_STENCIL : (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE));
        ID3D11Texture2D *rtTexture = NULL;

        ID3D11Device *device = mRenderer->getDevice();
        HRESULT result = device->CreateTexture2D(&desc, NULL, &rtTexture);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            gl::error(GL_OUT_OF_MEMORY);
            return;
        }
        ASSERT(SUCCEEDED(result));

        if (depth)
        {
            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = requestedFormat;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            dsvDesc.Texture2D.MipSlice = 0;
            dsvDesc.Flags = 0;
            result = device->CreateDepthStencilView(rtTexture, &dsvDesc, &mDepthStencil);

            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                rtTexture->Release();
                gl::error(GL_OUT_OF_MEMORY);
                return;
            }
            ASSERT(SUCCEEDED(result));
        }
        else
        {
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = requestedFormat;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;
            result = device->CreateRenderTargetView(rtTexture, &rtvDesc, &mRenderTarget);

            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                rtTexture->Release();
                gl::error(GL_OUT_OF_MEMORY);
                return;
            }
            ASSERT(SUCCEEDED(result));

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = requestedFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            result = device->CreateShaderResourceView(rtTexture, &srvDesc, &mShaderResource);

            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                rtTexture->Release();
                mRenderTarget->Release();
                gl::error(GL_OUT_OF_MEMORY);
                return;
            }
            ASSERT(SUCCEEDED(result));
        }
    }

    mWidth = width;
    mHeight = height;
    mInternalFormat = format;
    mSamples = supportedSamples;
    mActualFormat = format;
    mSubresourceIndex = D3D11CalcSubresource(0, 0, 1);
}

RenderTarget11::~RenderTarget11()
{
    if (mRenderTarget)
    {
        mRenderTarget->Release();
        mRenderTarget = NULL;
    }

    if (mDepthStencil)
    {
        mDepthStencil->Release();
        mDepthStencil = NULL;
    }

    if (mShaderResource)
    {
        mShaderResource->Release();
        mShaderResource = NULL;
    }
}

RenderTarget11 *RenderTarget11::makeRenderTarget11(RenderTarget *target)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::RenderTarget11*, target));
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

// Adds reference, caller must call Release
ID3D11ShaderResourceView *RenderTarget11::getShaderResourceView() const
{
    if (mShaderResource)
    {
        mShaderResource->AddRef();
    }

    return mShaderResource;
}

unsigned int RenderTarget11::getSubresourceIndex() const
{
    return mSubresourceIndex;
}

}
