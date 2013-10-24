#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
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
#include "libGLESv2/renderer/TextureStorage.h"
#include "libEGL/Surface.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/renderer/BufferStorage.h"
#include "libGLESv2/renderer/RenderTarget.h"

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

Texture::Texture(rx::Renderer *renderer, GLuint id, GLenum target) : RefCountObject(id)
{
    mRenderer = renderer;

    mSamplerState.minFilter = GL_NEAREST_MIPMAP_LINEAR;
    mSamplerState.magFilter = GL_LINEAR;
    mSamplerState.wrapS = GL_REPEAT;
    mSamplerState.wrapT = GL_REPEAT;
    mSamplerState.wrapR = GL_REPEAT;
    mSamplerState.maxAnisotropy = 1.0f;
    mSamplerState.lodOffset = 0;
    mSamplerState.compareMode = GL_NONE;
    mSamplerState.compareFunc = GL_LEQUAL;
    mUsage = GL_NONE;

    mDirtyImages = true;

    mImmutable = false;

    mTarget = target;
}

Texture::~Texture()
{
}

GLenum Texture::getTarget() const
{
    return mTarget;
}

void Texture::addProxyRef(const Renderbuffer *proxy)
{
    mRenderbufferProxies.addRef(proxy);
}

void Texture::releaseProxy(const Renderbuffer *proxy)
{
    mRenderbufferProxies.release(proxy);
}

void Texture::setMinFilter(GLenum filter)
{
    mSamplerState.minFilter = filter;
}

void Texture::setMagFilter(GLenum filter)
{
    mSamplerState.magFilter = filter;
}

void Texture::setWrapS(GLenum wrap)
{
    mSamplerState.wrapS = wrap;
}

void Texture::setWrapT(GLenum wrap)
{
    mSamplerState.wrapT = wrap;
}

void Texture::setWrapR(GLenum wrap)
{
    mSamplerState.wrapR = wrap;
}

void Texture::setMaxAnisotropy(float textureMaxAnisotropy, float contextMaxAnisotropy)
{
    mSamplerState.maxAnisotropy = std::min(textureMaxAnisotropy, contextMaxAnisotropy);
}

void Texture::setCompareMode(GLenum mode)
{
    mSamplerState.compareMode = mode;
}

void Texture::setCompareFunc(GLenum func)
{
    mSamplerState.compareFunc = func;
}

void Texture::setUsage(GLenum usage)
{
    mUsage = usage;
}

GLenum Texture::getMinFilter() const
{
    return mSamplerState.minFilter;
}

GLenum Texture::getMagFilter() const
{
    return mSamplerState.magFilter;
}

GLenum Texture::getWrapS() const
{
    return mSamplerState.wrapS;
}

GLenum Texture::getWrapT() const
{
    return mSamplerState.wrapT;
}

GLenum Texture::getWrapR() const
{
    return mSamplerState.wrapR;
}

float Texture::getMaxAnisotropy() const
{
    return mSamplerState.maxAnisotropy;
}

int Texture::getLodOffset()
{
    rx::TextureStorageInterface *texture = getStorage(false);
    return texture ? texture->getLodOffset() : 0;
}

void Texture::getSamplerState(SamplerState *sampler)
{
    *sampler = mSamplerState;
    sampler->lodOffset = getLodOffset();
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

GLenum Texture::getBaseLevelInternalFormat() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getInternalFormat() : GL_NONE);
}

void Texture::setImage(const PixelUnpackState &unpack, GLenum type, const void *pixels, rx::Image *image)
{
    // We no longer need the "GLenum format" parameter to TexImage to determine what data format "pixels" contains.
    // From our image internal format we know how many channels to expect, and "type" gives the format of pixel's components.
    const void *pixelData = pixels;

    if (unpack.pixelBuffer.id() != 0)
    {
        // Do a CPU readback here, if we have an unpack buffer bound and the fast GPU path is not supported
        Buffer *pixelBuffer = unpack.pixelBuffer.get();
        ptrdiff_t offset = reinterpret_cast<ptrdiff_t>(pixels);
        const void *bufferData = pixelBuffer->getStorage()->getData();
        pixelData = static_cast<const unsigned char *>(bufferData) + offset;
    }

    if (pixelData != NULL)
    {
        image->loadData(0, 0, 0, image->getWidth(), image->getHeight(), image->getDepth(), unpack.alignment, type, pixelData);
        mDirtyImages = true;
    }
}

bool Texture::isFastUnpackable(const PixelUnpackState &unpack, GLenum sizedInternalFormat)
{
    return unpack.pixelBuffer.id() != 0 && mRenderer->supportsFastCopyBufferToTexture(sizedInternalFormat);
}

bool Texture::fastUnpackPixels(const PixelUnpackState &unpack, const void *pixels, const Box &destArea,
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

void Texture::setCompressedImage(GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(0, 0, 0, image->getWidth(), image->getHeight(), image->getDepth(), pixels);
        mDirtyImages = true;
    }
}

bool Texture::subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels, rx::Image *image)
{
    const void *pixelData = pixels;

    // CPU readback & copy where direct GPU copy is not supported
    if (unpack.pixelBuffer.id() != 0)
    {
        Buffer *pixelBuffer = unpack.pixelBuffer.get();
        unsigned int offset = reinterpret_cast<unsigned int>(pixels);
        const void *bufferData = pixelBuffer->getStorage()->getData();
        pixelData = static_cast<const unsigned char *>(bufferData) + offset;
    }

    if (pixelData != NULL)
    {
        image->loadData(xoffset, yoffset, zoffset, width, height, depth, unpack.alignment, type, pixelData);
        mDirtyImages = true;
    }

    return true;
}

bool Texture::subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                 GLenum format, GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(xoffset, yoffset, zoffset, width, height, depth, pixels);
        mDirtyImages = true;
    }

    return true;
}

rx::TextureStorageInterface *Texture::getNativeTexture()
{
    // ensure the underlying texture is created

    rx::TextureStorageInterface *storage = getStorage(false);
    if (storage)
    {
        updateStorage();
    }

    return storage;
}

bool Texture::hasDirtyImages() const
{
    return mDirtyImages;
}

void Texture::resetDirty()
{
    mDirtyImages = false;
}

unsigned int Texture::getTextureSerial()
{
    rx::TextureStorageInterface *texture = getStorage(false);
    return texture ? texture->getTextureSerial() : 0;
}

bool Texture::isImmutable() const
{
    return mImmutable;
}

GLint Texture::creationLevels(GLsizei width, GLsizei height, GLsizei depth) const
{
    // NPOT checks are not required in ES 3.0, NPOT texture support is assumed.
    return 0;   // Maximum number of levels
}

