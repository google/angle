//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage9.h: Defines the abstract rx::TextureStorage9 class and its concrete derived
// classes TextureStorage2D9 and TextureStorageCubeMap9, which act as the interface to the
// D3D9 texture.

#ifndef LIBGLESV2_RENDERER_TEXTURESTORAGE9_H_
#define LIBGLESV2_RENDERER_TEXTURESTORAGE9_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "libGLESv2/renderer/TextureStorage.h"
#include "common/debug.h"

namespace rx
{
class Renderer9;
class SwapChain9;
class RenderTarget;
class RenderTarget9;
class Blit;

class TextureStorage9 : public TextureStorageInterface
{
  public:
    TextureStorage9(Renderer *renderer, DWORD usage);
    virtual ~TextureStorage9();

    static TextureStorage9 *makeTextureStorage9(TextureStorageInterface *storage);

    static DWORD GetTextureUsage(D3DFORMAT d3dfmt, GLenum glusage, bool forceRenderable);
    static bool IsTextureFormatRenderable(D3DFORMAT format);

    D3DPOOL getPool() const;
    DWORD getUsage() const;

    virtual IDirect3DBaseTexture9 *getBaseTexture() const = 0;
    virtual RenderTarget *getRenderTarget() const { return NULL; }
    virtual RenderTarget *getRenderTarget(GLenum faceTarget) const { return NULL; }
    virtual void generateMipmap(int level) {};
    virtual void generateMipmap(int face, int level) {};

    virtual int getLodOffset() const;
    virtual bool isRenderTarget() const;
    virtual bool isManaged() const;
    virtual int levelCount();

  protected:
    int mLodOffset;
    Renderer9 *mRenderer;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage9);

    const DWORD mD3DUsage;
    const D3DPOOL mD3DPool;
};

class TextureStorage2D9 : public TextureStorage9
{
  public:
    TextureStorage2D9(Renderer *renderer, SwapChain9 *swapchain);
    TextureStorage2D9(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height);
    virtual ~TextureStorage2D9();

    static TextureStorage2D9 *makeTextureStorage2D9(TextureStorageInterface *storage);

    IDirect3DSurface9 *getSurfaceLevel(int level, bool dirty);
    virtual RenderTarget *getRenderTarget() const;
    virtual IDirect3DBaseTexture9 *getBaseTexture() const;
    virtual void generateMipmap(int level);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage2D9);

    void initializeRenderTarget();

    IDirect3DTexture9 *mTexture;
    RenderTarget9 *mRenderTarget;
};

class TextureStorageCubeMap9 : public TextureStorage9
{
  public:
    TextureStorageCubeMap9(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size);
    virtual ~TextureStorageCubeMap9();

    static TextureStorageCubeMap9 *makeTextureStorageCubeMap9(TextureStorageInterface *storage);

    IDirect3DSurface9 *getCubeMapSurface(GLenum faceTarget, int level, bool dirty);
    virtual RenderTarget *getRenderTarget(GLenum faceTarget) const;
    virtual IDirect3DBaseTexture9 *getBaseTexture() const;
    virtual void generateMipmap(int face, int level);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorageCubeMap9);

    void initializeRenderTarget();

    IDirect3DCubeTexture9 *mTexture;
    RenderTarget9 *mRenderTarget[6];
};

}

#endif // LIBGLESV2_RENDERER_TEXTURESTORAGE9_H_

