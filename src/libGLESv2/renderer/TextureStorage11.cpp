#include "precompiled.h"
//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage11.cpp: Implements the abstract rx::TextureStorage11 class and its concrete derived
// classes TextureStorage11_2D and TextureStorage11_Cube, which act as the interface to the D3D11 texture.

#include "libGLESv2/renderer/TextureStorage11.h"

#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/renderer/RenderTarget11.h"
#include "libGLESv2/renderer/SwapChain11.h"
#include "libGLESv2/renderer/renderer11_utils.h"
#include "libGLESv2/renderer/Blit11.h"
#include "libGLESv2/renderer/formatutils11.h"

#include "common/utilities.h"
#include "libGLESv2/main.h"

namespace rx
{

TextureStorage11::TextureStorage11(Renderer *renderer, UINT bindFlags)
    : mBindFlags(bindFlags),
      mLodOffset(0),
      mMipLevels(0),
      mTextureFormat(DXGI_FORMAT_UNKNOWN),
      mShaderResourceFormat(DXGI_FORMAT_UNKNOWN),
      mRenderTargetFormat(DXGI_FORMAT_UNKNOWN),
      mDepthStencilFormat(DXGI_FORMAT_UNKNOWN),
      mSRV(NULL),
      mTextureWidth(0),
      mTextureHeight(0),
      mTextureDepth(0)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
}

TextureStorage11::~TextureStorage11()
{
}

TextureStorage11 *TextureStorage11::makeTextureStorage11(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11*, storage));
    return static_cast<TextureStorage11*>(storage);
}

DWORD TextureStorage11::GetTextureBindFlags(GLint internalFormat, GLuint clientVersion, GLenum glusage)
{
    UINT bindFlags = 0;

    if (gl_d3d11::GetSRVFormat(internalFormat, clientVersion) != DXGI_FORMAT_UNKNOWN)
    {
        bindFlags |= D3D11_BIND_SHADER_RESOURCE;
    }
    if (gl_d3d11::GetDSVFormat(internalFormat, clientVersion) != DXGI_FORMAT_UNKNOWN)
    {
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    if (gl_d3d11::GetRTVFormat(internalFormat, clientVersion) != DXGI_FORMAT_UNKNOWN &&
        glusage == GL_FRAMEBUFFER_ATTACHMENT_ANGLE)
    {
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    }

    return bindFlags;
}

UINT TextureStorage11::getBindFlags() const
{
    return mBindFlags;
}
int TextureStorage11::getLodOffset() const
{
    return mLodOffset;
}

bool TextureStorage11::isRenderTarget() const
{
    return (mBindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_DEPTH_STENCIL)) != 0;
}

bool TextureStorage11::isManaged() const
{
    return false;
}

int TextureStorage11::levelCount()
{
    int levels = 0;
    if (getBaseTexture())
    {
        levels = mMipLevels - getLodOffset();
    }
    return levels;
}

UINT TextureStorage11::getSubresourceIndex(int mipLevel, int layerTarget)
{
    UINT index = 0;
    if (getBaseTexture())
    {
        index = D3D11CalcSubresource(mipLevel, layerTarget, mMipLevels);
    }
    return index;
}