GLint Texture::creationLevels(GLsizei width, GLsizei height) const
{
    if ((isPow2(width) && isPow2(height)) || mRenderer->getNonPower2TextureSupport())
    {
        return 0;   // Maximum number of levels
    }
    else
    {
        // OpenGL ES 2.0 without GL_OES_texture_npot does not permit NPOT mipmaps.
        return 1;
    }
}

GLint Texture::creationLevels(GLsizei size) const
{
    return creationLevels(size, size);
}

int Texture::mipLevels() const
{
    return log2(std::max(std::max(getBaseLevelWidth(), getBaseLevelHeight()), getBaseLevelDepth()));
}

Texture2D::Texture2D(rx::Renderer *renderer, GLuint id) : Texture(renderer, id, GL_TEXTURE_2D)
{
    mTexStorage = NULL;
    mSurface = NULL;

    for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        mImageArray[i] = renderer->createImage();
    }
}

Texture2D::~Texture2D()
{
    delete mTexStorage;
    mTexStorage = NULL;
    
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;
    }

    for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        delete mImageArray[i];
    }
}

GLsizei Texture2D::getWidth(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getWidth();
    else
        return 0;
}

GLsizei Texture2D::getHeight(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getHeight();
    else
        return 0;
}

GLenum Texture2D::getInternalFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getInternalFormat();
    else
        return GL_NONE;
}

GLenum Texture2D::getActualFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getActualFormat();
    else
        return D3DFMT_UNKNOWN;
}

void Texture2D::redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height)
{
    releaseTexImage();

    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, getBaseLevelWidth() >> level);
    const int storageHeight = std::max(1, getBaseLevelHeight() >> level);
    const GLenum storageFormat = getBaseLevelInternalFormat();

    mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, internalformat, width, height, 1, false);

    if (mTexStorage)
    {
        const int storageLevels = mTexStorage->levelCount();
        
        if ((level >= storageLevels && storageLevels != 0) ||
            width != storageWidth ||
            height != storageHeight ||
            internalformat != storageFormat)   // Discard mismatched storage
        {
            for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
            {
                mImageArray[i]->markDirty();
            }

            delete mTexStorage;
            mTexStorage = NULL;
            mDirtyImages = true;
        }
    }
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLenum sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
                                                                                      : GetSizedInternalFormat(format, type, clientVersion);
    redefineImage(level, sizedInternalFormat, width, height);

    bool fastUnpacked = false;

    // Attempt a fast gpu copy of the pixel data to the surface
    if (isFastUnpackable(unpack, sizedInternalFormat) && isLevelComplete(level))
    {
        // Will try to create RT storage if it does not exist
        rx::RenderTarget *destRenderTarget = getRenderTarget(level);
        Box destArea(0, 0, 0, getWidth(level), getHeight(level), 1);

        if (destRenderTarget && fastUnpackPixels(unpack, pixels, destArea, sizedInternalFormat, type, destRenderTarget))
        {
            // Ensure we don't overwrite our newly initialized data
            mImageArray[level]->markClean();

            fastUnpacked = true;
        }
    }

    if (!fastUnpacked)
    {
        Texture::setImage(unpack, type, pixels, mImageArray[level]);
    }
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();

    GLenum internalformat = surface->getFormat();

    mImageArray[0]->redefine(mRenderer, GL_TEXTURE_2D, internalformat, surface->getWidth(), surface->getHeight(), 1, true);

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2D(mRenderer, surface->getSwapChain());

    mDirtyImages = true;
    mSurface = surface;
    mSurface->setBoundTexture(this);
}

void Texture2D::releaseTexImage()
{
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;

        if (mTexStorage)
        {
            delete mTexStorage;
            mTexStorage = NULL;
        }

        for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
        {
            mImageArray[i]->redefine(mRenderer, GL_TEXTURE_2D, GL_NONE, 0, 0, 0, true);
        }
    }
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height);

    Texture::setCompressedImage(imageSize, pixels, mImageArray[level]);
}

void Texture2D::commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    if (level < levelCount())
    {
        rx::Image *image = mImageArray[level];
        if (image->copyToStorage(mTexStorage, level, xoffset, yoffset, width, height))
        {
            image->markClean();
        }
    }
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    bool fastUnpacked = false;

    if (isFastUnpackable(unpack, getInternalFormat(level)) && isLevelComplete(level))
    {
        rx::RenderTarget *renderTarget = getRenderTarget(level);
        Box destArea(xoffset, yoffset, 0, width, height, 1);

        if (renderTarget && fastUnpackPixels(unpack, pixels, destArea, getInternalFormat(level), type, renderTarget))
        {
            // Ensure we don't overwrite our newly initialized data
            mImageArray[level]->markClean();

            fastUnpacked = true;
        }
    }

    if (!fastUnpacked && Texture::subImage(xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (Texture::subImageCompressed(xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLenum sizedInternalFormat = IsSizedInternalFormat(format, clientVersion) ? format
                                                                              : GetSizedInternalFormat(format, GL_UNSIGNED_BYTE, clientVersion);
    redefineImage(level, sizedInternalFormat, width, height);

    if (!mImageArray[level]->isRenderableFormat())
    {
        mImageArray[level]->copy(0, 0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        ensureRenderTarget();
        mImageArray[level]->markClean();

        if (width != 0 && height != 0 && level < levelCount())
        {
            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, format, 0, 0, mTexStorage, level);
        }
    }
}

void Texture2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    if (xoffset + width > mImageArray[level]->getWidth() || yoffset + height > mImageArray[level]->getHeight() || zoffset != 0)
    {
        return gl::error(GL_INVALID_VALUE);
    }

    // can only make our texture storage to a render target if level 0 is defined (with a width & height) and
    // the current level we're copying to is defined (with appropriate format, width & height)
    bool canCreateRenderTarget = isLevelComplete(level) && isLevelComplete(0);

    if (!mImageArray[level]->isRenderableFormat() || (!mTexStorage && !canCreateRenderTarget))
    {
        mImageArray[level]->copy(xoffset, yoffset, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        ensureRenderTarget();
        
        if (level < levelCount())
        {
            updateStorageLevel(level);

            GLuint clientVersion = mRenderer->getCurrentClientVersion();

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect,
                                 gl::GetFormat(getBaseLevelInternalFormat(), clientVersion),
                                 xoffset, yoffset, mTexStorage, level);
        }
    }
}

void Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    for (int level = 0; level < levels; level++)
    {
        GLsizei levelWidth = std::max(1, width >> level);
        GLsizei levelHeight = std::max(1, height >> level);
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, internalformat, levelWidth, levelHeight, 1, true);
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, GL_NONE, 0, 0, 0, true);
    }

    mImmutable = true;

    setCompleteTexStorage(new rx::TextureStorageInterface2D(mRenderer, levels, internalformat, IsRenderTargetUsage(mUsage), width, height));
}

