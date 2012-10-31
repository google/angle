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

#include "common/debug.h"

namespace gl
{
unsigned int TextureStorage::mCurrentTextureSerial = 1;

TextureStorage::TextureStorage(DWORD usage)
    : mD3DUsage(usage),
      mD3DPool(getDisplay()->getRenderer()->getTexturePool(usage)), // D3D9_REPLACE
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

TextureStorage2D::TextureStorage2D(IDirect3DTexture9 *surfaceTexture) : TextureStorage(D3DUSAGE_RENDERTARGET), mRenderTargetSerial(RenderbufferStorage::issueSerial())
{
    mTexture = surfaceTexture;
}

TextureStorage2D::TextureStorage2D(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : TextureStorage(GetTextureUsage(ConvertTextureInternalFormat(internalformat), usage, forceRenderable)),
      mRenderTargetSerial(RenderbufferStorage::issueSerial())
{
    mTexture = NULL;
    // if the width or height is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (width > 0 && height > 0)
    {
        IDirect3DDevice9 *device = getDisplay()->getRenderer()->getDevice(); // D3D9_REPLACE
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

IDirect3DBaseTexture9 *TextureStorage2D::getBaseTexture() const
{
    return mTexture;
}

unsigned int TextureStorage2D::getRenderTargetSerial(GLenum target) const
{
    return mRenderTargetSerial;
}

TextureStorageCubeMap::TextureStorageCubeMap(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : TextureStorage(GetTextureUsage(ConvertTextureInternalFormat(internalformat), usage, forceRenderable)),
      mFirstRenderTargetSerial(RenderbufferStorage::issueCubeSerials())
{
    mTexture = NULL;
    // if the size is not positive this should be treated as an incomplete texture
    // we handle that here by skipping the d3d texture creation
    if (size > 0)
    {
        IDirect3DDevice9 *device = getDisplay()->getRenderer()->getDevice(); // D3D9_REPLACE
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

IDirect3DBaseTexture9 *TextureStorageCubeMap::getBaseTexture() const
{
    return mTexture;
}

unsigned int TextureStorageCubeMap::getRenderTargetSerial(GLenum target) const
{
    return mFirstRenderTargetSerial + TextureCubeMap::faceIndex(target);
}

}