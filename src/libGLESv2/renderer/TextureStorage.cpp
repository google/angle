//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.cpp: Implements the abstract gl::TextureStorage class and its concrete derived
// classes TextureStorage2D and TextureStorageCubeMap, which act as the interface to the
// D3D-side texture.

#include "libGLESv2/main.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "libGLESv2/renderer/SwapChain.h"
#include "libGLESv2/Blit.h"

#include "libGLESv2/renderer/renderer9_utils.h"

#include "common/debug.h"

namespace gl
{
unsigned int TextureStorage::mCurrentTextureSerial = 1;

TextureStorage::TextureStorage(rx::Renderer *renderer, DWORD usage)
    : mD3DUsage(usage),
      mD3DPool(getDisplay()->getRenderer9()->getTexturePool(usage)), // D3D9_REPLACE
      mRenderer(renderer),
      mTextureSerial(issueTextureSerial()),
      mLodOffset(0)
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

D3DFORMAT TextureStorage::ConvertTextureInternalFormat(GLint internalformat)
{
    switch (internalformat)
    {
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH_COMPONENT32_OES:
      case GL_DEPTH24_STENCIL8_OES:
        return D3DFMT_INTZ;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return D3DFMT_DXT1;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
        return D3DFMT_DXT3;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
        return D3DFMT_DXT5;
      case GL_RGBA32F_EXT:
      case GL_RGB32F_EXT:
      case GL_ALPHA32F_EXT:
      case GL_LUMINANCE32F_EXT:
      case GL_LUMINANCE_ALPHA32F_EXT:
        return D3DFMT_A32B32G32R32F;
      case GL_RGBA16F_EXT:
      case GL_RGB16F_EXT:
      case GL_ALPHA16F_EXT:
      case GL_LUMINANCE16F_EXT:
      case GL_LUMINANCE_ALPHA16F_EXT:
        return D3DFMT_A16B16G16R16F;
      case GL_LUMINANCE8_EXT:
        if (getContext()->supportsLuminanceTextures())
        {
            return D3DFMT_L8;
        }
        break;
      case GL_LUMINANCE8_ALPHA8_EXT:
        if (getContext()->supportsLuminanceAlphaTextures())
        {
            return D3DFMT_A8L8;
        }
        break;
      case GL_RGB8_OES:
      case GL_RGB565:
        return D3DFMT_X8R8G8B8;
    }

    return D3DFMT_A8R8G8B8;
}

Blit *TextureStorage::getBlitter()
{
    Context *context = getContext();
    return context->getBlitter();
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

bool TextureStorage::copyToRenderTarget(IDirect3DSurface9 *dest, IDirect3DSurface9 *source, bool fromManaged)
{
    if (source && dest)
    {
        HRESULT result = D3DERR_OUTOFVIDEOMEMORY;
        rx::Renderer9 *renderer = getDisplay()->getRenderer9();
        IDirect3DDevice9 *device = renderer->getDevice(); // D3D9_REPLACE

        if (fromManaged)
        {
            D3DSURFACE_DESC desc;
            source->GetDesc(&desc);

            IDirect3DSurface9 *surf = 0;
            result = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &surf, NULL);

            if (SUCCEEDED(result))
            {
                Image::CopyLockableSurfaces(surf, source);
                result = device->UpdateSurface(surf, NULL, dest, NULL);
                surf->Release();
            }
        }
        else
        {
            renderer->endScene();
            result = device->StretchRect(source, NULL, dest, NULL, D3DTEXF_NONE);
        }

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            return false;
        }
    }

    return true;
} 

TextureStorage2D::TextureStorage2D(rx::Renderer *renderer, rx::SwapChain *swapchain) : TextureStorage(renderer, D3DUSAGE_RENDERTARGET), mRenderTargetSerial(RenderbufferStorage::issueSerial())
{
    IDirect3DTexture9 *surfaceTexture = swapchain->getOffscreenTexture();
    mTexture = surfaceTexture;
}