void Texture2D::setCompleteTexStorage(rx::TextureStorageInterface2D *newCompleteTexStorage)
{
    SafeDelete(mTexStorage);
    mTexStorage = newCompleteTexStorage;

    if (mTexStorage && mTexStorage->isManaged())
    {
        for (int level = 0; level < mTexStorage->levelCount(); level++)
        {
            mImageArray[level]->setManagedSurface(mTexStorage, level);
        }
    }

    mDirtyImages = true;
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete(const SamplerState &samplerState) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (!IsTextureFilteringSupported(getInternalFormat(0), mRenderer))
    {
        if (samplerState.magFilter != GL_NEAREST ||
            (samplerState.minFilter != GL_NEAREST && samplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    bool npotSupport = mRenderer->getNonPower2TextureSupport();

    if (!npotSupport)
    {
        if ((samplerState.wrapS != GL_CLAMP_TO_EDGE && !isPow2(width)) ||
            (samplerState.wrapT != GL_CLAMP_TO_EDGE && !isPow2(height)))
        {
            return false;
        }
    }

    if (IsMipmapFiltered(samplerState))
    {
        if (!npotSupport)
        {
            if (!isPow2(width) || !isPow2(height))
            {
                return false;
            }
        }

        if (!isMipmapComplete())
        {
            return false;
        }
    }

    // OpenGLES 3.0.2 spec section 3.8.13 states that a texture is not mipmap complete if:
    // The internalformat specified for the texture arrays is a sized internal depth or
    // depth and stencil format (see table 3.13), the value of TEXTURE_COMPARE_-
    // MODE is NONE, and either the magnification filter is not NEAREST or the mini-
    // fication filter is neither NEAREST nor NEAREST_MIPMAP_NEAREST.
    if (gl::GetDepthBits(getInternalFormat(0), mRenderer->getCurrentClientVersion()) > 0)
    {
        if (mSamplerState.compareMode == GL_NONE)
        {
            if ((mSamplerState.minFilter != GL_NEAREST && mSamplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST) ||
                mSamplerState.magFilter != GL_NEAREST)
            {
                return false;
            }
        }
    }

    return true;
}

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isMipmapComplete() const
{
    int q = mipLevels();

    for (int level = 0; level <= q; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2D::isLevelComplete(int level) const
{
    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    // The base image level is complete if the width and height are positive
    if (level == 0)
    {
        return true;
    }

    ASSERT(level >= 1 && level <= (int)ArraySize(mImageArray) && mImageArray[level] != NULL);
    rx::Image *image = mImageArray[level];

    if (image->getInternalFormat() != getBaseLevelInternalFormat())
    {
        return false;
    }

    if (image->getWidth() != std::max(1, width >> level))
    {
        return false;
    }

    if (image->getHeight() != std::max(1, height >> level))
    {
        return false;
    }

    return true;
}

bool Texture2D::isCompressed(GLint level) const
{
    return IsFormatCompressed(getInternalFormat(level), mRenderer->getCurrentClientVersion());
}

bool Texture2D::isDepth(GLint level) const
{
    return GetDepthBits(getInternalFormat(level), mRenderer->getCurrentClientVersion()) > 0;
}

// Constructs a native texture resource from the texture images
void Texture2D::initializeStorage(bool renderTarget)
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

    bool createRenderTarget = (renderTarget || IsRenderTargetUsage(mUsage));

    setCompleteTexStorage(createCompleteStorage(createRenderTarget));
    ASSERT(mTexStorage);

    // flush image data to the storage
    updateStorage();
}

rx::TextureStorageInterface2D *Texture2D::createCompleteStorage(bool renderTarget) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();

    ASSERT(width > 0 && height > 0);

    // use existing storage level count, when previously specified by TexStorage*D
    GLint levels = (mTexStorage ? mTexStorage->levelCount() : creationLevels(width, height));

    return new rx::TextureStorageInterface2D(mRenderer, levels, getBaseLevelInternalFormat(), renderTarget, width, height);
}

void Texture2D::updateStorage()
{
    int storageLevels = levelCount();

    for (int level = 0; level < storageLevels; level++)
    {
        if (isLevelComplete(level))
        {
            updateStorageLevel(level);
        }
    }
}

void Texture2D::updateStorageLevel(int level)
{
    ASSERT(level <= (int)ArraySize(mImageArray) && mImageArray[level] != NULL);
    ASSERT(isLevelComplete(level));

    if (mImageArray[level]->isDirty())
    {
        commitRect(level, 0, 0, getWidth(level), getHeight(level));
    }
}

bool Texture2D::ensureRenderTarget()
{
    initializeStorage(true);

    if (getBaseLevelWidth() > 0 && getBaseLevelHeight() > 0)
    {
        ASSERT(mTexStorage);
        if (!mTexStorage->isRenderTarget())
        {
            rx::TextureStorageInterface2D *newRenderTargetStorage = createCompleteStorage(true);

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

void Texture2D::generateMipmaps()
{
    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    int q = mipLevels();
    for (int level = 1; level <= q; level++)
    {
        redefineImage(level, getBaseLevelInternalFormat(),
                      std::max(getBaseLevelWidth() >> level, 1),
                      std::max(getBaseLevelHeight() >> level, 1));
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (int level = 1; level <= q; level++)
        {
            mTexStorage->generateMipmap(level);

            mImageArray[level]->markClean();
        }
    }
    else
    {
        for (int level = 1; level <= q; level++)
        {
            mRenderer->generateMipmap(mImageArray[level], mImageArray[level - 1]);
        }
    }
}

const rx::Image *Texture2D::getBaseLevelImage() const
{
    return mImageArray[0];
}

Renderbuffer *Texture2D::getRenderbuffer(GLint level)
{
    Renderbuffer *renderBuffer = mRenderbufferProxies.get(level, 0);
    if (!renderBuffer)
    {
        renderBuffer = new Renderbuffer(mRenderer, id(), new RenderbufferTexture2D(this, level));
        mRenderbufferProxies.add(level, 0, renderBuffer);
    }

    return renderBuffer;
}

unsigned int Texture2D::getRenderTargetSerial(GLint level)
{
    return (ensureRenderTarget() ? mTexStorage->getRenderTargetSerial(level) : 0);
}

rx::RenderTarget *Texture2D::getRenderTarget(GLint level)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateStorageLevel(level);

    // ensure this is NOT a depth texture
    if (isDepth(level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(level);
}

rx::RenderTarget *Texture2D::getDepthSencil(GLint level)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateStorageLevel(level);

    // ensure this is actually a depth texture
    if (!isDepth(level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(level);
}

int Texture2D::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() : 0;
}

rx::TextureStorageInterface *Texture2D::getStorage(bool renderTarget)
{
    if (!mTexStorage || (renderTarget && !mTexStorage->isRenderTarget()))
    {
        if (renderTarget)
        {
            ensureRenderTarget();
        }
        else
        {
            initializeStorage(false);
        }
    }

    return mTexStorage;
}

TextureCubeMap::TextureCubeMap(rx::Renderer *renderer, GLuint id) : Texture(renderer, id, GL_TEXTURE_CUBE_MAP)
{
    mTexStorage = NULL;
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++j)
        {
            mImageArray[i][j] = renderer->createImage();
        }
    }
}

TextureCubeMap::~TextureCubeMap()
{
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++j)
        {
            delete mImageArray[i][j];
        }
    }

    delete mTexStorage;
    mTexStorage = NULL;
}

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[targetToIndex(target)][level]->getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[targetToIndex(target)][level]->getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[targetToIndex(target)][level]->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[targetToIndex(target)][level]->getActualFormat();
    else
        return D3DFMT_UNKNOWN;
}

void TextureCubeMap::setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(0, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(1, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(2, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(3, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(4, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(5, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    int faceIndex = targetToIndex(target);
    redefineImage(faceIndex, level, format, width, height);

    Texture::setCompressedImage(imageSize, pixels, mImageArray[faceIndex][level]);
}

void TextureCubeMap::commitRect(int faceIndex, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    if (level < levelCount())
    {
        rx::Image *image = mImageArray[faceIndex][level];
        if (image->copyToStorage(mTexStorage, faceIndex, level, xoffset, yoffset, width, height))
            image->markClean();
    }
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    int faceIndex = targetToIndex(target);
    if (Texture::subImage(xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels, mImageArray[faceIndex][level]))
    {
        commitRect(faceIndex, level, xoffset, yoffset, width, height);
    }
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    int faceIndex = targetToIndex(target);
    if (Texture::subImageCompressed(xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels, mImageArray[faceIndex][level]))
    {
        commitRect(faceIndex, level, xoffset, yoffset, width, height);
    }
}

// Tests for cube map sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 86.
bool TextureCubeMap::isSamplerComplete(const SamplerState &samplerState) const
{
    int size = getBaseLevelWidth();

    bool mipmapping = IsMipmapFiltered(samplerState);

    if (!IsTextureFilteringSupported(getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0), mRenderer))
    {
        if (samplerState.magFilter != GL_NEAREST ||
            (samplerState.minFilter != GL_NEAREST && samplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    if (!isPow2(size) && !mRenderer->getNonPower2TextureSupport())
    {
        if (samplerState.wrapS != GL_CLAMP_TO_EDGE || samplerState.wrapT != GL_CLAMP_TO_EDGE || mipmapping)
        {
            return false;
        }
    }

    if (!mipmapping)
    {
        if (!isCubeComplete())
        {
            return false;
        }
    }
    else
    {
        if (!isMipmapCubeComplete())   // Also tests for isCubeComplete()
        {
            return false;
        }
    }

    return true;
}

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isCubeComplete() const
{
    int    baseWidth  = getBaseLevelWidth();
    int    baseHeight = getBaseLevelHeight();
    GLenum baseFormat = getBaseLevelInternalFormat();

    if (baseWidth <= 0 || baseWidth != baseHeight)
    {
        return false;
    }

    for (int faceIndex = 1; faceIndex < 6; faceIndex++)
    {
        const rx::Image &faceBaseImage = *mImageArray[faceIndex][0];

        if (faceBaseImage.getWidth()          != baseWidth  ||
            faceBaseImage.getHeight()         != baseHeight ||
            faceBaseImage.getInternalFormat() != baseFormat )
        {
            return false;
        }
    }

    return true;
}

bool TextureCubeMap::isMipmapCubeComplete() const
{
    if (isImmutable())
    {
        return true;
    }

    if (!isCubeComplete())
    {
        return false;
    }

    int q = mipLevels();

    for (int face = 0; face < 6; face++)
    {
        for (int level = 1; level <= q; level++)
        {
            if (!isFaceLevelComplete(face, level))
            {
                return false;
            }
        }
    }

    return true;
}

bool TextureCubeMap::isFaceLevelComplete(int faceIndex, int level) const
{
    ASSERT(level >= 0 && faceIndex < 6 && level < (int)ArraySize(mImageArray[faceIndex]) && mImageArray[faceIndex][level] != NULL);

    if (isImmutable())
    {
        return true;
    }

    int baseSize = getBaseLevelWidth();

    if (baseSize <= 0)
    {
        return false;
    }

    // "isCubeComplete" checks for base level completeness and we must call that
    // to determine if any face at level 0 is complete. We omit that check here
    // to avoid re-checking cube-completeness for every face at level 0.
    if (level == 0)
    {
        return true;
    }

    // Check that non-zero levels are consistent with the base level.
    const rx::Image *faceLevelImage = mImageArray[faceIndex][level];

    if (faceLevelImage->getInternalFormat() != getBaseLevelInternalFormat())
    {
        return false;
    }

    if (faceLevelImage->getWidth() != std::max(1, baseSize >> level))
    {
        return false;
    }

    return true;
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
    return IsFormatCompressed(getInternalFormat(target, level), mRenderer->getCurrentClientVersion());
}

bool TextureCubeMap::isDepth(GLenum target, GLint level) const
{
    return GetDepthBits(getInternalFormat(target, level), mRenderer->getCurrentClientVersion()) > 0;
}

// Constructs a native texture resource from the texture images, or returns an existing one
void TextureCubeMap::initializeStorage(bool renderTarget)
{
    GLsizei size = getBaseLevelWidth();

    if (!(size > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(size);
    GLenum internalformat = getBaseLevelInternalFormat();

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, IsRenderTargetUsage(mUsage), size);

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            for (int level = 0; level < levels; level++)
            {
                mImageArray[faceIndex][level]->setManagedSurface(mTexStorage, faceIndex, level);
            }
        }
    }

    mDirtyImages = true;
}

void TextureCubeMap::updateStorage()
{
    int storageLevels = levelCount();

    for (int face = 0; face < 6; face++)
    {
        for (int level = 0; level < storageLevels; level++)
        {
            if (isFaceLevelComplete(face, level))
            {
                updateStorageFaceLevel(face, level);
            }
        }
    }
}

void TextureCubeMap::updateStorageFaceLevel(int faceIndex, int level)
{
    ASSERT(level >= 0 && faceIndex < 6 && level < (int)ArraySize(mImageArray[faceIndex]) && mImageArray[faceIndex][level] != NULL);
    rx::Image *image = mImageArray[faceIndex][level];

    if (image->isDirty())
    {
        commitRect(faceIndex, level, 0, 0, image->getWidth(), image->getHeight());
    }
}

bool TextureCubeMap::ensureRenderTarget()
{
    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        return true;
    }

    rx::TextureStorageInterfaceCube *newTexStorage = NULL;

    if (getBaseLevelWidth() != 0)
    {
        GLsizei size = getBaseLevelWidth();
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(size);
        GLenum internalformat = getBaseLevelInternalFormat();

        newTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, true, size);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY, false);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
    return (mTexStorage && mTexStorage->isRenderTarget());
}

void TextureCubeMap::setImage(int faceIndex, GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLenum sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
                                                                                      : GetSizedInternalFormat(format, type, clientVersion);

    redefineImage(faceIndex, level, sizedInternalFormat, width, height);

    Texture::setImage(unpack, type, pixels, mImageArray[faceIndex][level]);
}

int TextureCubeMap::targetToIndex(GLenum target)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return target - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
}

void TextureCubeMap::redefineImage(int faceIndex, GLint level, GLenum internalformat, GLsizei width, GLsizei height)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, getBaseLevelWidth() >> level);
    const int storageHeight = std::max(1, getBaseLevelHeight() >> level);
    const GLenum storageFormat = getBaseLevelInternalFormat();

    mImageArray[faceIndex][level]->redefine(mRenderer, GL_TEXTURE_CUBE_MAP, internalformat, width, height, 1, false);

    if (mTexStorage)
    {
        const int storageLevels = mTexStorage->levelCount();
        
        if ((level >= storageLevels && storageLevels != 0) ||
            width != storageWidth ||
            height != storageHeight ||
            internalformat != storageFormat)   // Discard mismatched storage
        {
            for (int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
            {
                for (int faceIndex = 0; faceIndex < 6; faceIndex++)
                {
                    mImageArray[faceIndex][level]->markDirty();
                }
            }

            delete mTexStorage;
            mTexStorage = NULL;

            mDirtyImages = true;
        }
    }
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    int faceIndex = targetToIndex(target);
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLenum sizedInternalFormat = IsSizedInternalFormat(format, clientVersion) ? format
                                                                              : GetSizedInternalFormat(format, GL_UNSIGNED_BYTE, clientVersion);
    redefineImage(faceIndex, level, sizedInternalFormat, width, height);

    if (!mImageArray[faceIndex][level]->isRenderableFormat())
    {
        mImageArray[faceIndex][level]->copy(0, 0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        ensureRenderTarget();
        mImageArray[faceIndex][level]->markClean();

        ASSERT(width == height);

        if (width > 0 && level < levelCount())
        {
            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, format, 0, 0, mTexStorage, target, level);
        }
    }
}

void TextureCubeMap::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    int faceIndex = targetToIndex(target);

    GLsizei size = mImageArray[faceIndex][level]->getWidth();

    if (xoffset + width > size || yoffset + height > size || zoffset != 0)
    {
        return gl::error(GL_INVALID_VALUE);
    }

    // We can only make our texture storage to a render target if the level we're copying *to* is complete
    // and the base level is cube-complete. The base level must be cube complete (common case) because we cannot
    // rely on the "getBaseLevel*" methods reliably otherwise.
    bool canCreateRenderTarget = isFaceLevelComplete(faceIndex, level) && isCubeComplete();

    if (!mImageArray[faceIndex][level]->isRenderableFormat() || (!mTexStorage && !canCreateRenderTarget))
    {
        mImageArray[faceIndex][level]->copy(0, 0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        ensureRenderTarget();
        
        if (level < levelCount())
        {
            updateStorageFaceLevel(faceIndex, level);

            GLuint clientVersion = mRenderer->getCurrentClientVersion();

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, gl::GetFormat(getBaseLevelInternalFormat(), clientVersion),
                                 xoffset, yoffset, mTexStorage, target, level);
        }
    }
}

void TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, IsRenderTargetUsage(mUsage), size);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        GLsizei mipSize = std::max(1, size >> level);
        for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            mImageArray[faceIndex][level]->redefine(mRenderer, GL_TEXTURE_CUBE_MAP, internalformat, mipSize, mipSize, 1, true);
        }
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            mImageArray[faceIndex][level]->redefine(mRenderer, GL_TEXTURE_CUBE_MAP, GL_NONE, 0, 0, 0, true);
        }
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            for (int level = 0; level < levels; level++)
            {
                mImageArray[faceIndex][level]->setManagedSurface(mTexStorage, faceIndex, level);
            }
        }
    }
}

