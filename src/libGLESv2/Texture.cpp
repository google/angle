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
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/d3d/ImageD3D.h"
#include "libGLESv2/renderer/d3d/TextureStorage.h"
#include "libEGL/Surface.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/renderer/BufferImpl.h"
#include "libGLESv2/renderer/RenderTarget.h"
#include "libGLESv2/renderer/TextureImpl.h"

namespace gl
{

bool IsMipmapFiltered(const SamplerState &samplerState)
{
    switch (samplerState.minFilter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
        return false;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        return true;
      default: UNREACHABLE();
        return false;
    }
}

bool IsRenderTargetUsage(GLenum usage)
{
    return (usage == GL_FRAMEBUFFER_ATTACHMENT_ANGLE);
}

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

TextureWithRenderer::TextureWithRenderer(rx::Renderer *renderer, GLuint id, GLenum target)
    : Texture(id, target),
      mRenderer(renderer),
      mDirtyImages(true)
{
}

TextureWithRenderer::~TextureWithRenderer()
{
}

// TODO: This is only used by the D3D backends and FramebufferAttachment. Once
// FramebufferAttachment has been refactored this function should be pushed
// down to TextureD3D.
rx::TextureStorageInterface *TextureWithRenderer::getNativeTexture()
{
    // ensure the underlying texture is created
    initializeStorage(false);

    rx::TextureStorageInterface *storage = getBaseLevelStorage();
    if (storage)
    {
        updateStorage();
    }

    return storage;
}

bool TextureWithRenderer::hasDirtyImages() const
{
    return mDirtyImages;
}

void TextureWithRenderer::resetDirty()
{
    mDirtyImages = false;
}

void TextureWithRenderer::setImage(const PixelUnpackState &unpack, GLenum type, const void *pixels, rx::Image *image)
{
    // No-op
    if (image->getWidth() == 0 || image->getHeight() == 0 || image->getDepth() == 0)
    {
        return;
    }

    // We no longer need the "GLenum format" parameter to TexImage to determine what data format "pixels" contains.
    // From our image internal format we know how many channels to expect, and "type" gives the format of pixel's components.
    const void *pixelData = pixels;

    if (unpack.pixelBuffer.id() != 0)
    {
        // Do a CPU readback here, if we have an unpack buffer bound and the fast GPU path is not supported
        Buffer *pixelBuffer = unpack.pixelBuffer.get();
        ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(pixels);
        // TODO: setImage/subImage is the only place outside of renderer that asks for a buffers raw data.
        // This functionality should be moved into renderer and the getData method of BufferImpl removed.
        const void *bufferData = pixelBuffer->getImplementation()->getData();
        pixelData = static_cast<const unsigned char *>(bufferData) + offset;
    }

    if (pixelData != NULL)
    {
        image->loadData(0, 0, 0, image->getWidth(), image->getHeight(), image->getDepth(), unpack.alignment, type, pixelData);
        mDirtyImages = true;
    }
}

bool TextureWithRenderer::isFastUnpackable(const PixelUnpackState &unpack, GLenum sizedInternalFormat)
{
    return unpack.pixelBuffer.id() != 0 && mRenderer->supportsFastCopyBufferToTexture(sizedInternalFormat);
}

bool TextureWithRenderer::fastUnpackPixels(const PixelUnpackState &unpack, const void *pixels, const Box &destArea,
                               GLenum sizedInternalFormat, GLenum type, rx::RenderTarget *destRenderTarget)
{
    if (destArea.width <= 0 && destArea.height <= 0 && destArea.depth <= 0)
    {
        return true;
    }

    // In order to perform the fast copy through the shader, we must have the right format, and be able
    // to create a render target.
    ASSERT(mRenderer->supportsFastCopyBufferToTexture(sizedInternalFormat));

    unsigned int offset = reinterpret_cast<unsigned int>(pixels);

    return mRenderer->fastCopyBufferToTexture(unpack, offset, destRenderTarget, sizedInternalFormat, type, destArea);
}

void TextureWithRenderer::setCompressedImage(GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(0, 0, 0, image->getWidth(), image->getHeight(), image->getDepth(), pixels);
        mDirtyImages = true;
    }
}

bool TextureWithRenderer::subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels, rx::Image *image)
{
    const void *pixelData = pixels;

    // CPU readback & copy where direct GPU copy is not supported
    if (unpack.pixelBuffer.id() != 0)
    {
        Buffer *pixelBuffer = unpack.pixelBuffer.get();
        unsigned int offset = reinterpret_cast<unsigned int>(pixels);
        // TODO: setImage/subImage is the only place outside of renderer that asks for a buffers raw data.
        // This functionality should be moved into renderer and the getData method of BufferImpl removed.
        const void *bufferData = pixelBuffer->getImplementation()->getData();
        pixelData = static_cast<const unsigned char *>(bufferData) + offset;
    }

    if (pixelData != NULL)
    {
        image->loadData(xoffset, yoffset, zoffset, width, height, depth, unpack.alignment, type, pixelData);
        mDirtyImages = true;
    }

    return true;
}