bool TextureStorage11::updateSubresourceLevel(ID3D11Resource *srcTexture, unsigned int sourceSubresource,
                                              int level, int layerTarget, GLint xoffset, GLint yoffset, GLint zoffset,
                                              GLsizei width, GLsizei height, GLsizei depth)
{
    if (srcTexture)
    {
        GLuint clientVersion = mRenderer->getCurrentClientVersion();

        gl::Extents texSize(std::max(mTextureWidth  >> level, 1U),
                            std::max(mTextureHeight >> level, 1U),
                            std::max(mTextureDepth  >> level, 1U));
        gl::Box copyArea(xoffset, yoffset, zoffset, width, height, depth);

        bool fullCopy = copyArea.x == 0 &&
                        copyArea.y == 0 &&
                        copyArea.z == 0 &&
                        copyArea.width  == texSize.width &&
                        copyArea.height == texSize.height &&
                        copyArea.depth  == texSize.depth;

        ID3D11Resource *dstTexture = getBaseTexture();
        unsigned int dstSubresource = getSubresourceIndex(level + mLodOffset, layerTarget);

        ASSERT(dstTexture);

        if (!fullCopy && (d3d11::GetDepthBits(mTextureFormat) > 0 || d3d11::GetStencilBits(mTextureFormat) > 0))
        {
            // CopySubresourceRegion cannot copy partial depth stencils, use the blitter instead
            Blit11 *blitter = mRenderer->getBlitter();

            return blitter->copyDepthStencil(srcTexture, sourceSubresource, copyArea, texSize,
                                             dstTexture, dstSubresource, copyArea, texSize,
                                             NULL);
        }
        else
        {
            D3D11_BOX srcBox;
            srcBox.left = copyArea.x;
            srcBox.top = copyArea.y;
            srcBox.right = copyArea.x + roundUp((unsigned int)width, d3d11::GetBlockWidth(mTextureFormat, clientVersion));
            srcBox.bottom = copyArea.y + roundUp((unsigned int)height, d3d11::GetBlockHeight(mTextureFormat, clientVersion));
            srcBox.front = copyArea.z;
            srcBox.back = copyArea.z + copyArea.depth;

            ID3D11DeviceContext *context = mRenderer->getDeviceContext();

            context->CopySubresourceRegion(dstTexture, dstSubresource, copyArea.x, copyArea.y, copyArea.z,
                                           srcTexture, sourceSubresource, fullCopy ? NULL : &srcBox);
            return true;
        }
    }

    return false;
}

void TextureStorage11::generateMipmapLayer(RenderTarget11 *source, RenderTarget11 *dest)
{
    if (source && dest)
    {
        ID3D11ShaderResourceView *sourceSRV = source->getShaderResourceView();
        ID3D11RenderTargetView *destRTV = dest->getRenderTargetView();

        if (sourceSRV && destRTV)
        {
            gl::Box sourceArea(0, 0, 0, source->getWidth(), source->getHeight(), source->getDepth());
            gl::Extents sourceSize(source->getWidth(), source->getHeight(), source->getDepth());

            gl::Box destArea(0, 0, 0, dest->getWidth(), dest->getHeight(), dest->getDepth());
            gl::Extents destSize(dest->getWidth(), dest->getHeight(), dest->getDepth());

            Blit11 *blitter = mRenderer->getBlitter();

            blitter->copyTexture(sourceSRV, sourceArea, sourceSize, destRTV, destArea, destSize, NULL,
                                 gl::GetFormat(source->getInternalFormat(), mRenderer->getCurrentClientVersion()),
                                 GL_LINEAR);
        }
    }
}

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, SwapChain11 *swapchain)
    : TextureStorage11(renderer, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
{
    mTexture = swapchain->getOffscreenTexture();
    mSRV = swapchain->getRenderTargetShaderResource();

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mRenderTarget[i] = NULL;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    mTexture->GetDesc(&texDesc);
    mMipLevels = texDesc.MipLevels;
    mTextureFormat = texDesc.Format;
    mTextureWidth = texDesc.Width;
    mTextureHeight = texDesc.Height;
    mTextureDepth = 1;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    mSRV->GetDesc(&srvDesc);
    mShaderResourceFormat = srvDesc.Format;

    ID3D11RenderTargetView* offscreenRTV = swapchain->getRenderTarget();
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    offscreenRTV->GetDesc(&rtvDesc);
    mRenderTargetFormat = rtvDesc.Format;
    SafeRelease(offscreenRTV);

    mDepthStencilFormat = DXGI_FORMAT_UNKNOWN;
}

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderer->getCurrentClientVersion(), usage))
{
    mTexture = NULL;
    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mRenderTarget[i] = NULL;
    }

    GLuint clientVersion = mRenderer->getCurrentClientVersion();

    mTextureFormat = gl_d3d11::GetTexFormat(internalformat, clientVersion);
    mShaderResourceFormat = gl_d3d11::GetSRVFormat(internalformat, clientVersion);
    mDepthStencilFormat = gl_d3d11::GetDSVFormat(internalformat, clientVersion);
    mRenderTargetFormat = gl_d3d11::GetRTVFormat(internalformat, clientVersion);

    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0)
    {
        // adjust size if needed for compressed textures
        d3d11::MakeValidSize(false, mTextureFormat, clientVersion, &width, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;      // Compressed texture size constraints?
        desc.Height = height;
        desc.MipLevels = (levels > 0) ? levels + mLodOffset : 0;
        desc.ArraySize = 1;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            gl::error(GL_OUT_OF_MEMORY);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            gl::error(GL_OUT_OF_MEMORY);
        }
        else
        {
            mTexture->GetDesc(&desc);
            mMipLevels = desc.MipLevels;
            mTextureWidth = desc.Width;
            mTextureHeight = desc.Height;
            mTextureDepth = 1;
        }
    }
}

