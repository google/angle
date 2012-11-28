//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.h: Defines the abstract gl::TextureStorage class and its concrete derived
// classes TextureStorage2D and TextureStorageCubeMap, which act as the interface to the
// D3D-side texture.

#ifndef LIBGLESV2_TEXTURESTORAGE_H_
#define LIBGLESV2_TEXTURESTORAGE_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/debug.h"

namespace rx
{
class Renderer9;
class SwapChain;
class Blit;
}

namespace gl
{
class TextureStorage
{
  public:
    TextureStorage(rx::Renderer9 *renderer, DWORD usage);

    virtual ~TextureStorage();

    static DWORD GetTextureUsage(D3DFORMAT d3dfmt, GLenum glusage, bool forceRenderable);
    static bool IsTextureFormatRenderable(D3DFORMAT format);

    bool isRenderTarget() const;
    bool isManaged() const;
    D3DPOOL getPool() const;
    DWORD getUsage() const;
    unsigned int getTextureSerial() const;
    virtual IDirect3DBaseTexture9 *getBaseTexture() const = 0;
    virtual unsigned int getRenderTargetSerial(GLenum target) const = 0;
    int getLodOffset() const;
    int levelCount();

  protected:
    int mLodOffset;
    rx::Renderer9 *mRenderer;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage);

    const DWORD mD3DUsage;
    const D3DPOOL mD3DPool;

    const unsigned int mTextureSerial;
    static unsigned int issueTextureSerial();

    static unsigned int mCurrentTextureSerial;
};

class TextureStorage2D : public TextureStorage
{
  public:
    explicit TextureStorage2D(rx::Renderer9 *renderer, rx::SwapChain *swapchain);
    TextureStorage2D(rx::Renderer9 *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height);

    virtual ~TextureStorage2D();

    IDirect3DSurface9 *getSurfaceLevel(int level, bool dirty);
    virtual IDirect3DBaseTexture9 *getBaseTexture() const;
    void generateMipmap(int level);

    virtual unsigned int getRenderTargetSerial(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage2D);

    IDirect3DTexture9 *mTexture;
    const unsigned int mRenderTargetSerial;
};

class TextureStorageCubeMap : public TextureStorage
{
  public:
    TextureStorageCubeMap(rx::Renderer9 *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size);

    virtual ~TextureStorageCubeMap();

    IDirect3DSurface9 *getCubeMapSurface(GLenum faceTarget, int level, bool dirty);
    virtual IDirect3DBaseTexture9 *getBaseTexture() const;
    void generateMipmap(int face, int level);

    virtual unsigned int getRenderTargetSerial(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorageCubeMap);

    IDirect3DCubeTexture9 *mTexture;
    const unsigned int mFirstRenderTargetSerial;
};

}

#endif // LIBGLESV2_TEXTURESTORAGE_H_