bool TextureWithRenderer::subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                 GLenum format, GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(xoffset, yoffset, zoffset, width, height, depth, pixels);
        mDirtyImages = true;
    }

    return true;
}

GLint TextureWithRenderer::creationLevels(GLsizei width, GLsizei height, GLsizei depth) const
{
    // TODO(geofflang): use context's extensions
    if ((isPow2(width) && isPow2(height) && isPow2(depth)) || mRenderer->getRendererExtensions().textureNPOT)
    {
        // Maximum number of levels
        return log2(std::max(std::max(width, height), depth)) + 1;
    }
    else
    {
        // OpenGL ES 2.0 without GL_OES_texture_npot does not permit NPOT mipmaps.
        return 1;
    }
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

Texture2DArray::Texture2DArray(rx::Renderer *renderer, GLuint id)
    : TextureWithRenderer(renderer, id, GL_TEXTURE_2D_ARRAY)
{
    mTexStorage = NULL;

    for (int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++level)
    {
        mLayerCounts[level] = 0;
        mImageArray[level] = NULL;
    }
}

Texture2DArray::~Texture2DArray()
{
    delete mTexStorage;
    mTexStorage = NULL;

    deleteImages();
}

void Texture2DArray::deleteImages()
{
    for (int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++level)
    {
        for (int layer = 0; layer < mLayerCounts[level]; ++layer)
        {
            delete mImageArray[level][layer];
        }
        delete[] mImageArray[level];
        mImageArray[level] = NULL;
        mLayerCounts[level] = 0;
    }
}

GLsizei Texture2DArray::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mImageArray[level][0]->getWidth() : 0;
}

GLsizei Texture2DArray::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mImageArray[level][0]->getHeight() : 0;
}

GLsizei Texture2DArray::getLayers(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mLayerCounts[level] : 0;
}

GLenum Texture2DArray::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mImageArray[level][0]->getInternalFormat() : GL_NONE;
}

GLenum Texture2DArray::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mImageArray[level][0]->getActualFormat() : GL_NONE;
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
    GLenum sizedInternalFormat = IsSizedInternalFormat(internalFormat) ? internalFormat
                                                                       : GetSizedInternalFormat(format, type);
    redefineImage(level, sizedInternalFormat, width, height, depth);

    GLsizei inputDepthPitch = gl::GetDepthPitch(sizedInternalFormat, type, width, height, unpack.alignment);

    for (int i = 0; i < depth; i++)
    {
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;
        TextureWithRenderer::setImage(unpack, type, layerPixels, mImageArray[level][i]);
    }
}

void Texture2DArray::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height, depth);

    GLsizei inputDepthPitch = gl::GetDepthPitch(format, GL_UNSIGNED_BYTE, width, height, 1);

    for (int i = 0; i < depth; i++)
    {
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;
        TextureWithRenderer::setCompressedImage(imageSize, layerPixels, mImageArray[level][i]);
    }
}

void Texture2DArray::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLenum internalformat = getInternalFormat(level);
    GLsizei inputDepthPitch = gl::GetDepthPitch(internalformat, type, width, height, unpack.alignment);

    for (int i = 0; i < depth; i++)
    {
        int layer = zoffset + i;
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;

        if (TextureWithRenderer::subImage(xoffset, yoffset, zoffset, width, height, 1, format, type, unpack, layerPixels, mImageArray[level][layer]))
        {
            commitRect(level, xoffset, yoffset, layer, width, height);
        }
    }
}

void Texture2DArray::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    GLsizei inputDepthPitch = gl::GetDepthPitch(format, GL_UNSIGNED_BYTE, width, height, 1);

    for (int i = 0; i < depth; i++)
    {
        int layer = zoffset + i;
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;

        if (TextureWithRenderer::subImageCompressed(xoffset, yoffset, zoffset, width, height, 1, format, imageSize, layerPixels, mImageArray[level][layer]))
        {
            commitRect(level, xoffset, yoffset, layer, width, height);
        }
    }
}

void Texture2DArray::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    deleteImages();

    for (int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        GLsizei levelWidth = std::max(1, width >> level);
        GLsizei levelHeight = std::max(1, height >> level);

        mLayerCounts[level] = (level < levels ? depth : 0);

        if (mLayerCounts[level] > 0)
        {
            // Create new images for this level
            mImageArray[level] = new rx::Image*[mLayerCounts[level]];

            for (int layer = 0; layer < mLayerCounts[level]; layer++)
            {
                mImageArray[level][layer] = mRenderer->createImage();
                mImageArray[level][layer]->redefine(mRenderer, GL_TEXTURE_2D_ARRAY, internalformat, levelWidth,
                                                    levelHeight, 1, true);
            }
        }
    }

    mImmutable = true;
    setCompleteTexStorage(new rx::TextureStorageInterface2DArray(mRenderer, internalformat, IsRenderTargetUsage(mUsage), width, height, depth, levels));
}