TextureStorage11_2D::~TextureStorage11_2D()
{
    SafeRelease(mTexture);
    SafeRelease(mSRV);

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        SafeDelete(mRenderTarget[i]);
    }
}

TextureStorage11_2D *TextureStorage11_2D::makeTextureStorage11_2D(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_2D*, storage));
    return static_cast<TextureStorage11_2D*>(storage);
}

ID3D11Resource *TextureStorage11_2D::getBaseTexture() const
{
    return mTexture;
}

RenderTarget *TextureStorage11_2D::getRenderTarget(int level)
{
    if (level >= 0 && level < static_cast<int>(mMipLevels))
    {
        if (!mRenderTarget[level])
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mShaderResourceFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MostDetailedMip = level;
            srvDesc.Texture2D.MipLevels = 1;

            ID3D11ShaderResourceView *srv;
            result = device->CreateShaderResourceView(mTexture, &srvDesc, &srv);

            if (result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
            }
            ASSERT(SUCCEEDED(result));

            if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = mRenderTargetFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = level;

                ID3D11RenderTargetView *rtv;
                result = device->CreateRenderTargetView(mTexture, &rtvDesc, &rtv);

                if (result == E_OUTOFMEMORY)
                {
                    SafeRelease(srv);
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[level] = new RenderTarget11(mRenderer, rtv, mTexture, srv,
                                                          std::max(mTextureWidth >> level, 1U),
                                                          std::max(mTextureHeight >> level, 1U),
                                                          1);
            }
            else if (mDepthStencilFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
                dsvDesc.Format = mDepthStencilFormat;
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = level;
                dsvDesc.Flags = 0;

                ID3D11DepthStencilView *dsv;
                result = device->CreateDepthStencilView(mTexture, &dsvDesc, &dsv);

                if (result == E_OUTOFMEMORY)
                {
                    SafeRelease(srv);
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[level] = new RenderTarget11(mRenderer, dsv, mTexture, srv,
                                                          std::max(mTextureWidth >> level, 1U),
                                                          std::max(mTextureHeight >> level, 1U),
                                                          1);
            }
            else
            {
                UNREACHABLE();
            }
        }

        return mRenderTarget[level];
    }
    else
    {
        return NULL;
    }
}

ID3D11ShaderResourceView *TextureStorage11_2D::getSRV()
{
    if (!mSRV)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = (mMipLevels == 0 ? -1 : mMipLevels);
        srvDesc.Texture2D.MostDetailedMip = 0;

        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return gl::error(GL_OUT_OF_MEMORY, static_cast<ID3D11ShaderResourceView*>(NULL));
        }
        ASSERT(SUCCEEDED(result));
    }

    return mSRV;
}

void TextureStorage11_2D::generateMipmap(int level)
{
    RenderTarget11 *source = RenderTarget11::makeRenderTarget11(getRenderTarget(level - 1));
    RenderTarget11 *dest = RenderTarget11::makeRenderTarget11(getRenderTarget(level));

    generateMipmapLayer(source, dest);
}

