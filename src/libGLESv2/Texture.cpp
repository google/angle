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
    getImplementation()->setUsage(usage);
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

// Tests for texture sampling completeness
bool Texture::isSamplerComplete(const SamplerState &samplerState) const
{
    return getImplementation()->isSamplerComplete(samplerState);
}

rx::TextureStorageInterface *Texture::getNativeTexture()
{
    return getImplementation()->getNativeTexture();
}

void Texture::generateMipmaps()
{
    getImplementation()->generateMipmaps();
}

void Texture::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    getImplementation()->copySubImage(target, level, xoffset, yoffset, zoffset, x, y, width, height, source);
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

const rx::Image *Texture::getBaseLevelImage() const
{
    return (getImplementation()->getLayerCount(0) > 0 ? getImplementation()->getImage(0, 0) : NULL);
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

GLsizei Texture2D::getWidth(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getWidth();
    else
        return 0;
}

GLsizei Texture2D::getHeight(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getHeight();
    else
        return 0;
}

GLenum Texture2D::getInternalFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getInternalFormat();
    else
        return GL_NONE;
}

GLenum Texture2D::getActualFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, 0)->getActualFormat();
    else
        return GL_NONE;
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    releaseTexImage();

    mTexture->setImage(GL_TEXTURE_2D, level, width, height, 1, internalFormat, format, type, unpack, pixels);
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
    releaseTexImage();

    mTexture->setCompressedImage(GL_TEXTURE_2D, level, format, width, height, 1, imageSize, pixels);
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(GL_TEXTURE_2D, level, xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels);
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(GL_TEXTURE_2D, level, xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels);
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    releaseTexImage();

    mTexture->copyImage(GL_TEXTURE_2D, level, format, x, y, width, height, source);
}

void Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_2D, levels, internalformat, width, height, 1);
}

bool Texture2D::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture2D::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

void Texture2D::generateMipmaps()
{
    releaseTexImage();

    mTexture->generateMipmaps();
}

unsigned int Texture2D::getRenderTargetSerial(GLint level)
{
    return mTexture->getRenderTargetSerial(level, 0);
}

rx::RenderTarget *Texture2D::getRenderTarget(GLint level)
{
    return mTexture->getRenderTarget(level, 0);
}

rx::RenderTarget *Texture2D::getDepthStencil(GLint level)
{
    return mTexture->getDepthStencil(level, 0);
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

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mTexture->getImage(level, targetToLayerIndex(target))->getActualFormat();
    else
        return GL_NONE;
}

void TextureCubeMap::setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, level, width, height, 1, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(target, level, format, width, height, 1, imageSize, pixels);
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(target, level, xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels);
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(target, level, xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels);
}

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isCubeComplete() const
{
    return mTexture->isCubeComplete();
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(target, level)).compressed;
}

bool TextureCubeMap::isDepth(GLenum target, GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(target, level)).depthBits > 0;
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    mTexture->copyImage(target, level, format, x, y, width, height, source);
}

void TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_CUBE_MAP, levels, internalformat, size, size, 1);
}

unsigned int TextureCubeMap::getRenderTargetSerial(GLenum target, GLint level)
{
    return mTexture->getRenderTargetSerial(level, targetToLayerIndex(target));
}

int TextureCubeMap::targetToLayerIndex(GLenum target)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return target - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
}

GLenum TextureCubeMap::layerIndexToTarget(GLint layer)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer;
}

rx::RenderTarget *TextureCubeMap::getRenderTarget(GLenum target, GLint level)
{
    return mTexture->getRenderTarget(level, targetToLayerIndex(target));
}

rx::RenderTarget *TextureCubeMap::getDepthStencil(GLenum target, GLint level)
{
    return mTexture->getDepthStencil(level, targetToLayerIndex(target));
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

GLsizei Texture3D::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getWidth() : 0;
}

GLsizei Texture3D::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getHeight() : 0;
}

GLsizei Texture3D::getDepth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getDepth() : 0;
}

GLenum Texture3D::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getInternalFormat() : GL_NONE;
}

GLenum Texture3D::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mTexture->getImage(level, 0)->getActualFormat() : GL_NONE;
}

bool Texture3D::isCompressed(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture3D::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

void Texture3D::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_3D, level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

void Texture3D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(GL_TEXTURE_3D, level, format, width, height, depth, imageSize, pixels);
}

void Texture3D::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

void Texture3D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(GL_TEXTURE_3D, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels);
}

void Texture3D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_3D, levels, internalformat, width, height, depth);
}

unsigned int Texture3D::getRenderTargetSerial(GLint level, GLint layer)
{
    return mTexture->getRenderTargetSerial(level, layer);
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
    return GetInternalFormatInfo(getInternalFormat(level)).compressed;
}

bool Texture2DArray::isDepth(GLint level) const
{
    return GetInternalFormatInfo(getInternalFormat(level)).depthBits > 0;
}

void Texture2DArray::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->setImage(GL_TEXTURE_2D_ARRAY, level, width, height, depth, internalFormat, format, type, unpack, pixels);
}

void Texture2DArray::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    mTexture->setCompressedImage(GL_TEXTURE_2D_ARRAY, level, format, width, height, depth, imageSize, pixels);
}

void Texture2DArray::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    mTexture->subImage(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels);
}

void Texture2DArray::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    mTexture->subImageCompressed(GL_TEXTURE_2D_ARRAY, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels);
}

void Texture2DArray::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    mImmutable = true;

    mTexture->storage(GL_TEXTURE_2D_ARRAY, levels, internalformat, width, height, depth);
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