void TextureCubeMap::generateMipmaps()
{
    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    int q = mipLevels();
    for (int faceIndex = 0; faceIndex < 6; faceIndex++)
    {
        for (int level = 1; level <= q; level++)
        {
            int faceLevelSize = (std::max(mImageArray[faceIndex][0]->getWidth() >> level, 1));
            redefineImage(faceIndex, level, mImageArray[faceIndex][0]->getInternalFormat(), faceLevelSize, faceLevelSize);
        }
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            for (int level = 1; level <= q; level++)
            {
                mTexStorage->generateMipmap(faceIndex, level);

                mImageArray[faceIndex][level]->markClean();
            }
        }
    }
    else
    {
        for (int faceIndex = 0; faceIndex < 6; faceIndex++)
        {
            for (int level = 1; level <= q; level++)
            {
                mRenderer->generateMipmap(mImageArray[faceIndex][level], mImageArray[faceIndex][level - 1]);
            }
        }
    }
}

const rx::Image *TextureCubeMap::getBaseLevelImage() const
{
    // Note: if we are not cube-complete, there is no single base level image that can describe all
    // cube faces, so this method is only well-defined for a cube-complete base level.
    return mImageArray[0][0];
}

Renderbuffer *TextureCubeMap::getRenderbuffer(GLenum target, GLint level)
{
    if (!IsCubemapTextureTarget(target))
    {
        return gl::error(GL_INVALID_OPERATION, (Renderbuffer *)NULL);
    }

    int faceIndex = targetToIndex(target);

    Renderbuffer *renderBuffer = mRenderbufferProxies.get(level, faceIndex);
    if (!renderBuffer)
    {
        renderBuffer = new Renderbuffer(mRenderer, id(), new RenderbufferTextureCubeMap(this, target, level));
        mRenderbufferProxies.add(level, faceIndex, renderBuffer);
    }

    return renderBuffer;
}