TextureStorage11_Cube::TextureStorage11_Cube(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderer->getCurrentClientVersion(), usage))
{
    mTexture = NULL;
    for (unsigned int i = 0; i < 6; i++)
    {
        for (unsigned int j = 0; j < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; j++)
        {
            mRenderTarget[i][j] = NULL;
        }
    }

    GLuint clientVersion = mRenderer->getCurrentClientVersion();

    mTextureFormat = gl_d3d11::GetTexFormat(internalformat, clientVersion);
    mShaderResourceFormat = gl_d3d11::GetSRVFormat(internalformat, clientVersion);
    mDepthStencilFormat = gl_d3d11::GetDSVFormat(internalformat, clientVersion);
    mRenderTargetFormat = gl_d3d11::GetRTVFormat(internalformat, clientVersion);

    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (size > 0)
    {
        // adjust size if needed for compressed textures
        int height = size;
        d3d11::MakeValidSize(false, mTextureFormat, clientVersion, &size, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = size;
        desc.Height = size;
        desc.MipLevels = (levels > 0) ? levels + mLodOffset : 0;
        desc.ArraySize = 6;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            gl::error(GL_OUT_OF_MEMORY);
        }
        else
        {
            mTexture->GetDesc(&desc);
            mMipLevels = desc.MipLevels;
            mTextureWidth = desc.Width;
            mTextureHeight = desc.Height;
            mTextureDepth = 1;
        }
    }
}

TextureStorage11_Cube::~TextureStorage11_Cube()
{
    SafeRelease(mTexture);
    SafeRelease(mSRV);

    for (unsigned int i = 0; i < 6; i++)
    {
        for (unsigned int j = 0; j < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; j++)
        {
            SafeDelete(mRenderTarget[i][j]);
        }
    }
}

TextureStorage11_Cube *TextureStorage11_Cube::makeTextureStorage11_Cube(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_Cube*, storage));
    return static_cast<TextureStorage11_Cube*>(storage);
}

ID3D11Resource *TextureStorage11_Cube::getBaseTexture() const
{
    return mTexture;
}

RenderTarget *TextureStorage11_Cube::getRenderTargetFace(GLenum faceTarget, int level)
{
    unsigned int faceIdx = gl::TextureCubeMap::faceIndex(faceTarget);
    if (level >= 0 && level < static_cast<int>(mMipLevels))
    {
        if (!mRenderTarget[faceIdx][level])
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mShaderResourceFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.Texture2DArray.MostDetailedMip = level;
            srvDesc.Texture2DArray.MipLevels = 1;
            srvDesc.Texture2DArray.FirstArraySlice = faceIdx;
            srvDesc.Texture2DArray.ArraySize = 1;

            ID3D11ShaderResourceView *srv;
            result = device->CreateShaderResourceView(mTexture, &srvDesc, &srv);

            if (result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
            }
            ASSERT(SUCCEEDED(result));

            if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = mRenderTargetFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = level;
                rtvDesc.Texture2DArray.FirstArraySlice = faceIdx;
                rtvDesc.Texture2DArray.ArraySize = 1;

                ID3D11RenderTargetView *rtv;
                result = device->CreateRenderTargetView(mTexture, &rtvDesc, &rtv);

                if (result == E_OUTOFMEMORY)
                {
                    SafeRelease(srv);
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[faceIdx][level] = new RenderTarget11(mRenderer, rtv, mTexture, srv,
                                                                   std::max(mTextureWidth >> level, 1U),
                                                                   std::max(mTextureHeight >> level, 1U),
                                                                   1);
            }
            else if (mDepthStencilFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
                dsvDesc.Format = mDepthStencilFormat;
                dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Flags = 0;
                dsvDesc.Texture2DArray.MipSlice = level;
                dsvDesc.Texture2DArray.FirstArraySlice = faceIdx;
                dsvDesc.Texture2DArray.ArraySize = 1;

                ID3D11DepthStencilView *dsv;
                result = device->CreateDepthStencilView(mTexture, &dsvDesc, &dsv);

                if (result == E_OUTOFMEMORY)
                {
                    SafeRelease(srv);
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTarget[faceIdx][level] = new RenderTarget11(mRenderer, dsv, mTexture, srv,
                                                                   std::max(mTextureWidth >> level, 1U),
                                                                   std::max(mTextureHeight >> level, 1U),
                                                                   1);
            }
            else
            {
                UNREACHABLE();
            }
        }

        return mRenderTarget[faceIdx][level];
    }
    else
    {
        return NULL;
    }
}

