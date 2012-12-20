//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.h: Defines the abstract rx::TextureStorage class and its concrete derived
// classes TextureStorage2D and TextureStorageCubeMap, which act as the interface to the
// GPU-side texture.

#ifndef LIBGLESV2_RENDERER_TEXTURESTORAGE_H_
#define LIBGLESV2_RENDERER_TEXTURESTORAGE_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/debug.h"

namespace rx
{
class Renderer9;
class SwapChain9;
class RenderTarget;
class RenderTarget9;
class Blit;

class TextureStorageInterface
{
  public:
    TextureStorageInterface() {};
    virtual ~TextureStorageInterface() {};

    virtual int getLodOffset() const = 0;
    virtual bool isRenderTarget() const = 0;
    virtual bool isManaged() const = 0;
    virtual int levelCount() = 0;

    virtual RenderTarget *getRenderTarget() const = 0;
    virtual RenderTarget *getRenderTarget(GLenum faceTarget) const = 0;
    virtual void generateMipmap(int level) = 0;
    virtual void generateMipmap(int face, int level) = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorageInterface);

};

class TextureStorage
{
  public:
    TextureStorage();
    virtual ~TextureStorage();

    TextureStorageInterface *getStorageInterface() { return mInterface; }

    unsigned int getTextureSerial() const;
    virtual unsigned int getRenderTargetSerial(GLenum target) const = 0;

    virtual int getLodOffset() const;
    virtual bool isRenderTarget() const;
    virtual bool isManaged() const;
    virtual int levelCount();

  protected:
    TextureStorageInterface *mInterface;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage);

    const unsigned int mTextureSerial;
    static unsigned int issueTextureSerial();

    static unsigned int mCurrentTextureSerial;
};

class TextureStorage2D : public TextureStorage
{
  public:
    TextureStorage2D(Renderer *renderer, SwapChain9 *swapchain);
    TextureStorage2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height);
    virtual ~TextureStorage2D();

    void generateMipmap(int level);
    RenderTarget *getRenderTarget() const;

    virtual unsigned int getRenderTargetSerial(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage2D);

    const unsigned int mRenderTargetSerial;
};

class TextureStorageCubeMap : public TextureStorage
{
  public:
    TextureStorageCubeMap(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size);
    virtual ~TextureStorageCubeMap();

    void generateMipmap(int face, int level);
    RenderTarget *getRenderTarget(GLenum faceTarget) const;

    virtual unsigned int getRenderTargetSerial(GLenum target) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorageCubeMap);

    const unsigned int mFirstRenderTargetSerial;
};

}

#endif // LIBGLESV2_RENDERER_TEXTURESTORAGE_H_