unsigned int TextureCubeMap::getRenderTargetSerial(GLenum target, GLint level)
{
    return (ensureRenderTarget() ? mTexStorage->getRenderTargetSerial(target, level) : 0);
}

rx::RenderTarget *TextureCubeMap::getRenderTarget(GLenum target, GLint level)
{
    ASSERT(IsCubemapTextureTarget(target));

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateStorageFaceLevel(targetToIndex(target), level);

    // ensure this is NOT a depth texture
    if (isDepth(target, level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(target, level);
}

rx::RenderTarget *TextureCubeMap::getDepthStencil(GLenum target, GLint level)
{
    ASSERT(IsCubemapTextureTarget(target));

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateStorageFaceLevel(targetToIndex(target), level);

    // ensure this is a depth texture
    if (!isDepth(target, level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(target, level);
}

int TextureCubeMap::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() - getLodOffset() : 0;
}

rx::TextureStorageInterface *TextureCubeMap::getStorage(bool renderTarget)
{
    if (!mTexStorage || (renderTarget && !mTexStorage->isRenderTarget()))
    {
        if (renderTarget)
        {
            ensureRenderTarget();
        }
        else
        {
            initializeStorage(false);
        }
    }

    return mTexStorage;
}

Texture3D::Texture3D(rx::Renderer *renderer, GLuint id) : Texture(renderer, id, GL_TEXTURE_3D)
{
    mTexStorage = NULL;

    for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        mImageArray[i] = renderer->createImage();
    }
}

Texture3D::~Texture3D()
{
    delete mTexStorage;
    mTexStorage = NULL;

    for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        delete mImageArray[i];
    }
}

GLsizei Texture3D::getWidth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mImageArray[level]->getWidth() : 0;
}