ID3D11ShaderResourceView *TextureStorage11_Cube::getSRV()
{
    if (!mSRV)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = (mMipLevels == 0 ? -1 : mMipLevels);
        srvDesc.TextureCube.MostDetailedMip = 0;

        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return gl::error(GL_OUT_OF_MEMORY, static_cast<ID3D11ShaderResourceView*>(NULL));
        }
        ASSERT(SUCCEEDED(result));
    }

    return mSRV;
}

void TextureStorage11_Cube::generateMipmap(int face, int level)
{
    RenderTarget11 *source = RenderTarget11::makeRenderTarget11(getRenderTargetFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level - 1));
    RenderTarget11 *dest = RenderTarget11::makeRenderTarget11(getRenderTargetFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level));

    generateMipmapLayer(source, dest);
}

TextureStorage11_3D::TextureStorage11_3D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage,
                                         GLsizei width, GLsizei height, GLsizei depth)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderer->getCurrentClientVersion(), usage))
{
    mTexture = NULL;

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mLevelRenderTargets[i] = NULL;
    }

    GLuint clientVersion = mRenderer->getCurrentClientVersion();

    mTextureFormat = gl_d3d11::GetTexFormat(internalformat, clientVersion);
    mShaderResourceFormat = gl_d3d11::GetSRVFormat(internalformat, clientVersion);
    mDepthStencilFormat = gl_d3d11::GetDSVFormat(internalformat, clientVersion);
    mRenderTargetFormat = gl_d3d11::GetRTVFormat(internalformat, clientVersion);

    // If the width, height or depth are not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0 && depth > 0)
    {
        // adjust size if needed for compressed textures
        d3d11::MakeValidSize(false, mTextureFormat, clientVersion, &width, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE3D_DESC desc;
        desc.Width = width;
        desc.Height = height;
        desc.Depth = depth;
        desc.MipLevels = (levels > 0) ? levels + mLodOffset : 0;
        desc.Format = mTextureFormat;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture3D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            gl::error(GL_OUT_OF_MEMORY);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            gl::error(GL_OUT_OF_MEMORY);
        }
        else
        {
            mTexture->GetDesc(&desc);
            mMipLevels = desc.MipLevels;
            mTextureWidth = desc.Width;
            mTextureHeight = desc.Height;
            mTextureDepth = desc.Depth;
        }
    }
}

TextureStorage11_3D::~TextureStorage11_3D()
{
    SafeRelease(mTexture);
    SafeRelease(mSRV);

    for (RenderTargetMap::iterator i = mLevelLayerRenderTargets.begin(); i != mLevelLayerRenderTargets.end(); i++)
    {
        SafeDelete(i->second);
    }
    mLevelLayerRenderTargets.clear();

    for (unsigned int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        SafeDelete(mLevelRenderTargets[i]);
    }
}

TextureStorage11_3D *TextureStorage11_3D::makeTextureStorage11_3D(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_3D*, storage));
    return static_cast<TextureStorage11_3D*>(storage);
}

ID3D11Resource *TextureStorage11_3D::getBaseTexture() const
{
    return mTexture;
}

ID3D11ShaderResourceView *TextureStorage11_3D::getSRV()
{
    if (!mSRV)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MipLevels = (mMipLevels == 0 ? -1 : mMipLevels);
        srvDesc.Texture3D.MostDetailedMip = 0;

        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return gl::error(GL_OUT_OF_MEMORY, static_cast<ID3D11ShaderResourceView*>(NULL));
        }
        ASSERT(SUCCEEDED(result));
    }

    return mSRV;
}

