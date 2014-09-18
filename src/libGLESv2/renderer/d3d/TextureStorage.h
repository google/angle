//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.h: Defines the abstract rx::TextureStorage class.

#ifndef LIBGLESV2_RENDERER_TEXTURESTORAGE_H_
#define LIBGLESV2_RENDERER_TEXTURESTORAGE_H_

#include "common/debug.h"

#include <GLES2/gl2.h>

namespace gl
{
struct ImageIndex;
}

namespace rx
{
class Renderer;
class SwapChain;
class RenderTarget;

class TextureStorage
{
  public:
    TextureStorage() {};
    virtual ~TextureStorage() {};

    virtual int getTopLevel() const = 0;
    virtual bool isRenderTarget() const = 0;
    virtual bool isManaged() const = 0;
    virtual int getLevelCount() const = 0;

    virtual RenderTarget *getRenderTarget(const gl::ImageIndex &index) = 0;
    virtual void generateMipmaps() = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorage);

};

class TextureStorageInterface
{
  public:
    TextureStorageInterface(TextureStorage *storageInstance, unsigned int rtSerialLayerStride);
    virtual ~TextureStorageInterface();

    TextureStorage *getStorageInstance() { return mInstance; }

    unsigned int getTextureSerial() const;

    virtual int getTopLevel() const;
    virtual bool isRenderTarget() const;
    virtual bool isManaged() const;
    virtual int getLevelCount() const;

    unsigned int getRenderTargetSerial(const gl::ImageIndex &index) const;

  protected:
    TextureStorage *mInstance;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureStorageInterface);

    const unsigned int mTextureSerial;
    static unsigned int issueTextureSerial();

    static unsigned int mCurrentTextureSerial;

    unsigned int mFirstRenderTargetSerial;
    unsigned int mRenderTargetSerialsLayerStride;
};

}

#endif // LIBGLESV2_RENDERER_TEXTURESTORAGE_H_