GLsizei Texture3D::getHeight(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mImageArray[level]->getHeight() : 0;
}

GLsizei Texture3D::getDepth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mImageArray[level]->getDepth() : 0;
}

GLenum Texture3D::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mImageArray[level]->getInternalFormat() : GL_NONE;
}

GLenum Texture3D::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS) ? mImageArray[level]->getActualFormat() : D3DFMT_UNKNOWN;
}

bool Texture3D::isCompressed(GLint level) const
{
    return IsFormatCompressed(getInternalFormat(level), mRenderer->getCurrentClientVersion());
}

bool Texture3D::isDepth(GLint level) const
{
    return GetDepthBits(getInternalFormat(level), mRenderer->getCurrentClientVersion()) > 0;
}

void Texture3D::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLenum sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
                                                                                      : GetSizedInternalFormat(format, type, clientVersion);
    redefineImage(level, sizedInternalFormat, width, height, depth);

    bool fastUnpacked = false;

    // Attempt a fast gpu copy of the pixel data to the surface if the app bound an unpack buffer
    if (isFastUnpackable(unpack, sizedInternalFormat))
    {
        // Will try to create RT storage if it does not exist
        rx::RenderTarget *destRenderTarget = getRenderTarget(level);
        Box destArea(0, 0, 0, getWidth(level), getHeight(level), getDepth(level));

        if (destRenderTarget && fastUnpackPixels(unpack, pixels, destArea, sizedInternalFormat, type, destRenderTarget))
        {
            // Ensure we don't overwrite our newly initialized data
            mImageArray[level]->markClean();

            fastUnpacked = true;
        }
    }

    if (!fastUnpacked)
    {
        Texture::setImage(unpack, type, pixels, mImageArray[level]);
    }
}

void Texture3D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height, depth);

    Texture::setCompressedImage(imageSize, pixels, mImageArray[level]);
}

void Texture3D::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    bool fastUnpacked = false;

    // Attempt a fast gpu copy of the pixel data to the surface if the app bound an unpack buffer
    if (isFastUnpackable(unpack, getInternalFormat(level)))
    {
        rx::RenderTarget *destRenderTarget = getRenderTarget(level);
        Box destArea(xoffset, yoffset, zoffset, width, height, depth);

        if (destRenderTarget && fastUnpackPixels(unpack, pixels, destArea, getInternalFormat(level), type, destRenderTarget))
        {
            // Ensure we don't overwrite our newly initialized data
            mImageArray[level]->markClean();

            fastUnpacked = true;
        }
    }

    if (!fastUnpacked && Texture::subImage(xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, zoffset, width, height, depth);
    }
}

void Texture3D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (Texture::subImageCompressed(xoffset, yoffset, zoffset, width, height, depth, format, imageSize, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, zoffset, width, height, depth);
    }
}

void Texture3D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface3D(mRenderer, levels, internalformat, IsRenderTargetUsage(mUsage), width, height, depth);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_3D, internalformat, width, height, depth, true);
        width = std::max(1, width >> 1);
        height = std::max(1, height >> 1);
        depth = std::max(1, depth >> 1);
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_3D, GL_NONE, 0, 0, 0, true);
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int level = 0; level < levels; level++)
        {
            mImageArray[level]->setManagedSurface(mTexStorage, level);
        }
    }
}


void Texture3D::generateMipmaps()
{
    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    int q = mipLevels();
    for (int level = 1; level <= q; level++)
    {
        redefineImage(level, getBaseLevelInternalFormat(),
                      std::max(getBaseLevelWidth() >> level, 1),
                      std::max(getBaseLevelHeight() >> level, 1),
                      std::max(getBaseLevelDepth() >> level, 1));
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (int level = 1; level <= q; level++)
        {
            mTexStorage->generateMipmap(level);

            mImageArray[level]->markClean();
        }
    }
    else
    {
        for (int level = 1; level <= q; level++)
        {
            mRenderer->generateMipmap(mImageArray[level], mImageArray[level - 1]);
        }
    }
}

const rx::Image *Texture3D::getBaseLevelImage() const
{
    return mImageArray[0];
}

void Texture3D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    if (xoffset + width > mImageArray[level]->getWidth() || yoffset + height > mImageArray[level]->getHeight() || zoffset >= mImageArray[level]->getDepth())
    {
        return gl::error(GL_INVALID_VALUE);
    }

    // can only make our texture storage to a render target if level 0 is defined (with a width & height) and
    // the current level we're copying to is defined (with appropriate format, width & height)
    bool canCreateRenderTarget = isLevelComplete(level) && isLevelComplete(0);

    if (!mImageArray[level]->isRenderableFormat() || (!mTexStorage && !canCreateRenderTarget))
    {
        mImageArray[level]->copy(xoffset, yoffset, zoffset, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        ensureRenderTarget();

        if (level < levelCount())
        {
            updateStorageLevel(level);

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            GLuint clientVersion = mRenderer->getCurrentClientVersion();

            mRenderer->copyImage(source, sourceRect,
                                 gl::GetFormat(getBaseLevelInternalFormat(), clientVersion),
                                 xoffset, yoffset, zoffset, mTexStorage, level);
        }
    }
}

bool Texture3D::isSamplerComplete(const SamplerState &samplerState) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (!IsTextureFilteringSupported(getInternalFormat(0), mRenderer))
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