void Texture2DArray::generateMipmaps()
{
    int baseWidth = getBaseLevelWidth();
    int baseHeight = getBaseLevelHeight();
    int baseDepth = getBaseLevelDepth();
    GLenum baseFormat = getBaseLevelInternalFormat();

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    int levelCount = mipLevels();
    for (int level = 1; level < levelCount; level++)
    {
        redefineImage(level, baseFormat, std::max(baseWidth >> level, 1), std::max(baseHeight >> level, 1), baseDepth);
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (int level = 1; level < levelCount; level++)
        {
            mTexStorage->generateMipmap(level);

            for (int layer = 0; layer < mLayerCounts[level]; layer++)
            {
                mImageArray[level][layer]->markClean();
            }
        }
    }
    else
    {
        for (int level = 1; level < levelCount; level++)
        {
            for (int layer = 0; layer < mLayerCounts[level]; layer++)
            {
                mRenderer->generateMipmap(mImageArray[level][layer], mImageArray[level - 1][layer]);
            }
        }
    }
}

const rx::Image *Texture2DArray::getBaseLevelImage() const
{
    return (mLayerCounts[0] > 0 ? mImageArray[0][0] : NULL);
}

rx::TextureStorageInterface *Texture2DArray::getBaseLevelStorage()
{
    return mTexStorage;
}

void Texture2DArray::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    // can only make our texture storage to a render target if level 0 is defined (with a width & height) and
    // the current level we're copying to is defined (with appropriate format, width & height)
    bool canCreateRenderTarget = isLevelComplete(level) && isLevelComplete(0);

    if (!mImageArray[level][0]->isRenderableFormat() || (!mTexStorage && !canCreateRenderTarget))
    {
        mImageArray[level][zoffset]->copy(xoffset, yoffset, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        ensureRenderTarget();

        if (isValidLevel(level))
        {
            updateStorageLevel(level);

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, gl::GetFormat(getInternalFormat(0)),
                                 xoffset, yoffset, zoffset, mTexStorage, level);
        }
    }
}

