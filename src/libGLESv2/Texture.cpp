#include "precompiled.h"
//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "libGLESv2/Texture.h"

#include "libGLESv2/main.h"
#include "common/mathutil.h"
#include "common/utilities.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/Image.h"
#include "libGLESv2/renderer/d3d/TextureStorage.h"
#include "libEGL/Surface.h"
#include "libGLESv2/renderer/RenderTarget.h"
#include "libGLESv2/renderer/TextureImpl.h"

namespace gl
{

Texture::Texture(GLuint id, GLenum target)
    : RefCountObject(id),
      mUsage(GL_NONE),
      mImmutable(false),
      mTarget(target)
{
}

Texture::~Texture()
{
}

GLenum Texture::getTarget() const
{
    return mTarget;
}

void Texture::setUsage(GLenum usage)
{
    mUsage = usage;
}

void Texture::getSamplerStateWithNativeOffset(SamplerState *sampler)
{
    *sampler = mSamplerState;

    // Offset the effective base level by the texture storage's top level
    rx::TextureStorageInterface *texture = getNativeTexture();
    int topLevel = texture ? texture->getTopLevel() : 0;
    sampler->baseLevel = topLevel + mSamplerState.baseLevel;
}

GLenum Texture::getUsage() const
{
    return mUsage;
}

GLint Texture::getBaseLevelWidth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getWidth() : 0);
}

GLint Texture::getBaseLevelHeight() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getHeight() : 0);
}

GLint Texture::getBaseLevelDepth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getDepth() : 0);
}

// Note: "base level image" is loosely defined to be any image from the base level,
// where in the base of 2D array textures and cube maps there are several. Don't use
// the base level image for anything except querying texture format and size.
GLenum Texture::getBaseLevelInternalFormat() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getInternalFormat() : GL_NONE);
}

unsigned int Texture::getTextureSerial()
{
    rx::TextureStorageInterface *texture = getNativeTexture();
    return texture ? texture->getTextureSerial() : 0;
}

bool Texture::isImmutable() const
{
    return mImmutable;
}

int Texture::immutableLevelCount()
{
    return (mImmutable ? getNativeTexture()->getStorageInstance()->getLevelCount() : 0);
}

int Texture::mipLevels() const
{
    return log2(std::max(std::max(getBaseLevelWidth(), getBaseLevelHeight()), getBaseLevelDepth())) + 1;
}

Texture2D::Texture2D(rx::Texture2DImpl *impl, GLuint id)
    : Texture(id, GL_TEXTURE_2D),
      mTexture(impl)
{
    mSurface = NULL;
}

Texture2D::~Texture2D()
{
    SafeDelete(mTexture);

    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;
    }
}

rx::TextureStorageInterface *Texture2D::getNativeTexture()
{
    return mTexture->getNativeTexture();
}

void Texture2D::setUsage(GLenum usage)
{
    mUsage = usage;
    mTexture->setUsage(usage);
}

bool Texture2D::hasDirtyImages() const
{
    return mTexture->hasDirtyImages();
}

void Texture2D::resetDirty()
{
    mTexture->resetDirty();
}

GLsizei Texture2D::getWidth(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level)->getWidth();
    else
        return 0;
}

GLsizei Texture2D::getHeight(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level)->getHeight();
    else
        return 0;
}

GLenum Texture2D::getInternalFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level)->getInternalFormat();
    else
        return GL_NONE;
}

GLenum Texture2D::getActualFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level)->getActualFormat();
    else
        return GL_NONE;
}

void Texture2D::redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height)
{
    releaseTexImage();

    mTexture->redefineImage(level, internalformat, width, height);
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLenum sizedInternalFormat = IsSizedInternalFormat(internalFormat) ? internalFormat
                                                                       : GetSizedInternalFormat(format, type);
    redefineImage(level, sizedInternalFormat, width, height);

    mTexture->setImage(level, width, height, internalFormat, format, type, unpack, pixels);
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();

    mTexture->bindTexImage(surface);

    mSurface = surface;
    mSurface->setBoundTexture(this);
}

void Texture2D::releaseTexImage()
{
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;

        mTexture->releaseTexImage();
    }
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height);

    mTexture->setCompressedImage(level, format, width, height, imageSize, pixels);
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(level, xoffset, yoffset, width, height, format, type, unpack, pixels);
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(level, xoffset, yoffset, width, height, format, imageSize, pixels);
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    GLenum sizedInternalFormat = IsSizedInternalFormat(format) ? format
                                                               : GetSizedInternalFormat(format, GL_UNSIGNED_BYTE);
    redefineImage(level, sizedInternalFormat, width, height);

    mTexture->copyImage(level, format, x, y, width, height, source);
}

void Texture2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    mTexture->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
}

void Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    mImmutable = true;

    mTexture->storage(levels, internalformat, width, height);
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete(const SamplerState &samplerState) const
{
    return mTexture->isSamplerComplete(samplerState);
}

bool Texture2D::isCompressed(GLint level) const
{
    return IsFormatCompressed(getInternalFormat(level));
}

bool Texture2D::isDepth(GLint level) const
{
    return GetDepthBits(getInternalFormat(level)) > 0;
}

void Texture2D::generateMipmaps()
{
    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    int levelCount = mipLevels();
    for (int level = 1; level < levelCount; level++)
    {
        redefineImage(level, getBaseLevelInternalFormat(),
                      std::max(getBaseLevelWidth() >> level, 1),
                      std::max(getBaseLevelHeight() >> level, 1));
    }

    mTexture->generateMipmaps();
}

const rx::Image *Texture2D::getBaseLevelImage() const
{
    return mTexture->getImage(0);
}

unsigned int Texture2D::getRenderTargetSerial(GLint level)
{
    return mTexture->getRenderTargetSerial(level);
}

rx::RenderTarget *Texture2D::getRenderTarget(GLint level)
{
    return mTexture->getRenderTarget(level);
}

rx::RenderTarget *Texture2D::getDepthSencil(GLint level)
{
    return mTexture->getDepthSencil(level);
}

TextureCubeMap::TextureCubeMap(rx::TextureCubeImpl *impl, GLuint id)
    : Texture(id, GL_TEXTURE_CUBE_MAP),
      mTexture(impl)
{
}

TextureCubeMap::~TextureCubeMap()
{
    SafeDelete(mTexture);
}

rx::TextureStorageInterface *TextureCubeMap::getNativeTexture()
{
    return mTexture->getNativeTexture();
}

void TextureCubeMap::setUsage(GLenum usage)
{
    mUsage = usage;
    mTexture->setUsage(usage);
}

bool TextureCubeMap::hasDirtyImages() const
{
    return mTexture->hasDirtyImages();
}

void TextureCubeMap::resetDirty()
{
    mTexture->resetDirty();
}

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(target, level)->getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(target, level)->getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(target, level)->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(target, level)->getActualFormat();
    else
        return GL_NONE;
}

void TextureCubeMap::setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(0, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(1, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(2, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(3, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(4, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(5, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(target, level, format, width, height, imageSize, pixels);
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(target, level, xoffset, yoffset, width, height, format, type, unpack, pixels);
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(target, level, xoffset, yoffset, width, height, format, imageSize, pixels);
}

// Tests for cube map sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 86.
bool TextureCubeMap::isSamplerComplete(const SamplerState &samplerState) const
{
    return mTexture->isSamplerComplete(samplerState);
}

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isCubeComplete() const
{
    return mTexture->isCubeComplete();
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
    return IsFormatCompressed(getInternalFormat(target, level));
}

bool TextureCubeMap::isDepth(GLenum target, GLint level) const
{
    return GetDepthBits(getInternalFormat(target, level)) > 0;
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    mTexture->copyImage(target, level, format, x, y, width, height, source);
}

void TextureCubeMap::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    mTexture->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
}

void TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    mImmutable = true;

    mTexture->storage(levels, internalformat, size);
}

void TextureCubeMap::generateMipmaps()
{
    mTexture->generateMipmaps();
}

const rx::Image *TextureCubeMap::getBaseLevelImage() const
{
    // Note: if we are not cube-complete, there is no single base level image that can describe all
    // cube faces, so this method is only well-defined for a cube-complete base level.
    return mTexture->getImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
}

unsigned int TextureCubeMap::getRenderTargetSerial(GLenum target, GLint level)
{
    return mTexture->getRenderTargetSerial(target, level);
}

rx::RenderTarget *TextureCubeMap::getRenderTarget(GLenum target, GLint level)
{
    return mTexture->getRenderTarget(target, level);
}

rx::RenderTarget *TextureCubeMap::getDepthStencil(GLenum target, GLint level)
{
    return mTexture->getDepthStencil(target, level);
}

Texture3D::Texture3D(rx::Texture3DImpl *impl, GLuint id)
    : Texture(id, GL_TEXTURE_3D),
      mTexture(impl)
{
}

Texture3D::~Texture3D()
{
    SafeDelete(mTexture);
}

rx::TextureStorageInterface *Texture3D::getNativeTexture()
{
    return mTexture->getNativeTexture();
}

void Texture3D::setUsage(GLenum usage)
{
    mUsage = usage;
    mTexture->setUsage(usage);
}

bool Texture3D::hasDirtyImages() const
{
    return mTexture->hasDirtyImages();
}

void Texture3D::resetDirty()
{
    mTexture->resetDirty();
}

GLsizei Texture3D::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level)->getWidth() : 0;
}

GLsizei Texture3D::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level)->getHeight() : 0;
}

