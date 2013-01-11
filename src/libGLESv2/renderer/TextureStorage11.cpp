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

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, SwapChain11 *swapchain)
    : TextureStorage11(renderer, D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)
{
    ID3D11Texture2D *surfaceTexture = swapchain->getOffscreenTexture();
    mTexture = surfaceTexture;

    D3D11_TEXTURE2D_DESC desc;
    surfaceTexture->GetDesc(&desc);

    initializeRenderTarget(desc.Format, desc.Width, desc.Height);
}

TextureStorage11_2D::TextureStorage11_2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage11(renderer, GetTextureBindFlags(gl_d3d11::ConvertTextureFormat(internalformat), usage, forceRenderable))
{
    mTexture = NULL;
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

void TextureStorage11_2D::generateMipmap(int level)
{
    // TODO
    UNIMPLEMENTED();
}

void TextureStorage11_2D::initializeRenderTarget(DXGI_FORMAT format, int width, int height)
{
    mRenderTarget = NULL;

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

            ASSERT(SUCCESS(result));

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

}