bool Texture3D::isMipmapComplete() const
{
    int q = mipLevels();

    for (int level = 0; level <= q; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture3D::isLevelComplete(int level) const
{
    ASSERT(level >= 0 && level < (int)ArraySize(mImageArray) && mImageArray[level] != NULL);

    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    rx::Image *levelImage = mImageArray[level];

    if (levelImage->getInternalFormat() != getBaseLevelInternalFormat())
    {
        return false;
    }

    if (levelImage->getWidth() != std::max(1, width >> level))
    {
        return false;
    }

    if (levelImage->getHeight() != std::max(1, height >> level))
    {
        return false;
    }

    if (levelImage->getDepth() != std::max(1, depth >> level))
    {
        return false;
    }

    return true;
}

Renderbuffer *Texture3D::getRenderbuffer(GLint level, GLint layer)
{
    Renderbuffer *renderBuffer = mRenderbufferProxies.get(level, layer);
    if (!renderBuffer)
    {
        renderBuffer = new Renderbuffer(mRenderer, id(), new RenderbufferTexture3DLayer(this, level, layer));
        mRenderbufferProxies.add(level, 0, renderBuffer);
    }

    return renderBuffer;
}

unsigned int Texture3D::getRenderTargetSerial(GLint level, GLint layer)
{
    return (ensureRenderTarget() ? mTexStorage->getRenderTargetSerial(level, layer) : 0);
}

int Texture3D::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() : 0;
}

void Texture3D::initializeStorage(bool renderTarget)
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (!(width > 0 && height > 0 && depth > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(width, height, depth);

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface3D(mRenderer, levels, getBaseLevelInternalFormat(), IsRenderTargetUsage(mUsage), width, height, depth);

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int level = 0; level < levels; level++)
        {
            mImageArray[level]->setManagedSurface(mTexStorage, level);
        }
    }

    mDirtyImages = true;
}

void Texture3D::updateStorage()
{
    int storageLevels = levelCount();

    for (int level = 0; level < storageLevels; level++)
    {
        if (isLevelComplete(level))
        {
            updateStorageLevel(level);
        }
    }
}

void Texture3D::updateStorageLevel(int level)
{
    ASSERT(level >= 0 && level < (int)ArraySize(mImageArray) && mImageArray[level] != NULL);
    ASSERT(isLevelComplete(level));

    if (mImageArray[level]->isDirty())
    {
        commitRect(level, 0, 0, 0, getWidth(level), getHeight(level), getDepth(level));
    }
}

bool Texture3D::ensureRenderTarget()
{
    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        return true;
    }

    rx::TextureStorageInterface3D *newTexStorage = NULL;

    if (getBaseLevelWidth() != 0 && getBaseLevelHeight() != 0 && getBaseLevelDepth() != 0)
    {
        GLsizei width = getBaseLevelWidth();
        GLsizei height = getBaseLevelHeight();
        GLsizei depth = getBaseLevelDepth();
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(width, height, depth);

        newTexStorage = new rx::TextureStorageInterface3D(mRenderer, levels, getBaseLevelInternalFormat(), true, width, height, depth);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY, false);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
    return (mTexStorage && mTexStorage->isRenderTarget());
}

rx::RenderTarget *Texture3D::getRenderTarget(GLint level)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateStorageLevel(level);

    // ensure this is NOT a depth texture
    if (isDepth(level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(level);
}

rx::RenderTarget *Texture3D::getRenderTarget(GLint level, GLint layer)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateStorage();

    // ensure this is NOT a depth texture
    if (isDepth(level))
    {
        return NULL;
    }

    return mTexStorage->getRenderTarget(level, layer);
}

rx::RenderTarget *Texture3D::getDepthStencil(GLint level, GLint layer)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
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

rx::TextureStorageInterface *Texture3D::getStorage(bool renderTarget)
{
    if (!mTexStorage || (renderTarget && !mTexStorage->isRenderTarget()))
    {
        if (renderTarget)
        {
            ensureRenderTarget();
        }
        else
        {
            initializeStorage(false);
        }
    }

    return mTexStorage;
}

void Texture3D::redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, getBaseLevelWidth() >> level);
    const int storageHeight = std::max(1, getBaseLevelHeight() >> level);
    const int storageDepth = std::max(1, getBaseLevelDepth() >> level);
    const GLenum storageFormat = getBaseLevelInternalFormat();

    mImageArray[level]->redefine(mRenderer, GL_TEXTURE_3D, internalformat, width, height, depth, false);

    if (mTexStorage)
    {
        const int storageLevels = mTexStorage->levelCount();

        if ((level >= storageLevels && storageLevels != 0) ||
            width != storageWidth ||
            height != storageHeight ||
            depth != storageDepth ||
            internalformat != storageFormat)   // Discard mismatched storage
        {
            for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
            {
                mImageArray[i]->markDirty();
            }

            delete mTexStorage;
            mTexStorage = NULL;
            mDirtyImages = true;
        }
    }
}

void Texture3D::commitRect(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth)
{
    if (level < levelCount())
    {
        rx::Image *image = mImageArray[level];
        if (image->copyToStorage(mTexStorage, level, xoffset, yoffset, zoffset, width, height, depth))
        {
            image->markClean();
        }
    }
}

Texture2DArray::Texture2DArray(rx::Renderer *renderer, GLuint id) : Texture(renderer, id, GL_TEXTURE_2D_ARRAY)
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
    for (int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++level)
    {
        for (int layer = 0; layer < mLayerCounts[level]; ++layer)
        {
            delete mImageArray[level][layer];
        }
        delete[] mImageArray[level];
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

GLsizei Texture2DArray::getDepth(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mLayerCounts[level] : 0;
}

GLenum Texture2DArray::getInternalFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mImageArray[level][0]->getInternalFormat() : GL_NONE;
}

GLenum Texture2DArray::getActualFormat(GLint level) const
{
    return (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && mLayerCounts[level] > 0) ? mImageArray[level][0]->getActualFormat() : D3DFMT_UNKNOWN;
}

bool Texture2DArray::isCompressed(GLint level) const
{
    return IsFormatCompressed(getInternalFormat(level), mRenderer->getCurrentClientVersion());
}

bool Texture2DArray::isDepth(GLint level) const
{
    return GetDepthBits(getInternalFormat(level), mRenderer->getCurrentClientVersion()) > 0;
}

void Texture2DArray::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLenum sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
                                                                                      : GetSizedInternalFormat(format, type, clientVersion);
    redefineImage(level, sizedInternalFormat, width, height, depth);

    GLsizei inputDepthPitch = gl::GetDepthPitch(sizedInternalFormat, type, clientVersion, width, height, unpack.alignment);

    for (int i = 0; i < depth; i++)
    {
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;
        Texture::setImage(unpack, type, layerPixels, mImageArray[level][i]);
    }
}

void Texture2DArray::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height, depth);

    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLsizei inputDepthPitch = gl::GetDepthPitch(format, GL_UNSIGNED_BYTE, clientVersion, width, height, 1);

    for (int i = 0; i < depth; i++)
    {
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;
        Texture::setCompressedImage(imageSize, layerPixels, mImageArray[level][i]);
    }
}

void Texture2DArray::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLenum internalformat = getInternalFormat(level);
    GLuint clientVersion =  mRenderer->getCurrentClientVersion();
    GLsizei inputDepthPitch = gl::GetDepthPitch(internalformat, type, clientVersion, width, height, unpack.alignment);

    for (int i = 0; i < depth; i++)
    {
        int layer = zoffset + i;
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;

        if (Texture::subImage(xoffset, yoffset, zoffset, width, height, 1, format, type, unpack, layerPixels, mImageArray[level][layer]))
        {
            commitRect(level, xoffset, yoffset, layer, width, height);
        }
    }
}