GLsizei Texture3D::getDepth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level)->getDepth() : 0;
}

GLenum Texture3D::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level)->getInternalFormat() : GL_NONE;
}

GLenum Texture3D::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level)->getActualFormat() : GL_NONE;
}

bool Texture3D::isCompressed(GLint level) const
{
    return IsFormatCompressed(getInternalFormat(level));
}

bool Texture3D::isDepth(GLint level) const
{
    return GetDepthBits(getInternalFormat(level)) > 0;
}

void Texture3D::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

void Texture3D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(level, format, width, height, depth, imageSize, pixels);
}

void Texture3D::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

void Texture3D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels);
}

void Texture3D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    mImmutable = true;

    mTexture->storage(levels, internalformat, width, height, depth);
}

void Texture3D::generateMipmaps()
{
    mTexture->generateMipmaps();
}

const rx::Image *Texture3D::getBaseLevelImage() const
{
    return mTexture->getImage(0);
}

void Texture3D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    mTexture->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
}

bool Texture3D::isSamplerComplete(const SamplerState &samplerState) const
{
    return mTexture->isSamplerComplete(samplerState);
}

bool Texture3D::isMipmapComplete() const
{
    return mTexture->isMipmapComplete();
}

unsigned int Texture3D::getRenderTargetSerial(GLint level, GLint layer)
{
    return mTexture->getRenderTargetSerial(level, layer);
}

rx::RenderTarget *Texture3D::getRenderTarget(GLint level)
{
    return mTexture->getRenderTarget(level);
}

rx::RenderTarget *Texture3D::getRenderTarget(GLint level, GLint layer)
{
    return mTexture->getRenderTarget(level, layer);
}

rx::RenderTarget *Texture3D::getDepthStencil(GLint level, GLint layer)
{
    return mTexture->getDepthStencil(level, layer);
}

Texture2DArray::Texture2DArray(rx::Texture2DArrayImpl *impl, GLuint id)
    : Texture(id, GL_TEXTURE_2D_ARRAY),
      mTexture(impl)
{
}

Texture2DArray::~Texture2DArray()
{
    SafeDelete(mTexture);
}

rx::TextureStorageInterface *Texture2DArray::getNativeTexture()
{
    return mTexture->getNativeTexture();
}

void Texture2DArray::setUsage(GLenum usage)
{
    mUsage = usage;
    mTexture->setUsage(usage);
}

bool Texture2DArray::hasDirtyImages() const
{
    return mTexture->hasDirtyImages();
}

void Texture2DArray::resetDirty()
{
    mTexture->resetDirty();
}

GLsizei Texture2DArray::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getWidth() : 0;
}

GLsizei Texture2DArray::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getHeight() : 0;
}

GLsizei Texture2DArray::getLayers(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getLayerCount(level) : 0;
}

GLenum Texture2DArray::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getInternalFormat() : GL_NONE;
}

GLenum Texture2DArray::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mTexture->getLayerCount(level) > 0) ? mTexture->getImage(level, 0)->getActualFormat() : GL_NONE;
}

bool Texture2DArray::isCompressed(GLint level) const
{
    return IsFormatCompressed(getInternalFormat(level));
}

bool Texture2DArray::isDepth(GLint level) const
{
    return GetDepthBits(getInternalFormat(level)) > 0;
}

void Texture2DArray::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

void Texture2DArray::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(level, format, width, height, depth, imageSize, pixels);
}

void Texture2DArray::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

void Texture2DArray::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels);
}

void Texture2DArray::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    mImmutable = true;

    mTexture->storage(levels, internalformat, width, height, depth);
}

void Texture2DArray::generateMipmaps()
{
    mTexture->generateMipmaps();
}

const rx::Image *Texture2DArray::getBaseLevelImage() const
{
    return (mTexture->getLayerCount(0) > 0 ? mTexture->getImage(0, 0) : NULL);
}

void Texture2DArray::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    mTexture->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
}

bool Texture2DArray::isSamplerComplete(const SamplerState &samplerState) const
{
    return mTexture->isSamplerComplete(samplerState);
}

bool Texture2DArray::isMipmapComplete() const
{
    return mTexture->isMipmapComplete();
}

unsigned int Texture2DArray::getRenderTargetSerial(GLint level, GLint layer)
{
    return mTexture->getRenderTargetSerial(level, layer);
}

rx::RenderTarget *Texture2DArray::getRenderTarget(GLint level, GLint layer)
{
    return mTexture->getRenderTarget(level, layer);
}

rx::RenderTarget *Texture2DArray::getDepthStencil(GLint level, GLint layer)
{
    return mTexture->getDepthStencil(level, layer);
}

}
