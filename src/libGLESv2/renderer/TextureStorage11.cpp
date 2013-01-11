//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
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

#include "libGLESv2/main.h"

namespace rx
{

TextureStorage11::TextureStorage11(Renderer *renderer, UINT bindFlags)
    : mBindFlags(bindFlags),
      mLodOffset(0)
{
    mRenderer = Renderer11::makeRenderer11(renderer);
}

TextureStorage11::~TextureStorage11()
{
}    

TextureStorage11 *TextureStorage11::makeTextureStorage11(TextureStorage *storage)
{
    ASSERT(dynamic_cast<TextureStorage11*>(storage) != NULL);
    return static_cast<TextureStorage11*>(storage);
}

DWORD TextureStorage11::GetTextureBindFlags(DXGI_FORMAT format, GLenum glusage, bool forceRenderable)
{
    UINT bindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    if (format == DXGI_FORMAT_D24_UNORM_S8_UINT)
    {
        bindFlags |= D3D11_BIND_DEPTH_STENCIL;
    }
    else if(forceRenderable || (TextureStorage11::IsTextureFormatRenderable(format) && (glusage == GL_FRAMEBUFFER_ATTACHMENT_ANGLE)))
    {
        bindFlags |= D3D11_BIND_RENDER_TARGET;
    }
    return bindFlags;
}

bool TextureStorage11::IsTextureFormatRenderable(DXGI_FORMAT format)
{
    switch(format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
      case DXGI_FORMAT_A8_UNORM:
      case DXGI_FORMAT_R32G32B32A32_FLOAT:
      case DXGI_FORMAT_R32G32B32_FLOAT:
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
      case DXGI_FORMAT_B8G8R8A8_UNORM:
      case DXGI_FORMAT_R8_UNORM:
      case DXGI_FORMAT_R8G8_UNORM:
      case DXGI_FORMAT_R16_FLOAT:
      case DXGI_FORMAT_R16G16_FLOAT:
        return true;
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC2_UNORM: 
      case DXGI_FORMAT_BC3_UNORM:
        return false;
      default:
        UNREACHABLE();
        return false;
    }
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

// TODO - We should store level count internally at creation time instead
// of making driver calls to determine it each time levelCount() is called.
int TextureStorage11::levelCount()
{
    int levels = 0;
    if (getBaseTexture())
    {
        D3D11_TEXTURE2D_DESC desc;
        getBaseTexture()->GetDesc(&desc);
        levels = desc.MipLevels - getLodOffset();
    }
    return levels;
}

// TODO - Once we're storing level count internally, we should no longer
// need to look up the texture description to determine the number of mip levels.
UINT TextureStorage11::getSubresourceIndex(int level, int faceIndex)
{
    UINT index = 0;
    if (getBaseTexture())
    {
        D3D11_TEXTURE2D_DESC desc;
        getBaseTexture()->GetDesc(&desc);
        index = D3D11CalcSubresource(level, faceIndex, desc.MipLevels);
    }
    return index;
}

bool TextureStorage11::updateSubresourceLevel(ID3D11Texture2D *srcTexture, int level, int face, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    if (srcTexture)
    {
        D3D11_BOX srcBox;
        srcBox.left = xoffset;
        srcBox.top = yoffset;
        srcBox.right = xoffset + width;
        srcBox.bottom = yoffset + height;
        srcBox.front = 0;
        srcBox.back = 1;

        ID3D11DeviceContext *context = mRenderer->getDeviceContext();
        
        ASSERT(getBaseTexture());        
        context->CopySubresourceRegion(getBaseTexture(), getSubresourceIndex(level, face), xoffset, yoffset, 0, srcTexture, 0, &srcBox);
        return true;
    }

    return false;
}

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, SwapChain11 *swapchain)
    : TextureStorage11(renderer, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
{
    ID3D11Texture2D *surfaceTexture = swapchain->getOffscreenTexture();
    mTexture = surfaceTexture;
    mSRV = NULL;
    mRenderTarget = NULL;

    D3D11_TEXTURE2D_DESC desc;
    surfaceTexture->GetDesc(&desc);

    initializeSRV(desc.Format, desc.MipLevels);
    initializeRenderTarget(desc.Format, desc.Width, desc.Height);
}

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage11(renderer, GetTextureBindFlags(gl_d3d11::ConvertTextureFormat(internalformat), usage, forceRenderable))
{
    mTexture = NULL;
    mSRV = NULL;
    mRenderTarget = NULL;
    DXGI_FORMAT format = gl_d3d11::ConvertTextureFormat(internalformat);
    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0)
    {
        // adjust size if needed for compressed textures
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &width, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = width;      // Compressed texture size constraints?
        desc.Height = height;
        desc.MipLevels = levels + mLodOffset;
        desc.ArraySize = 1;
        desc.Format = format;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = getBindFlags();
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &mTexture);

        if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            error(GL_OUT_OF_MEMORY);
        }
    }

    initializeSRV(format, levels + mLodOffset);
    initializeRenderTarget(format, width, height);
}

TextureStorage11_2D::~TextureStorage11_2D()
{
    if (mTexture)
        mTexture->Release();
}

TextureStorage11_2D *TextureStorage11_2D::makeTextureStorage11_2D(TextureStorage *storage)
{
    ASSERT(dynamic_cast<TextureStorage11_2D*>(storage) != NULL);
    return static_cast<TextureStorage11_2D*>(storage);
}

RenderTarget *TextureStorage11_2D::getRenderTarget() const
{
    return mRenderTarget;
}