void Texture2DArray::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLsizei inputDepthPitch = gl::GetDepthPitch(format, GL_UNSIGNED_BYTE, clientVersion, width, height, 1);

    for (int i = 0; i < depth; i++)
    {
        int layer = zoffset + i;
        const void *layerPixels = pixels ? (reinterpret_cast<const unsigned char*>(pixels) + (inputDepthPitch * i)) : NULL;

        if (Texture::subImageCompressed(xoffset, yoffset, zoffset, width, height, 1, format, imageSize, layerPixels, mImageArray[level][layer]))
        {
            commitRect(level, xoffset, yoffset, layer, width, height);
        }
    }
}

void Texture2DArray::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2DArray(mRenderer, levels, internalformat, IsRenderTargetUsage(mUsage), width, height, depth);
    mImmutable = true;

    for (int level = 0; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        GLsizei levelWidth = std::max(width >> level, 1);
        GLsizei levelHeight = std::max(height >> level, 1);

        // Clear this level
        for (int layer = 0; layer < mLayerCounts[level]; layer++)
        {
            delete mImageArray[level][layer];
        }
        delete[] mImageArray[level];
        mImageArray[level] = NULL;
        mLayerCounts[level] = 0;

        if (level < levels)
        {
            // Create new images for this level
            mImageArray[level] = new rx::Image*[depth]();
            mLayerCounts[level] = depth;

            for (int layer = 0; layer < mLayerCounts[level]; layer++)
            {
                mImageArray[level][layer] = mRenderer->createImage();
                mImageArray[level][layer]->redefine(mRenderer, GL_TEXTURE_2D_ARRAY, internalformat, levelWidth,
                                                    levelHeight, 1, true);
            }
        }
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int level = 0; level < levels; level++)
        {
            for (int layer = 0; layer < mLayerCounts[level]; layer++)
            {
                mImageArray[level][layer]->setManagedSurface(mTexStorage, layer, level);
            }
        }
    }
}

void Texture2DArray::generateMipmaps()
{
    int baseWidth = getBaseLevelWidth();
    int baseHeight = getBaseLevelHeight();
    int baseDepth = getBaseLevelDepth();
    GLenum baseFormat = getBaseLevelInternalFormat();

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    int q = mipLevels();
    for (int level = 1; level <= q; level++)
    {
        redefineImage(level, baseFormat, std::max(baseWidth >> level, 1), std::max(baseHeight >> level, 1), baseDepth);
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (int level = 1; level <= q; level++)
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
        for (int level = 1; level <= q; level++)
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

void Texture2DArray::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    if (xoffset + width > getWidth(level) || yoffset + height > getHeight(level) || zoffset >= getDepth(level) || getDepth(level) == 0)
    {
        return gl::error(GL_INVALID_VALUE);
    }

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

        if (level < levelCount())
        {
            updateStorageLevel(level);

            GLuint clientVersion = mRenderer->getCurrentClientVersion();

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, gl::GetFormat(getInternalFormat(0), clientVersion),
                                 xoffset, yoffset, zoffset, mTexStorage, level);
        }
    }
}

bool Texture2DArray::isSamplerComplete(const SamplerState &samplerState) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (!IsTextureFilteringSupported(getBaseLevelInternalFormat(), mRenderer))
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
    int q = mipLevels();

    for (int level = 1; level <= q; level++)
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
    ASSERT(level >= 0 && level < (int)ArraySize(mImageArray) && mImageArray[level] != NULL);

    if (isImmutable())
    {
        return true;
    }

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
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

    if (getDepth(level) != depth)
    {
        return false;
    }

    return true;
}

Renderbuffer *Texture2DArray::getRenderbuffer(GLint level, GLint layer)
{
    Renderbuffer *renderBuffer = mRenderbufferProxies.get(level, layer);
    if (!renderBuffer)
    {
        renderBuffer = new Renderbuffer(mRenderer, id(), new RenderbufferTexture2DArrayLayer(this, level, layer));
        mRenderbufferProxies.add(level, 0, renderBuffer);
    }

    return renderBuffer;
}

unsigned int Texture2DArray::getRenderTargetSerial(GLint level, GLint layer)
{
    return (ensureRenderTarget() ? mTexStorage->getRenderTargetSerial(level, layer) : 0);
}

int Texture2DArray::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() : 0;
}

void Texture2DArray::initializeStorage(bool renderTarget)
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return; // do not attempt to create native textures for nonexistant data
    }

    GLint levels = creationLevels(width, height);
    GLenum internalformat = getBaseLevelInternalFormat();

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2DArray(mRenderer, levels, internalformat, IsRenderTargetUsage(mUsage), width, height, depth);

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();
        for (int level = 0; level < levels; level++)
        {
            for (int layer = 0; layer < mLayerCounts[level]; layer++)
            {
                mImageArray[level][layer]->setManagedSurface(mTexStorage, layer, level);
            }
        }
    }

    mDirtyImages = true;
}

void Texture2DArray::updateStorage()
{
    int storageLevels = levelCount();

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
    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        return true;
    }

    rx::TextureStorageInterface2DArray *newTexStorage = NULL;

    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();
    GLsizei depth = getBaseLevelDepth();

    if (width != 0 && height != 0 && depth != 0)
    {
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(width, height);
        GLenum internalformat = getInternalFormat(0);

        newTexStorage = new rx::TextureStorageInterface2DArray(mRenderer, levels, internalformat, true, width, height, depth);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY, false);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
    return (mTexStorage && mTexStorage->isRenderTarget());
}

rx::RenderTarget *Texture2DArray::getRenderTarget(GLint level, GLint layer)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
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
    if (getStorage(true) == NULL)
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

rx::TextureStorageInterface *Texture2DArray::getStorage(bool renderTarget)
{
    if (!mTexStorage || (renderTarget && !mTexStorage->isRenderTarget()))
    {
        if (renderTarget)
        {
            ensureRenderTarget();
        }
        else
        {
            initializeStorage(false);
        }
    }

    return mTexStorage;
}

void Texture2DArray::redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, getBaseLevelWidth() >> level);
    const int storageHeight = std::max(1, getBaseLevelHeight() >> level);
    const int storageDepth = getBaseLevelDepth();
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
        const int storageLevels = mTexStorage->levelCount();

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
    if (level < levelCount() && layerTarget < getDepth(level))
    {
        rx::Image *image = mImageArray[level][layerTarget];
        if (image->copyToStorage(mTexStorage, level, xoffset, yoffset, layerTarget, width, height))
        {
            image->markClean();
        }
    }
}

}
