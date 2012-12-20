//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.cpp: Implements the abstract rx::TextureStorage class and its concrete derived
// classes TextureStorage2D and TextureStorageCubeMap, which act as the interface to the
// D3D-side texture.

#include "libGLESv2/main.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "libGLESv2/renderer/SwapChain9.h"
#include "libGLESv2/renderer/Blit.h"
#include "libGLESv2/renderer/RenderTarget9.h"
#include "libGLESv2/renderer/renderer9_utils.h"

#include "common/debug.h"

namespace rx
{
unsigned int TextureStorage::mCurrentTextureSerial = 1;

TextureStorage::TextureStorage(Renderer *renderer, DWORD usage)
    : mLodOffset(0),
      mRenderer(Renderer9::makeRenderer9(renderer)),
      mD3DUsage(usage),
      mD3DPool(mRenderer->getTexturePool(usage)),
      mTextureSerial(issueTextureSerial())
{
}

TextureStorage::~TextureStorage()
{
}

DWORD TextureStorage::GetTextureUsage(D3DFORMAT d3dfmt, GLenum glusage, bool forceRenderable)
{
    DWORD d3dusage = 0;

    if (d3dfmt == D3DFMT_INTZ)
    {
        d3dusage |= D3DUSAGE_DEPTHSTENCIL;
    }
    else if(forceRenderable || (TextureStorage::IsTextureFormatRenderable(d3dfmt) && (glusage == GL_FRAMEBUFFER_ATTACHMENT_ANGLE)))
    {
        d3dusage |= D3DUSAGE_RENDERTARGET;
    }
    return d3dusage;
}

bool TextureStorage::IsTextureFormatRenderable(D3DFORMAT format)
{
    if (format == D3DFMT_INTZ)
    {
        return true;
    }
    switch(format)
    {
      case D3DFMT_L8:
      case D3DFMT_A8L8:
      case D3DFMT_DXT1:
      case D3DFMT_DXT3:
      case D3DFMT_DXT5:
        return false;
      case D3DFMT_A8R8G8B8:
      case D3DFMT_X8R8G8B8:
      case D3DFMT_A16B16G16R16F:
      case D3DFMT_A32B32G32R32F:
        return true;
      default:
        UNREACHABLE();
    }

    return false;
}

bool TextureStorage::isRenderTarget() const
{
    return (mD3DUsage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL)) != 0;
}

bool TextureStorage::isManaged() const
{
    return (mD3DPool == D3DPOOL_MANAGED);
}

D3DPOOL TextureStorage::getPool() const
{
    return mD3DPool;
}

DWORD TextureStorage::getUsage() const
{
    return mD3DUsage;
}

unsigned int TextureStorage::getTextureSerial() const
{
    return mTextureSerial;
}

unsigned int TextureStorage::issueTextureSerial()
{
    return mCurrentTextureSerial++;
}

int TextureStorage::getLodOffset() const
{
    return mLodOffset;
}

int TextureStorage::levelCount()
{
    return getBaseTexture() ? getBaseTexture()->GetLevelCount() - getLodOffset() : 0;
}

TextureStorage2D::TextureStorage2D(Renderer *renderer, rx::SwapChain9 *swapchain) : TextureStorage(renderer, D3DUSAGE_RENDERTARGET), mRenderTargetSerial(gl::RenderbufferStorage::issueSerial())
{
    IDirect3DTexture9 *surfaceTexture = swapchain->getOffscreenTexture();
    mTexture = surfaceTexture;

    initializeRenderTarget();
}

TextureStorage2D::TextureStorage2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage(renderer, GetTextureUsage(Renderer9::makeRenderer9(renderer)->ConvertTextureInternalFormat(internalformat), usage, forceRenderable)),
      mRenderTargetSerial(gl::RenderbufferStorage::issueSerial())
{
    mTexture = NULL;
    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0)
    {
        IDirect3DDevice9 *device = mRenderer->getDevice(); // D3D9_REPLACE
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &width, &height, &mLodOffset);
        HRESULT result = device->CreateTexture(width, height, levels ? levels + mLodOffset : 0, getUsage(),
                                               mRenderer->ConvertTextureInternalFormat(internalformat), getPool(), &mTexture, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            error(GL_OUT_OF_MEMORY);
        }
    }

    initializeRenderTarget();
}