ID3D11Texture2D *TextureStorage11_2D::getBaseTexture() const
{
    return mTexture;
}

ID3D11ShaderResourceView *TextureStorage11_2D::getSRV() const
{
    return mSRV;
}

void TextureStorage11_2D::generateMipmap(int level)
{
    // TODO
    UNIMPLEMENTED();
}

void TextureStorage11_2D::initializeRenderTarget(DXGI_FORMAT format, int width, int height)
{
    ASSERT(mRenderTarget == NULL);

    if (mTexture != NULL && isRenderTarget())
    {
        if (getBindFlags() & D3D11_BIND_RENDER_TARGET)
        {
            // Create render target view -- texture should already be created with 
            // BIND_RENDER_TARGET flag.
            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = format;
            rtvDesc.Texture2D.MipSlice = 0;

            ID3D11RenderTargetView *renderTargetView;
            ID3D11Device *device = mRenderer->getDevice();
            HRESULT result = device->CreateRenderTargetView(mTexture, &rtvDesc, &renderTargetView);

            if (result == E_OUTOFMEMORY)
                return;

            ASSERT(SUCCEEDED(result));

            mRenderTarget = new RenderTarget11(mRenderer, renderTargetView, width, height);
        }
        else if (getBindFlags() & D3D11_BIND_DEPTH_STENCIL)
        {
            // TODO
            UNIMPLEMENTED();
        }
        else
            UNREACHABLE();
    }
}

void TextureStorage11_2D::initializeSRV(DXGI_FORMAT format, int levels)
{
    ASSERT(mSRV == NULL);

    if (mTexture)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = levels;

        ID3D11Device *device = mRenderer->getDevice();
        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return error(GL_OUT_OF_MEMORY);
        }

        ASSERT(SUCCEEDED(result));
    }
}

TextureStorage11_Cube::TextureStorage11_Cube(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : TextureStorage11(renderer, GetTextureBindFlags(gl_d3d11::ConvertTextureFormat(internalformat), usage, forceRenderable))
{
    mTexture = NULL;
    mSRV = NULL;
    
    for (int i = 0; i < 6; ++i)
    {
        mRenderTarget[i] = NULL;
    }

    DXGI_FORMAT format = gl_d3d11::ConvertTextureFormat(internalformat);
    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (size > 0)
    {
        // adjust size if needed for compressed textures
        int height = size;
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &size, &height, &mLodOffset);

        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = size;
        desc.Height = size;
        desc.MipLevels = levels + mLodOffset;
        desc.ArraySize = 6;
        desc.Format = format;
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
            error(GL_OUT_OF_MEMORY);
        }
    }

    initializeSRV(format, levels + mLodOffset);
    initializeRenderTarget(format, size);
}

TextureStorage11_Cube::~TextureStorage11_Cube()
{
    if (mTexture)
        mTexture->Release();
}

TextureStorage11_Cube *TextureStorage11_Cube::makeTextureStorage11_Cube(TextureStorage *storage)
{
    ASSERT(dynamic_cast<TextureStorage11_Cube*>(storage) != NULL);
    return static_cast<TextureStorage11_Cube*>(storage);
}

RenderTarget *TextureStorage11_Cube::getRenderTarget(GLenum faceTarget) const
{
    return mRenderTarget[gl::TextureCubeMap::faceIndex(faceTarget)];
}

ID3D11Texture2D *TextureStorage11_Cube::getBaseTexture() const
{
    return mTexture;
}

ID3D11ShaderResourceView *TextureStorage11_Cube::getSRV() const
{
    return mSRV;
}

void TextureStorage11_Cube::generateMipmap(int face, int level)
{
    // TODO
    UNIMPLEMENTED();
}

void TextureStorage11_Cube::initializeRenderTarget(DXGI_FORMAT format, int size)
{
    if (mTexture != NULL && isRenderTarget())
    {
        if (getBindFlags() & D3D11_BIND_RENDER_TARGET)
        {
            // Create render target view -- texture should already be created with 
            // BIND_RENDER_TARGET flag.

            for (int i = 0; i < 6; ++i)
            {
                ASSERT(mRenderTarget[i] == NULL);
                D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
                rtvDesc.Format = format;
                rtvDesc.Texture2DArray.MipSlice = 0;
                rtvDesc.Texture2DArray.FirstArraySlice = i;
                rtvDesc.Texture2DArray.ArraySize = 1;

                ID3D11RenderTargetView *renderTargetView;
                ID3D11Device *device = mRenderer->getDevice();
                HRESULT result = device->CreateRenderTargetView(mTexture, &rtvDesc, &renderTargetView);

                if (result == E_OUTOFMEMORY)
                    return;

                ASSERT(SUCCEEDED(result));

                mRenderTarget[i] = new RenderTarget11(mRenderer, renderTargetView, size, size);
            }
        }
        else if (getBindFlags() & D3D11_BIND_DEPTH_STENCIL)
        {
            // TODO
            UNIMPLEMENTED();
        }
        else
            UNREACHABLE();
    }
}

void TextureStorage11_Cube::initializeSRV(DXGI_FORMAT format, int levels)
{
    ASSERT(mSRV == NULL);

    if (mTexture)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        srvDesc.Format = format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = levels;
        srvDesc.TextureCube.MostDetailedMip = 0;

        ID3D11Device *device = mRenderer->getDevice();
        HRESULT result = device->CreateShaderResourceView(mTexture, &srvDesc, &mSRV);

        if (result == E_OUTOFMEMORY)
        {
            return error(GL_OUT_OF_MEMORY);
        }

        ASSERT(SUCCEEDED(result));
    }
}

}