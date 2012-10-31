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

namespace gl
{

class TextureStorage
{
  public:
    explicit TextureStorage(DWORD usage);

    virtual ~TextureStorage();

    static DWORD GetTextureUsage(D3DFORMAT d3dfmt, GLenum glusage, bool forceRenderable);
    static bool IsTextureFormatRenderable(D3DFORMAT format);
    static D3DFORMAT ConvertTextureInternalFormat(GLint internalformat);

    bool isRenderTarget() const;
    bool isManaged() const;
    D3DPOOL getPool() const;
    DWORD getUsage() const;
    unsigned int getTextureSerial() const;
    virtual unsigned int getRenderTargetSerial(GLenum target) const = 0;
    int getLodOffset() const;

  protected:
    int mLodOffset;

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
    explicit TextureStorage2D(IDirect3DTexture9 *surfaceTexture);
    TextureStorage2D(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height);

    virtual ~TextureStorage2D();

    IDirect3DSurface9 *getSurfaceLevel(int level, bool dirty);
    IDirect3DBaseTexture9 *getBaseTexture() const;

    virtual unsigned int getRenderTargetSerial(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage2D);

    IDirect3DTexture9 *mTexture;
    const unsigned int mRenderTargetSerial;
};

class TextureStorageCubeMap : public TextureStorage
{
  public:
    TextureStorageCubeMap(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size);

    virtual ~TextureStorageCubeMap();

    IDirect3DSurface9 *getCubeMapSurface(GLenum faceTarget, int level, bool dirty);
    IDirect3DBaseTexture9 *getBaseTexture() const;

    virtual unsigned int getRenderTargetSerial(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorageCubeMap);

    IDirect3DCubeTexture9 *mTexture;
    const unsigned int mFirstRenderTargetSerial;
};

}

#endif // LIBGLESV2_TEXTURESTORAGE_H_

