//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget11.cpp: Implements a DX11-specific wrapper for ID3D11View pointers
// retained by Renderbuffers.

#include "libGLESv2/renderer/d3d/d3d11/RenderTarget11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/renderer/d3d/d3d11/renderer11_utils.h"
#include "libGLESv2/renderer/d3d/d3d11/formatutils11.h"
#include "libGLESv2/main.h"

namespace rx
{

static bool getTextureProperties(ID3D11Resource *resource, unsigned int *mipLevels, unsigned int *samples)
{
    ID3D11Texture1D *texture1D = d3d11::DynamicCastComObject<ID3D11Texture1D>(resource);
    if (texture1D)
    {
        D3D11_TEXTURE1D_DESC texDesc;
        texture1D->GetDesc(&texDesc);
        SafeRelease(texture1D);

        *mipLevels = texDesc.MipLevels;
        *samples = 0;

        return true;
    }

    ID3D11Texture2D *texture2D = d3d11::DynamicCastComObject<ID3D11Texture2D>(resource);
    if (texture2D)
    {
        D3D11_TEXTURE2D_DESC texDesc;
        texture2D->GetDesc(&texDesc);
        SafeRelease(texture2D);

        *mipLevels = texDesc.MipLevels;
        *samples = texDesc.SampleDesc.Count > 1 ? texDesc.SampleDesc.Count : 0;

        return true;
    }

    ID3D11Texture3D *texture3D = d3d11::DynamicCastComObject<ID3D11Texture3D>(resource);
    if (texture3D)
    {
        D3D11_TEXTURE3D_DESC texDesc;
        texture3D->GetDesc(&texDesc);
        SafeRelease(texture3D);

        *mipLevels = texDesc.MipLevels;
        *samples = 0;

        return true;
    }

    return false;
}

static unsigned int getRTVSubresourceIndex(ID3D11Resource *resource, ID3D11RenderTargetView *view)
{
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    view->GetDesc(&rtvDesc);

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;

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

    unsigned int mipLevels, samples;
    getTextureProperties(resource,  &mipLevels, &samples);

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

static unsigned int getDSVSubresourceIndex(ID3D11Resource *resource, ID3D11DepthStencilView *view)
{
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    view->GetDesc(&dsvDesc);

    unsigned int mipSlice = 0;
    unsigned int arraySlice = 0;

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

      case D3D11_DSV_DIMENSION_UNKNOWN:
        UNIMPLEMENTED();
        break;

      default:
        UNREACHABLE();
        break;
    }

    unsigned int mipLevels, samples;
    getTextureProperties(resource, &mipLevels, &samples);

    return D3D11CalcSubresource(mipSlice, arraySlice, mipLevels);
}

RenderTarget11::RenderTarget11(ID3D11RenderTargetView *rtv, ID3D11Resource *resource, ID3D11ShaderResourceView *srv,
                               GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei samples)
    : mSubresourceIndex(0),
      mTexture(resource),
      mRenderTarget(rtv),
      mDepthStencil(NULL),
      mShaderResource(srv)
{
    if (mTexture)
    {
        mTexture->AddRef();
    }

    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    if (mShaderResource)
    {
        mShaderResource->AddRef();
    }

    mWidth = width;
    mHeight = height;
    mDepth = depth;
    mSamples = samples;
    mInternalFormat = internalFormat;
    mActualFormat = internalFormat;

    if (mRenderTarget && mTexture)
    {
        mSubresourceIndex = getRTVSubresourceIndex(mTexture, mRenderTarget);

        D3D11_RENDER_TARGET_VIEW_DESC desc;
        mRenderTarget->GetDesc(&desc);

        const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(desc.Format);
        mActualFormat = dxgiFormatInfo.internalFormat;
    }
}

RenderTarget11::RenderTarget11(ID3D11DepthStencilView *dsv, ID3D11Resource *resource, ID3D11ShaderResourceView *srv,
                               GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei samples)
    : mSubresourceIndex(0),
      mTexture(resource),
      mRenderTarget(NULL),
      mDepthStencil(dsv),
      mShaderResource(srv)
{
    if (mTexture)
    {
        mTexture->AddRef();
    }

    if (mDepthStencil)
    {
        mDepthStencil->AddRef();
    }

    if (mShaderResource)
    {
        mShaderResource->AddRef();
    }

    mWidth = width;
    mHeight = height;
    mDepth = depth;
    mSamples = samples;
    mInternalFormat = internalFormat;
    mActualFormat = internalFormat;

    if (mDepthStencil && mTexture)
    {
        mSubresourceIndex = getDSVSubresourceIndex(mTexture, mDepthStencil);

        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        mDepthStencil->GetDesc(&desc);

        const d3d11::DXGIFormat &dxgiFormatInfo = d3d11::GetDXGIFormatInfo(desc.Format);
        mActualFormat = dxgiFormatInfo.internalFormat;
    }
}

RenderTarget11::~RenderTarget11()
{
    SafeRelease(mTexture);
    SafeRelease(mRenderTarget);
    SafeRelease(mDepthStencil);
    SafeRelease(mShaderResource);
}

RenderTarget11 *RenderTarget11::makeRenderTarget11(RenderTarget *target)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::RenderTarget11*, target));
    return static_cast<rx::RenderTarget11*>(target);
}

void RenderTarget11::invalidate(GLint x, GLint y, GLsizei width, GLsizei height)
{
    // Currently a no-op
}

ID3D11Resource *RenderTarget11::getTexture() const
{
    return mTexture;
}

ID3D11RenderTargetView *RenderTarget11::getRenderTargetView() const
{
    return mRenderTarget;
}

ID3D11DepthStencilView *RenderTarget11::getDepthStencilView() const
{
    return mDepthStencil;
}

ID3D11ShaderResourceView *RenderTarget11::getShaderResourceView() const
{
    return mShaderResource;
}

unsigned int RenderTarget11::getSubresourceIndex() const
{
    return mSubresourceIndex;
}

}