RenderTarget *TextureStorage11_3D::getRenderTarget(int mipLevel)
{
    if (mipLevel >= 0 && mipLevel < static_cast<int>(mMipLevels))
    {
        if (!mLevelRenderTargets[mipLevel])
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mShaderResourceFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MostDetailedMip = mipLevel;
            srvDesc.Texture3D.MipLevels = 1;

            ID3D11ShaderResourceView *srv;
            result = device->CreateShaderResourceView(mTexture, &srvDesc, &srv);

            if (result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
            }
            ASSERT(SUCCEEDED(result));

            if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = mRenderTargetFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.MipSlice = mipLevel;
                rtvDesc.Texture3D.FirstWSlice = 0;
                rtvDesc.Texture3D.WSize = -1;

                ID3D11RenderTargetView *rtv;
                result = device->CreateRenderTargetView(mTexture, &rtvDesc, &rtv);

                if (result == E_OUTOFMEMORY)
                {
                    SafeRelease(srv);
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mLevelRenderTargets[mipLevel] = new RenderTarget11(mRenderer, rtv, mTexture, srv,
                                                                   std::max(mTextureWidth >> mipLevel, 1U),
                                                                   std::max(mTextureHeight >> mipLevel, 1U),
                                                                   std::max(mTextureDepth >> mipLevel, 1U));
            }
            else
            {
                UNREACHABLE();
            }
        }

        return mLevelRenderTargets[mipLevel];
    }
    else
    {
        return NULL;
    }
}

RenderTarget *TextureStorage11_3D::getRenderTargetLayer(int mipLevel, int layer)
{
    if (mipLevel >= 0 && mipLevel < static_cast<int>(mMipLevels))
    {
        LevelLayerKey key(mipLevel, layer);
        if (mLevelLayerRenderTargets.find(key) == mLevelLayerRenderTargets.end())
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            // TODO, what kind of SRV is expected here?
            ID3D11ShaderResourceView *srv = NULL;

            if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = mRenderTargetFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.MipSlice = mipLevel;
                rtvDesc.Texture3D.FirstWSlice = layer;
                rtvDesc.Texture3D.WSize = 1;

                ID3D11RenderTargetView *rtv;
                result = device->CreateRenderTargetView(mTexture, &rtvDesc, &rtv);

                if (result == E_OUTOFMEMORY)
                {
                    SafeRelease(srv);
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mLevelLayerRenderTargets[key] = new RenderTarget11(mRenderer, rtv, mTexture, srv,
                                                                   std::max(mTextureWidth >> mipLevel, 1U),
                                                                   std::max(mTextureHeight >> mipLevel, 1U),
                                                                   1);
            }
            else
            {
                UNREACHABLE();
            }
        }

        return mLevelLayerRenderTargets[key];
    }
    else
    {
        return NULL;
    }
}

void TextureStorage11_3D::generateMipmap(int level)
{
    RenderTarget11 *source = RenderTarget11::makeRenderTarget11(getRenderTarget(level - 1));
    RenderTarget11 *dest = RenderTarget11::makeRenderTarget11(getRenderTarget(level));

    generateMipmapLayer(source, dest);
}

TextureStorage11_2DArray::TextureStorage11_2DArray(Renderer *renderer, int levels, GLenum internalformat, GLenum usage,
                                                   GLsizei width, GLsizei height, GLsizei depth)
    : TextureStorage11(renderer, GetTextureBindFlags(internalformat, renderer->getCurrentClientVersion(), usage))
{
    mTexture = NULL;

    GLuint clientVersion = mRenderer->getCurrentClientVersion();

    mTextureFormat = gl_d3d11::GetTexFormat(internalformat, clientVersion);
    mShaderResourceFormat = gl_d3d11::GetSRVFormat(internalformat, clientVersion);
    mDepthStencilFormat = gl_d3d11::GetDSVFormat(internalformat, clientVersion);
    mRenderTargetFormat = gl_d3d11::GetRTVFormat(internalformat, clientVersion);

    // if the width, height or depth is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0 && depth > 0)
    {
        // adjust size if needed for compressed textures
        d3d11::MakeValidSize(false, mTextureFormat, clientVersion, &width, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = (levels > 0) ? levels + mLodOffset : 0;
        desc.ArraySize = depth;
        desc.Format = mTextureFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        // this can happen from windows TDR
        if (d3d11::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
            gl::error(GL_OUT_OF_MEMORY);
        }
        else if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            gl::error(GL_OUT_OF_MEMORY);
        }
        else
        {
            mTexture->GetDesc(&desc);
            mMipLevels = desc.MipLevels;
            mTextureWidth = desc.Width;
            mTextureHeight = desc.Height;
            mTextureDepth = desc.ArraySize;
        }
    }
}

