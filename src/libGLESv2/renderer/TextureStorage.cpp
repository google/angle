#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureStorage.cpp: Implements the abstract rx::TextureStorageInterface class and its concrete derived
// classes TextureStorageInterface2D and TextureStorageInterfaceCube, which act as the interface to the
// GPU-side texture.

#include "libGLESv2/renderer/TextureStorage.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/Texture.h"

#include "common/debug.h"
#include "common/mathutil.h"

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

static unsigned int GetActualLevelCount(GLsizei width, GLsizei height, GLsizei depth, GLint levelCount)
{
    return (levelCount <= 0) ? std::max(std::max(gl::log2(width), gl::log2(height)), gl::log2(depth)) : levelCount;
}

TextureStorageInterface2D::TextureStorageInterface2D(Renderer *renderer, SwapChain *swapchain) 
{
    mFirstRenderTargetSerial = gl::RenderbufferStorage::issueSerials(1);

    mInstance = renderer->createTextureStorage2D(swapchain);
}

TextureStorageInterface2D::TextureStorageInterface2D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
{
    unsigned int actualLevels = GetActualLevelCount(width, height, 0, levels);
    mFirstRenderTargetSerial = gl::RenderbufferStorage::issueSerials(actualLevels);

    mInstance = renderer->createTextureStorage2D(levels, internalformat, usage, forceRenderable, width, height);
}

TextureStorageInterface2D::~TextureStorageInterface2D()
{
}

RenderTarget *TextureStorageInterface2D::getRenderTarget(GLint level) const
{
    return mInstance->getRenderTarget(level);
}

void TextureStorageInterface2D::generateMipmap(int level)
{
    mInstance->generateMipmap(level);
}

unsigned int TextureStorageInterface2D::getRenderTargetSerial(GLint level) const
{
    return mFirstRenderTargetSerial + level;
}

TextureStorageInterfaceCube::TextureStorageInterfaceCube(Renderer *renderer, int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
{
    unsigned int actualLevels = GetActualLevelCount(size, size, 0, levels);
    mFirstRenderTargetSerial = gl::RenderbufferStorage::issueSerials(actualLevels * 6);

    mInstance = renderer->createTextureStorageCube(levels, internalformat, usage, forceRenderable, size);
}

TextureStorageInterfaceCube::~TextureStorageInterfaceCube()
{
}

RenderTarget *TextureStorageInterfaceCube::getRenderTarget(GLenum faceTarget, GLint level) const
{
    return mInstance->getRenderTargetFace(faceTarget, level);
}

void TextureStorageInterfaceCube::generateMipmap(int face, int level)
{
    mInstance->generateMipmap(face, level);
}

unsigned int TextureStorageInterfaceCube::getRenderTargetSerial(GLenum target, GLint level) const
{
    return mFirstRenderTargetSerial + (level * 6) + gl::TextureCubeMap::faceIndex(target);
}

TextureStorageInterface3D::TextureStorageInterface3D(Renderer *renderer, int levels, GLenum internalformat, GLenum usage,
                                                     GLsizei width, GLsizei height, GLsizei depth)
{
    mLevels = GetActualLevelCount(width, height, depth, levels);
    mFirstRenderTargetSerial = gl::RenderbufferStorage::issueSerials(mLevels * depth);

    mInstance = renderer->createTextureStorage3D(levels, internalformat, usage, width, height, depth);
}

TextureStorageInterface3D::~TextureStorageInterface3D()
{
}

void TextureStorageInterface3D::generateMipmap(int level)
{
    mInstance->generateMipmap(level);
}

RenderTarget *TextureStorageInterface3D::getRenderTarget(GLint level, GLint layer) const
{
    return mInstance->getRenderTargetLayer(level, layer);
}

unsigned int TextureStorageInterface3D::getRenderTargetSerial(GLint level, GLint layer) const
{
    return mFirstRenderTargetSerial + (layer * mLevels) + level;
}

TextureStorageInterface2DArray::TextureStorageInterface2DArray(Renderer *renderer, int levels, GLenum internalformat, GLenum usage,
                                                               GLsizei width, GLsizei height, GLsizei depth)
{
    mLevels = GetActualLevelCount(width, height, 0, levels);
    mFirstRenderTargetSerial = gl::RenderbufferStorage::issueSerials(mLevels * depth);

    mInstance = renderer->createTextureStorage2DArray(levels, internalformat, usage, width, height, depth);
}

TextureStorageInterface2DArray::~TextureStorageInterface2DArray()
{
}

void TextureStorageInterface2DArray::generateMipmap(int level)
{
    mInstance->generateMipmap(level);
}

RenderTarget *TextureStorageInterface2DArray::getRenderTarget(GLint level, GLint layer) const
{
    return mInstance->getRenderTargetLayer(level, layer);
}

unsigned int TextureStorageInterface2DArray::getRenderTargetSerial(GLint level, GLint layer) const
{
    return mFirstRenderTargetSerial + (layer * mLevels) + level;
}

}
