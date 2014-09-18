//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.cpp: Shared members of abstract rx::TextureStorage class.

#include "libGLESv2/renderer/d3d/TextureStorage.h"
#include "libGLESv2/renderer/d3d/TextureD3D.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Texture.h"

#include "common/debug.h"
#include "common/mathutil.h"

namespace rx
{
unsigned int TextureStorageInterface::mCurrentTextureSerial = 1;

TextureStorageInterface::TextureStorageInterface(TextureStorage *textureStorage, unsigned int rtSerialLayerStride)
    : mTextureSerial(issueTextureSerial()),
      mInstance(textureStorage),
      mFirstRenderTargetSerial(gl::RenderbufferStorage::issueSerials(static_cast<unsigned int>(textureStorage->getLevelCount()) * rtSerialLayerStride)),
      mRenderTargetSerialsLayerStride(rtSerialLayerStride)
{}

TextureStorageInterface::~TextureStorageInterface()
{
    delete mInstance;
}

bool TextureStorageInterface::isRenderTarget() const
{
    return mInstance->isRenderTarget();
}

bool TextureStorageInterface::isManaged() const
{
    return mInstance->isManaged();
}

unsigned int TextureStorageInterface::getTextureSerial() const
{
    return mTextureSerial;
}

unsigned int TextureStorageInterface::issueTextureSerial()
{
    return mCurrentTextureSerial++;
}

int TextureStorageInterface::getTopLevel() const
{
    return mInstance->getTopLevel();
}

int TextureStorageInterface::getLevelCount() const
{
    return mInstance->getLevelCount();
}

unsigned int TextureStorageInterface::getRenderTargetSerial(const gl::ImageIndex &index) const
{
    unsigned int layerOffset = (index.hasLayer() ? (static_cast<unsigned int>(index.layerIndex) * mRenderTargetSerialsLayerStride) : 0);
    return mFirstRenderTargetSerial + static_cast<unsigned int>(index.mipIndex) + layerOffset;
}

}