TextureStorage11_2DArray::~TextureStorage11_2DArray()
{
    SafeRelease(mTexture);
    SafeRelease(mSRV);

    for (RenderTargetMap::iterator i = mRenderTargets.begin(); i != mRenderTargets.end(); i++)
    {
        SafeDelete(i->second);
    }
    mRenderTargets.clear();
}

TextureStorage11_2DArray *TextureStorage11_2DArray::makeTextureStorage11_2DArray(TextureStorage *storage)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureStorage11_2DArray*, storage));
    return static_cast<TextureStorage11_2DArray*>(storage);
}

ID3D11Resource *TextureStorage11_2DArray::getBaseTexture() const
{
    return mTexture;
}

ID3D11ShaderResourceView *TextureStorage11_2DArray::getSRV()
{
    if (!mSRV)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = mShaderResourceFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = (mMipLevels == 0 ? -1 : mMipLevels);
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.ArraySize = mTextureDepth;

        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return gl::error(GL_OUT_OF_MEMORY, static_cast<ID3D11ShaderResourceView*>(NULL));
        }
        ASSERT(SUCCEEDED(result));
    }

    return mSRV;
}

RenderTarget *TextureStorage11_2DArray::getRenderTargetLayer(int mipLevel, int layer)
{
    if (mipLevel >= 0 && mipLevel < static_cast<int>(mMipLevels))
    {
        LevelLayerKey key(mipLevel, layer);
        if (mRenderTargets.find(key) == mRenderTargets.end())
        {
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = mShaderResourceFormat;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
            srvDesc.Texture2DArray.MostDetailedMip = mipLevel;
            srvDesc.Texture2DArray.MipLevels = 1;
            srvDesc.Texture2DArray.FirstArraySlice = layer;
            srvDesc.Texture2DArray.ArraySize = 1;

            ID3D11ShaderResourceView *srv;
            result = device->CreateShaderResourceView(mTexture, &srvDesc, &srv);

            if (result == E_OUTOFMEMORY)
            {
                return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
            }
            ASSERT(SUCCEEDED(result));

            if (mRenderTargetFormat != DXGI_FORMAT_UNKNOWN)
            {
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = mRenderTargetFormat;
                rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = mipLevel;
                rtvDesc.Texture2DArray.FirstArraySlice = layer;
                rtvDesc.Texture2DArray.ArraySize = 1;

                ID3D11RenderTargetView *rtv;
                result = device->CreateRenderTargetView(mTexture, &rtvDesc, &rtv);

                if (result == E_OUTOFMEMORY)
                {
                    SafeRelease(srv);
                    return gl::error(GL_OUT_OF_MEMORY, static_cast<RenderTarget*>(NULL));
                }
                ASSERT(SUCCEEDED(result));

                // RenderTarget11 expects to be the owner of the resources it is given but TextureStorage11
                // also needs to keep a reference to the texture.
                mTexture->AddRef();

                mRenderTargets[key] = new RenderTarget11(mRenderer, rtv, mTexture, srv,
                                                         std::max(mTextureWidth >> mipLevel, 1U),
                                                         std::max(mTextureHeight >> mipLevel, 1U),
                                                         1);
            }
            else
            {
                UNREACHABLE();
            }
        }

        return mRenderTargets[key];
    }
    else
    {
        return NULL;
    }
}

void TextureStorage11_2DArray::generateMipmap(int level)
{
    for (unsigned int layer = 0; layer < mTextureDepth; layer++)
    {
        RenderTarget11 *source = RenderTarget11::makeRenderTarget11(getRenderTargetLayer(level - 1, layer));
        RenderTarget11 *dest = RenderTarget11::makeRenderTarget11(getRenderTargetLayer(level, layer));

        generateMipmapLayer(source, dest);
    }
}

}