bool Texture2DArray::isSamplerComplete(const SamplerState &samplerState) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getLayers(0);

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    // TODO(geofflang): use context's texture caps
    if (!mRenderer->getRendererTextureCaps().get(getBaseLevelInternalFormat()).filterable)
    {
        if (samplerState.magFilter != GL_NEAREST ||
            (samplerState.minFilter != GL_NEAREST && samplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    if (IsMipmapFiltered(samplerState) && !isMipmapComplete())
    {
        return false;
    }

    return true;
}

bool Texture2DArray::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 1; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2DArray::isLevelComplete(int level) const
{
    ASSERT(level >= 0 && level < (int)ArraySize(mImageArray));

    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei layers = getLayers(0);

    if (width <= 0 || height <= 0 || layers <= 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    if (getInternalFormat(level) != getInternalFormat(0))
    {
        return false;
    }

    if (getWidth(level) != std::max(1, width >> level))
    {
        return false;
    }

    if (getHeight(level) != std::max(1, height >> level))
    {
        return false;
    }

    if (getLayers(level) != layers)
    {
        return false;
    }

    return true;
}

unsigned int Texture2DArray::getRenderTargetSerial(GLint level, GLint layer)
{
    return (ensureRenderTarget() ? mTexStorage->getRenderTargetSerial(level, layer) : 0);
}

bool Texture2DArray::isValidLevel(int level) const
{
    return (mTexStorage ? (level >= 0 && level < mTexStorage->getLevelCount()) : 0);
}

void Texture2DArray::initializeStorage(bool renderTarget)
{
    // Only initialize the first time this texture is used as a render target or shader resource
    if (mTexStorage)
    {
        return;
    }

    // do not attempt to create storage for nonexistant data
    if (!isLevelComplete(0))
    {
        return;
    }

    bool createRenderTarget = (renderTarget || mUsage == GL_FRAMEBUFFER_ATTACHMENT_ANGLE);

    setCompleteTexStorage(createCompleteStorage(createRenderTarget));
    ASSERT(mTexStorage);

    // flush image data to the storage
    updateStorage();
}

rx::TextureStorageInterface2DArray *Texture2DArray::createCompleteStorage(bool renderTarget) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getLayers(0);

    ASSERT(width > 0 && height > 0 && depth > 0);

    // use existing storage level count, when previously specified by TexStorage*D
    GLint levels = (mTexStorage ? mTexStorage->getLevelCount() : creationLevels(width, height, 1));

    return new rx::TextureStorageInterface2DArray(mRenderer, getBaseLevelInternalFormat(), renderTarget, width, height, depth, levels);
}

void Texture2DArray::setCompleteTexStorage(rx::TextureStorageInterface2DArray *newCompleteTexStorage)
{
    SafeDelete(mTexStorage);
    mTexStorage = newCompleteTexStorage;
    mDirtyImages = true;

    // We do not support managed 2D array storage, as managed storage is ES2/D3D9 only
    ASSERT(!mTexStorage->isManaged());
}

void Texture2DArray::updateStorage()
{
    ASSERT(mTexStorage != NULL);
    GLint storageLevels = mTexStorage->getLevelCount();
    for (int level = 0; level < storageLevels; level++)
    {
        if (isLevelComplete(level))
        {
            updateStorageLevel(level);
        }
    }
}

void Texture2DArray::updateStorageLevel(int level)
{
    ASSERT(level >= 0 && level < (int)ArraySize(mLayerCounts));
    ASSERT(isLevelComplete(level));

    for (int layer = 0; layer < mLayerCounts[level]; layer++)
    {
        ASSERT(mImageArray[level] != NULL && mImageArray[level][layer] != NULL);
        if (mImageArray[level][layer]->isDirty())
        {
            commitRect(level, 0, 0, layer, getWidth(level), getHeight(level));
        }
    }
}

bool Texture2DArray::ensureRenderTarget()
{
    initializeStorage(true);

    if (getBaseLevelWidth() > 0 && getBaseLevelHeight() > 0 && getLayers(0) > 0)
    {
        ASSERT(mTexStorage);
        if (!mTexStorage->isRenderTarget())
        {
            rx::TextureStorageInterface2DArray *newRenderTargetStorage = createCompleteStorage(true);

            if (!mRenderer->copyToRenderTarget(newRenderTargetStorage, mTexStorage))
            {
                delete newRenderTargetStorage;
                return gl::error(GL_OUT_OF_MEMORY, false);
            }

            setCompleteTexStorage(newRenderTargetStorage);
        }
    }

    return (mTexStorage && mTexStorage->isRenderTarget());
}

rx::RenderTarget *Texture2DArray::getRenderTarget(GLint level, GLint layer)
{
    // ensure the underlying texture is created
    if (!ensureRenderTarget())
    {
        return NULL;
    }

    updateStorageLevel(level);

    // ensure this is NOT a depth texture
    if (isDepth(level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(level, layer);
}

rx::RenderTarget *Texture2DArray::getDepthStencil(GLint level, GLint layer)
{
    // ensure the underlying texture is created
    if (!ensureRenderTarget())
    {
        return NULL;
    }

    updateStorageLevel(level);

    // ensure this is a depth texture
    if (!isDepth(level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(level, layer);
}

void Texture2DArray::redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, getBaseLevelWidth() >> level);
    const int storageHeight = std::max(1, getBaseLevelHeight() >> level);
    const int storageDepth = getLayers(0);
    const GLenum storageFormat = getBaseLevelInternalFormat();

    for (int layer = 0; layer < mLayerCounts[level]; layer++)
    {
        delete mImageArray[level][layer];
    }
    delete[] mImageArray[level];
    mImageArray[level] = NULL;
    mLayerCounts[level] = depth;

    if (depth > 0)
    {
        mImageArray[level] = new rx::Image*[depth]();

        for (int layer = 0; layer < mLayerCounts[level]; layer++)
        {
            mImageArray[level][layer] = mRenderer->createImage();
            mImageArray[level][layer]->redefine(mRenderer, GL_TEXTURE_2D_ARRAY, internalformat, width, height, 1, false);
        }
    }

    if (mTexStorage)
    {
        const int storageLevels = mTexStorage->getLevelCount();

        if ((level >= storageLevels && storageLevels != 0) ||
            width != storageWidth ||
            height != storageHeight ||
            depth != storageDepth ||
            internalformat != storageFormat)   // Discard mismatched storage
        {
            for (int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
            {
                for (int layer = 0; layer < mLayerCounts[level]; layer++)
                {
                    mImageArray[level][layer]->markDirty();
                }
            }

            delete mTexStorage;
            mTexStorage = NULL;
            mDirtyImages = true;
        }
    }
}

void Texture2DArray::commitRect(GLint level, GLint xoffset, GLint yoffset, GLint layerTarget, GLsizei width, GLsizei height)
{
    if (isValidLevel(level) && layerTarget < getLayers(level))
    {
        rx::ImageD3D *image = rx::ImageD3D::makeImageD3D(mImageArray[level][layerTarget]);
        if (image->copyToStorage(mTexStorage, level, xoffset, yoffset, layerTarget, width, height))
        {
            image->markClean();
        }
    }
}

}