TextureStorage2D::~TextureStorage2D()
{
    if (mTexture)
    {
        mTexture->Release();
    }

    delete mRenderTarget;
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *TextureStorage2D::getSurfaceLevel(int level, bool dirty)
{
    IDirect3DSurface9 *surface = NULL;

    if (mTexture)
    {
        HRESULT result = mTexture->GetSurfaceLevel(level + mLodOffset, &surface);
        ASSERT(SUCCEEDED(result));

        // With managed textures the driver needs to be informed of updates to the lower mipmap levels
        if (level != 0 && isManaged() && dirty)
        {
            mTexture->AddDirtyRect(NULL);
        }
    }

    return surface;
}

RenderTarget *TextureStorage2D::getRenderTarget() const
{
    return mRenderTarget;
}

void TextureStorage2D::generateMipmap(int level)
{
    IDirect3DSurface9 *upper = getSurfaceLevel(level - 1, false);
    IDirect3DSurface9 *lower = getSurfaceLevel(level, true);

    if (upper != NULL && lower != NULL)
    {
        mRenderer->boxFilter(upper, lower);
    }

    if (upper != NULL) upper->Release();
    if (lower != NULL) lower->Release();
}

IDirect3DBaseTexture9 *TextureStorage2D::getBaseTexture() const
{
    return mTexture;
}

unsigned int TextureStorage2D::getRenderTargetSerial(GLenum target) const
{
    return mRenderTargetSerial;
}

void TextureStorage2D::initializeRenderTarget()
{
    if (mTexture != NULL && isRenderTarget())
    {
        IDirect3DSurface9 *surface = getSurfaceLevel(0, false);

        mRenderTarget = new RenderTarget9(mRenderer, surface);
    }
    else
    {
        mRenderTarget = NULL;
    }
}

TextureStorageCubeMap::TextureStorageCubeMap(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : TextureStorage(renderer, GetTextureUsage(Renderer9::makeRenderer9(renderer)->ConvertTextureInternalFormat(internalformat), usage, forceRenderable)),
      mFirstRenderTargetSerial(gl::RenderbufferStorage::issueCubeSerials())
{
    mTexture = NULL;
    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (size > 0)
    {
        IDirect3DDevice9 *device = mRenderer->getDevice();
        int height = size;
        gl::MakeValidSize(false, gl::IsCompressed(internalformat), &size, &height, &mLodOffset);
        HRESULT result = device->CreateCubeTexture(size, levels ? levels + mLodOffset : 0, getUsage(),
                                                   mRenderer->ConvertTextureInternalFormat(internalformat), getPool(), &mTexture, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            error(GL_OUT_OF_MEMORY);
        }
    }

    initializeRenderTarget();
}

TextureStorageCubeMap::~TextureStorageCubeMap()
{
    if (mTexture)
    {
        mTexture->Release();
    }

    for (int i = 0; i < 6; ++i)
    {
        delete mRenderTarget[i];
    }
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *TextureStorageCubeMap::getCubeMapSurface(GLenum faceTarget, int level, bool dirty)
{
    IDirect3DSurface9 *surface = NULL;

    if (mTexture)
    {
        D3DCUBEMAP_FACES face = gl_d3d9::ConvertCubeFace(faceTarget);
        HRESULT result = mTexture->GetCubeMapSurface(face, level + mLodOffset, &surface);
        ASSERT(SUCCEEDED(result));

        // With managed textures the driver needs to be informed of updates to the lower mipmap levels
        if (level != 0 && isManaged() && dirty)
        {
            mTexture->AddDirtyRect(face, NULL);
        }
    }

    return surface;
}

RenderTarget *TextureStorageCubeMap::getRenderTarget(GLenum faceTarget) const
{
    return mRenderTarget[gl::TextureCubeMap::faceIndex(faceTarget)];
}

void TextureStorageCubeMap::generateMipmap(int face, int level)
{
    IDirect3DSurface9 *upper = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level - 1, false);
    IDirect3DSurface9 *lower = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, true);

    if (upper != NULL && lower != NULL)
    {
        mRenderer->boxFilter(upper, lower);
    }

    if (upper != NULL) upper->Release();
    if (lower != NULL) lower->Release();
}

IDirect3DBaseTexture9 *TextureStorageCubeMap::getBaseTexture() const
{
    return mTexture;
}

unsigned int TextureStorageCubeMap::getRenderTargetSerial(GLenum target) const
{
    return mFirstRenderTargetSerial + gl::TextureCubeMap::faceIndex(target);
}

void TextureStorageCubeMap::initializeRenderTarget()
{
    if (mTexture != NULL && isRenderTarget())
    {
        IDirect3DSurface9 *surface = NULL;

        for (int i = 0; i < 6; ++i)
        {
            surface = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, false);

            mRenderTarget[i] = new RenderTarget9(mRenderer, surface);
        }
    }
    else
    {
        for (int i = 0; i < 6; ++i)
        {
            mRenderTarget[i] = NULL;
        }
    }
}

}