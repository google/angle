//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.cpp: Implements the abstract rx::TextureStorageInterface class and its concrete derived
// classes TextureStorageInterface2D and TextureStorageInterfaceCube, which act as the interface to the
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
unsigned int TextureStorageInterface::mCurrentTextureSerial = 1;

TextureStorageInterface::TextureStorageInterface()
    : mTextureSerial(issueTextureSerial()),
      mInstance(NULL)
{
}

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

int TextureStorageInterface::getLodOffset() const
{
    return mInstance->getLodOffset();
}


int TextureStorageInterface::levelCount()
{
    return mInstance->levelCount();
}

TextureStorageInterface2D::TextureStorageInterface2D(Renderer *renderer, SwapChain9 *swapchain) 
    : mRenderTargetSerial(gl::RenderbufferStorage::issueSerial())
{
    TextureStorage9_2D *newInterface = new TextureStorage9_2D(renderer, swapchain);
    mInstance = newInterface;
}

TextureStorageInterface2D::TextureStorageInterface2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
    : mRenderTargetSerial(gl::RenderbufferStorage::issueSerial())
{
    TextureStorage9_2D *newInterface = new TextureStorage9_2D(renderer, levels, internalformat, usage, forceRenderable, width, height); // D3D9_REPLACE
    mInstance = newInterface;
}

TextureStorageInterface2D::~TextureStorageInterface2D()
{
}

RenderTarget *TextureStorageInterface2D::getRenderTarget() const
{
    return mInstance->getRenderTarget();
}

void TextureStorageInterface2D::generateMipmap(int level)
{
    mInstance->generateMipmap(level);
}

unsigned int TextureStorageInterface2D::getRenderTargetSerial(GLenum target) const
{
    return mRenderTargetSerial;
}

TextureStorageInterfaceCube::TextureStorageInterfaceCube(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
    : mFirstRenderTargetSerial(gl::RenderbufferStorage::issueCubeSerials())
{
    TextureStorage9_Cube *newInterface = new TextureStorage9_Cube(renderer, levels, internalformat, usage, forceRenderable, size); // D3D9_REPLACE
    mInstance = newInterface;
}

TextureStorageInterfaceCube::~TextureStorageInterfaceCube()
{
}

RenderTarget *TextureStorageInterfaceCube::getRenderTarget(GLenum faceTarget) const
{
    return mInstance->getRenderTarget(faceTarget);
}

void TextureStorageInterfaceCube::generateMipmap(int face, int level)
{
    mInstance->generateMipmap(face, level);
}

unsigned int TextureStorageInterfaceCube::getRenderTargetSerial(GLenum target) const
{
    return mFirstRenderTargetSerial + gl::TextureCubeMap::faceIndex(target);
}

}