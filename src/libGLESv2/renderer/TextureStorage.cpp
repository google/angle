//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.cpp: Implements the abstract rx::TextureStorage class and its concrete derived
// classes TextureStorage2D and TextureStorageCubeMap, which act as the interface to the
// GPU-side texture.

#include "libGLESv2/main.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "libGLESv2/renderer/TextureStorage9.h"
#include "libGLESv2/renderer/SwapChain9.h"
#include "libGLESv2/renderer/Blit.h"
#include "libGLESv2/renderer/RenderTarget9.h"
#include "libGLESv2/renderer/renderer9_utils.h"

#include "common/debug.h"

namespace rx
{
unsigned int TextureStorage::mCurrentTextureSerial = 1;

TextureStorage::TextureStorage()
    : mTextureSerial(issueTextureSerial()),
      mInterface(NULL)
{
}

TextureStorage::~TextureStorage()
{
    delete mInterface;
}

bool TextureStorage::isRenderTarget() const
{
    return mInterface->isRenderTarget();
}


bool TextureStorage::isManaged() const
{
    return mInterface->isManaged();
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
    return mInterface->getLodOffset();
}


int TextureStorage::levelCount()
{
    return mInterface->levelCount();
}

TextureStorage2D::TextureStorage2D(Renderer *renderer, SwapChain9 *swapchain) 
    : mRenderTargetSerial(gl::RenderbufferStorage::issueSerial())
{
    TextureStorage2D9 *newInterface = new TextureStorage2D9(renderer, swapchain);
    mInterface = newInterface;
}

TextureStorage2D::TextureStorage2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : mRenderTargetSerial(gl::RenderbufferStorage::issueSerial())
{
    TextureStorage2D9 *newInterface = new TextureStorage2D9(renderer, levels, internalformat, usage, forceRenderable, width, height); // D3D9_REPLACE
    mInterface = newInterface;
}

TextureStorage2D::~TextureStorage2D()
{
}

RenderTarget *TextureStorage2D::getRenderTarget() const
{
    return mInterface->getRenderTarget();
}

void TextureStorage2D::generateMipmap(int level)
{
    mInterface->generateMipmap(level);
}

unsigned int TextureStorage2D::getRenderTargetSerial(GLenum target) const
{
    return mRenderTargetSerial;
}

TextureStorageCubeMap::TextureStorageCubeMap(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : mFirstRenderTargetSerial(gl::RenderbufferStorage::issueCubeSerials())
{
    TextureStorageCubeMap9 *newInterface = new TextureStorageCubeMap9(renderer, levels, internalformat, usage, forceRenderable, size); // D3D9_REPLACE
    mInterface = newInterface;
}

TextureStorageCubeMap::~TextureStorageCubeMap()
{
}

RenderTarget *TextureStorageCubeMap::getRenderTarget(GLenum faceTarget) const
{
    return mInterface->getRenderTarget(faceTarget);
}

void TextureStorageCubeMap::generateMipmap(int face, int level)
{
    mInterface->generateMipmap(face, level);
}

unsigned int TextureStorageCubeMap::getRenderTargetSerial(GLenum target) const
{
    return mFirstRenderTargetSerial + gl::TextureCubeMap::faceIndex(target);
}

}