TextureStorage2D::TextureStorage2D(rx::Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage(renderer, GetTextureUsage(ConvertTextureInternalFormat(internalformat), usage, forceRenderable)),
      mRenderTargetSerial(RenderbufferStorage::issueSerial())
{
    mTexture = NULL;
    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0)
    {
        IDirect3DDevice9 *device = getDisplay()->getRenderer9()->getDevice(); // D3D9_REPLACE
        MakeValidSize(false, gl::IsCompressed(internalformat), &width, &height, &mLodOffset);
        HRESULT result = device->CreateTexture(width, height, levels ? levels + mLodOffset : 0, getUsage(),
                                               ConvertTextureInternalFormat(internalformat), getPool(), &mTexture, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            error(GL_OUT_OF_MEMORY);
        }
    }
}

TextureStorage2D::~TextureStorage2D()
{
    if (mTexture)
    {
        mTexture->Release();
    }
}

bool TextureStorage2D::copyToRenderTarget(TextureStorage2D *dest, TextureStorage2D *source)
{
    bool result = false;

    if (source && dest)
    {
        int levels = source->levelCount();
        for (int i = 0; i < levels; ++i)
        {
            IDirect3DSurface9 *srcSurf = source->getSurfaceLevel(i, false);
            IDirect3DSurface9 *dstSurf = dest->getSurfaceLevel(i, false);
            
            result = TextureStorage::copyToRenderTarget(dstSurf, srcSurf, source->isManaged());

            if (srcSurf) srcSurf->Release();
            if (dstSurf) dstSurf->Release();

            if (!result)
                return false;
        }
    }

    return result;
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

void TextureStorage2D::generateMipmap(int level)
{
    IDirect3DSurface9 *upper = getSurfaceLevel(level - 1, false);
    IDirect3DSurface9 *lower = getSurfaceLevel(level, true);

    if (upper != NULL && lower != NULL)
    {
        getBlitter()->boxFilter(upper, lower);
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

TextureStorageCubeMap::TextureStorageCubeMap(rx::Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : TextureStorage(renderer, GetTextureUsage(ConvertTextureInternalFormat(internalformat), usage, forceRenderable)),
      mFirstRenderTargetSerial(RenderbufferStorage::issueCubeSerials())
{
    mTexture = NULL;
    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (size > 0)
    {
        IDirect3DDevice9 *device = getDisplay()->getRenderer9()->getDevice(); // D3D9_REPLACE
        int height = size;
        MakeValidSize(false, gl::IsCompressed(internalformat), &size, &height, &mLodOffset);
        HRESULT result = device->CreateCubeTexture(size, levels ? levels + mLodOffset : 0, getUsage(),
                                                   ConvertTextureInternalFormat(internalformat), getPool(), &mTexture, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            error(GL_OUT_OF_MEMORY);
        }
    }
}

TextureStorageCubeMap::~TextureStorageCubeMap()
{
    if (mTexture)
    {
        mTexture->Release();
    }
}

bool TextureStorageCubeMap::copyToRenderTarget(TextureStorageCubeMap *dest, TextureStorageCubeMap *source)
{
    bool result = false;

    if (source && dest)
    {
        int levels = source->levelCount();
        for (int f = 0; f < 6; f++)
        {
            for (int i = 0; i < levels; i++)
            {
                IDirect3DSurface9 *srcSurf = source->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i, false);
                IDirect3DSurface9 *dstSurf = dest->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i, true);

                result = TextureStorage::copyToRenderTarget(dstSurf, srcSurf, source->isManaged());

                if (srcSurf) srcSurf->Release();
                if (dstSurf) dstSurf->Release();

                if (!result)
                    return false;
            }
        }
    }

    return result;
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *TextureStorageCubeMap::getCubeMapSurface(GLenum faceTarget, int level, bool dirty)
{
    IDirect3DSurface9 *surface = NULL;

    if (mTexture)
    {
        D3DCUBEMAP_FACES face = es2dx::ConvertCubeFace(faceTarget);
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

void TextureStorageCubeMap::generateMipmap(int face, int level)
{
    IDirect3DSurface9 *upper = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level - 1, false);
    IDirect3DSurface9 *lower = getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, true);

    if (upper != NULL && lower != NULL)
    {
        getBlitter()->boxFilter(upper, lower);
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
    return mFirstRenderTargetSerial + TextureCubeMap::faceIndex(target);
}

}