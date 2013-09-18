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
#include "libGLESv2/renderer/Blit9.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/Image.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "libEGL/Surface.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/renderer/BufferStorage.h"

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

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMinFilter(GLenum filter)
{
    switch (filter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        mSamplerState.minFilter = filter;
        return true;
      default:
        return false;
    }
}

// Returns true on successful filter state update (valid enum parameter)
bool Texture::setMagFilter(GLenum filter)
{
    switch (filter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
        mSamplerState.magFilter = filter;
        return true;
      default:
        return false;
    }
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapS(GLenum wrap)
{
    switch (wrap)
    {
      case GL_REPEAT:
      case GL_CLAMP_TO_EDGE:
      case GL_MIRRORED_REPEAT:
        mSamplerState.wrapS = wrap;
        return true;
      default:
        return false;
    }
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapT(GLenum wrap)
{
    switch (wrap)
    {
      case GL_REPEAT:
      case GL_CLAMP_TO_EDGE:
      case GL_MIRRORED_REPEAT:
        mSamplerState.wrapT = wrap;
        return true;
      default:
        return false;
    }
}

// Returns true on successful wrap state update (valid enum parameter)
bool Texture::setWrapR(GLenum wrap)
{
    switch (wrap)
    {
      case GL_REPEAT:
      case GL_CLAMP_TO_EDGE:
      case GL_MIRRORED_REPEAT:
        mSamplerState.wrapR = wrap;
        return true;
      default:
        return false;
    }
}

// Returns true on successful max anisotropy update (valid anisotropy value)
bool Texture::setMaxAnisotropy(float textureMaxAnisotropy, float contextMaxAnisotropy)
{
    textureMaxAnisotropy = std::min(textureMaxAnisotropy, contextMaxAnisotropy);
    if (textureMaxAnisotropy < 1.0f)
    {
        return false;
    }

    mSamplerState.maxAnisotropy = textureMaxAnisotropy;

    return true;
}

bool Texture::setCompareMode(GLenum mode)
{
    // Acceptable mode parameters from GLES 3.0.2 spec, table 3.17
    switch (mode)
    {
      case GL_NONE:
      case GL_COMPARE_REF_TO_TEXTURE:
        mSamplerState.compareMode = mode;
        return true;
      default:
        return false;
    }
}

bool Texture::setCompareFunc(GLenum func)
{
    // Acceptable function parameters from GLES 3.0.2 spec, table 3.17
    switch (func)
    {
      case GL_LEQUAL:
      case GL_GEQUAL:
      case GL_LESS:
      case GL_GREATER:
      case GL_EQUAL:
      case GL_NOTEQUAL:
      case GL_ALWAYS:
      case GL_NEVER:
        mSamplerState.compareFunc = func;
        return true;
      default:
        return false;
    }
}

// Returns true on successful usage state update (valid enum parameter)
bool Texture::setUsage(GLenum usage)
{
    switch (usage)
    {
      case GL_NONE:
      case GL_FRAMEBUFFER_ATTACHMENT_ANGLE:
        mUsage = usage;
        return true;
      default:
        return false;
    }
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

bool Texture::fastUnpackPixels(const PixelUnpackState &unpack, const void *pixels, const Box &destArea,
                               GLenum sizedInternalFormat, GLenum type, GLint level)
{
    if (destArea.width <= 0 && destArea.height <= 0 && destArea.depth <= 0)
    {
        return true;
    }

    // In order to perform the fast copy through the shader, we must have the right format, and be able
    // to create a render target.
    if (IsFastCopyBufferToTextureSupported(sizedInternalFormat, mRenderer->getCurrentClientVersion()))
    {
        unsigned int offset = reinterpret_cast<unsigned int>(pixels);
        rx::RenderTarget *destRenderTarget = getStorage(true)->getStorageInstance()->getRenderTarget(level);

        return mRenderer->fastCopyBufferToTexture(unpack, offset, destRenderTarget, sizedInternalFormat, type, destArea);
    }

    // Return false if we do not support fast unpack
    return false;
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
    if (pixels != NULL)
    {
        image->loadData(xoffset, yoffset, zoffset, width, height, depth, unpack.alignment, type, pixels);
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
        updateTexture();
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

void Texture2D::redefineImage(GLint level, GLint internalformat, GLsizei width, GLsizei height)
{
    releaseTexImage();

    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, mImageArray[0]->getWidth() >> level);
    const int storageHeight = std::max(1, mImageArray[0]->getHeight() >> level);
    const int storageFormat = mImageArray[0]->getInternalFormat();

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

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLint sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
                                                                                     : GetSizedInternalFormat(format, type, clientVersion);
    redefineImage(level, sizedInternalFormat, width, height);

    // Attempt a fast gpu copy of the pixel data to the surface
    //   If we want to support rendering (which is necessary for GPU unpack buffers), level 0 must be complete
    Box destArea(0, 0, 0, getWidth(level), getHeight(level), 1);
    if (unpack.pixelBuffer.id() != 0 && isLevelComplete(0) && fastUnpackPixels(unpack, pixels, destArea, sizedInternalFormat, type, level))
    {
        // Ensure we don't overwrite our newly initialized data
        mImageArray[level]->markClean();
    }
    else
    {
        Texture::setImage(unpack, type, pixels, mImageArray[level]);
    }
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();

    GLint internalformat = surface->getFormat();

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
        if (image->updateSurface(mTexStorage, level, xoffset, yoffset, width, height))
        {
            image->markClean();
        }
    }
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    if (Texture::subImage(xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels, mImageArray[level]))
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
    GLint sizedInternalFormat = IsSizedInternalFormat(format, clientVersion) ? format
                                                                             : GetSizedInternalFormat(format, GL_UNSIGNED_BYTE, clientVersion);
    redefineImage(level, sizedInternalFormat, width, height);

    if (!mImageArray[level]->isRenderableFormat())
    {
        mImageArray[level]->copy(0, 0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
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
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        if (level < levelCount())
        {
            updateTextureLevel(level);

            GLuint clientVersion = mRenderer->getCurrentClientVersion();

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect,
                                 gl::GetFormat(mImageArray[0]->getInternalFormat(), clientVersion),
                                 xoffset, yoffset, mTexStorage, level);
        }
    }
}

void Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2D(mRenderer, levels, internalformat, mUsage, false, width, height);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, internalformat, width, height, 1, true);
        width = std::max(1, width >> 1);
        height = std::max(1, height >> 1);
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, GL_NONE, 0, 0, 0, true);
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

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete(const SamplerState &samplerState) const
{
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();

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
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();

    int q = log2(std::max(width, height));

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

    const rx::Image *baseImage = mImageArray[0];
    GLsizei width = baseImage->getWidth();
    GLsizei height = baseImage->getHeight();

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

    if (image->getInternalFormat() != baseImage->getInternalFormat())
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
void Texture2D::createTexture()
{
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();

    if (!(width > 0 && height > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(width, height);
    GLenum internalformat = mImageArray[0]->getInternalFormat();

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2D(mRenderer, levels, internalformat, mUsage, false, width, height);
    
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

void Texture2D::updateTexture()
{
    int levels = (isMipmapComplete() ? levelCount() : 1);

    for (int level = 0; level < levels; level++)
    {
        updateTextureLevel(level);
    }
}

void Texture2D::updateTextureLevel(int level)
{
    ASSERT(level <= (int)ArraySize(mImageArray) && mImageArray[level] != NULL);
    rx::Image *image = mImageArray[level];

    if (image->isDirty())
    {
        commitRect(level, 0, 0, mImageArray[level]->getWidth(), mImageArray[level]->getHeight());
    }
}

void Texture2D::convertToRenderTarget()
{
    rx::TextureStorageInterface2D *newTexStorage = NULL;

    if (mImageArray[0]->getWidth() != 0 && mImageArray[0]->getHeight() != 0)
    {
        GLsizei width = mImageArray[0]->getWidth();
        GLsizei height = mImageArray[0]->getHeight();
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(width, height);
        GLenum internalformat = mImageArray[0]->getInternalFormat();

        newTexStorage = new rx::TextureStorageInterface2D(mRenderer, levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, true, width, height);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {   
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

void Texture2D::generateMipmaps()
{
    if (!mRenderer->getNonPower2TextureSupport())
    {
        if (!isPow2(mImageArray[0]->getWidth()) || !isPow2(mImageArray[0]->getHeight()))
        {
            return gl::error(GL_INVALID_OPERATION);
        }
    }

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    unsigned int q = log2(std::max(mImageArray[0]->getWidth(), mImageArray[0]->getHeight()));
    for (unsigned int i = 1; i <= q; i++)
    {
        redefineImage(i, mImageArray[0]->getInternalFormat(),
                      std::max(mImageArray[0]->getWidth() >> i, 1),
                      std::max(mImageArray[0]->getHeight() >> i, 1));
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            mTexStorage->generateMipmap(i);

            mImageArray[i]->markClean();
        }
    }
    else
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            mRenderer->generateMipmap(mImageArray[i], mImageArray[i - 1]);
        }
    }
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
    if (!mTexStorage || !mTexStorage->isRenderTarget())
    {
        convertToRenderTarget();
    }

    return mTexStorage ? mTexStorage->getRenderTargetSerial(level) : 0;
}

rx::RenderTarget *Texture2D::getRenderTarget(GLint level)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();

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

    updateTexture();

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
            convertToRenderTarget();
        }
        else
        {
            createTexture();
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
        return mImageArray[faceIndex(target)][level]->getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level]->getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level]->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level]->getActualFormat();
    else
        return D3DFMT_UNKNOWN;
}

void TextureCubeMap::setImagePosX(GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(0, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(1, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(2, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(3, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(4, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    setImage(5, level, width, height, internalFormat, format, type, unpack, pixels);
}

void TextureCubeMap::setCompressedImage(GLenum face, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(faceIndex(face), level, format, width, height);

    Texture::setCompressedImage(imageSize, pixels, mImageArray[faceIndex(face)][level]);
}

void TextureCubeMap::commitRect(int face, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    if (level < levelCount())
    {
        rx::Image *image = mImageArray[face][level];
        if (image->updateSurface(mTexStorage, face, level, xoffset, yoffset, width, height))
            image->markClean();
    }
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    if (Texture::subImage(xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels, mImageArray[faceIndex(target)][level]))
    {
        commitRect(faceIndex(target), level, xoffset, yoffset, width, height);
    }
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (Texture::subImageCompressed(xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels, mImageArray[faceIndex(target)][level]))
    {
        commitRect(faceIndex(target), level, xoffset, yoffset, width, height);
    }
}

// Tests for cube map sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 86.
bool TextureCubeMap::isSamplerComplete(const SamplerState &samplerState) const
{
    int size = mImageArray[0][0]->getWidth();

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
    if (mImageArray[0][0]->getWidth() <= 0 || mImageArray[0][0]->getHeight() != mImageArray[0][0]->getWidth())
    {
        return false;
    }

    for (unsigned int face = 1; face < 6; face++)
    {
        if (mImageArray[face][0]->getWidth() != mImageArray[0][0]->getWidth() ||
            mImageArray[face][0]->getWidth() != mImageArray[0][0]->getHeight() ||
            mImageArray[face][0]->getInternalFormat() != mImageArray[0][0]->getInternalFormat())
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

    GLsizei size = mImageArray[0][0]->getWidth();
    int q = log2(size);

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

bool TextureCubeMap::isFaceLevelComplete(int face, int level) const
{
    ASSERT(level >= 0 && face < 6 && level < (int)ArraySize(mImageArray[face]) && mImageArray[face][level] != NULL);

    if (isImmutable())
    {
        return true;
    }

    const rx::Image *baseImage = mImageArray[face][0];
    GLsizei size = baseImage->getWidth();

    if (size <= 0)
    {
        return false;
    }

    // The base image level is complete if the width and height are positive
    if (level == 0)
    {
        return true;
    }

    rx::Image *image = mImageArray[face][level];

    if (image->getInternalFormat() != baseImage->getInternalFormat())
    {
        return false;
    }

    if (mImageArray[face][level]->getWidth() != std::max(1, size >> level))
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
void TextureCubeMap::createTexture()
{
    GLsizei size = mImageArray[0][0]->getWidth();

    if (!(size > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(size);
    GLenum internalformat = mImageArray[0][0]->getInternalFormat();

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, mUsage, false, size);

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int face = 0; face < 6; face++)
        {
            for (int level = 0; level < levels; level++)
            {
                mImageArray[face][level]->setManagedSurface(mTexStorage, face, level);
            }
        }
    }

    mDirtyImages = true;
}

void TextureCubeMap::updateTexture()
{
    int levels = (isMipmapCubeComplete() ? levelCount() : 1);

    for (int face = 0; face < 6; face++)
    {
        for (int level = 0; level < levels; level++)
        {
            updateTextureFaceLevel(face, level);
        }
    }
}

void TextureCubeMap::updateTextureFaceLevel(int face, int level)
{
    ASSERT(level >= 0 && face < 6 && level < (int)ArraySize(mImageArray[face]) && mImageArray[face][level] != NULL);
    rx::Image *image = mImageArray[face][level];

    if (image->isDirty())
    {
        commitRect(face, level, 0, 0, image->getWidth(), image->getHeight());
    }
}

void TextureCubeMap::convertToRenderTarget()
{
    rx::TextureStorageInterfaceCube *newTexStorage = NULL;

    if (mImageArray[0][0]->getWidth() != 0)
    {
        GLsizei size = mImageArray[0][0]->getWidth();
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(size);
        GLenum internalformat = mImageArray[0][0]->getInternalFormat();

        newTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, true, size);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

void TextureCubeMap::setImage(int faceIndex, GLint level, GLsizei width, GLsizei height, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLint sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
                                                                                     : GetSizedInternalFormat(format, type, clientVersion);

    redefineImage(faceIndex, level, sizedInternalFormat, width, height);

    Texture::setImage(unpack, type, pixels, mImageArray[faceIndex][level]);
}

unsigned int TextureCubeMap::faceIndex(GLenum face)
{
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_X - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 1);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 2);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 3);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_POSITIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 4);
    META_ASSERT(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z - GL_TEXTURE_CUBE_MAP_POSITIVE_X == 5);

    return face - GL_TEXTURE_CUBE_MAP_POSITIVE_X;
}

void TextureCubeMap::redefineImage(int face, GLint level, GLint internalformat, GLsizei width, GLsizei height)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, mImageArray[0][0]->getWidth() >> level);
    const int storageHeight = std::max(1, mImageArray[0][0]->getHeight() >> level);
    const int storageFormat = mImageArray[0][0]->getInternalFormat();

    mImageArray[face][level]->redefine(mRenderer, GL_TEXTURE_CUBE_MAP, internalformat, width, height, 1, false);

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
                for (int f = 0; f < 6; f++)
                {
                    mImageArray[f][i]->markDirty();
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
    unsigned int faceindex = faceIndex(target);
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLint sizedInternalFormat = IsSizedInternalFormat(format, clientVersion) ? format
                                                                             : GetSizedInternalFormat(format, GL_UNSIGNED_BYTE, clientVersion);
    redefineImage(faceindex, level, sizedInternalFormat, width, height);

    if (!mImageArray[faceindex][level]->isRenderableFormat())
    {
        mImageArray[faceindex][level]->copy(0, 0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        mImageArray[faceindex][level]->markClean();

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
    GLsizei size = mImageArray[faceIndex(target)][level]->getWidth();

    if (xoffset + width > size || yoffset + height > size || zoffset != 0)
    {
        return gl::error(GL_INVALID_VALUE);
    }

    unsigned int face = faceIndex(target);

    // can only make our texture storage to a render target if level 0 is defined (with a width & height) and
    // the current level we're copying to is defined (with appropriate format, width & height)
    bool canCreateRenderTarget = isFaceLevelComplete(face, level) && isFaceLevelComplete(face, 0);

    if (!mImageArray[face][level]->isRenderableFormat() || (!mTexStorage && !canCreateRenderTarget))
    {
        mImageArray[face][level]->copy(0, 0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        if (level < levelCount())
        {
            updateTextureFaceLevel(face, level);

            GLuint clientVersion = mRenderer->getCurrentClientVersion();

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect, gl::GetFormat(mImageArray[0][0]->getInternalFormat(), clientVersion),
                                 xoffset, yoffset, mTexStorage, target, level);
        }
    }
}

void TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterfaceCube(mRenderer, levels, internalformat, mUsage, false, size);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        for (int face = 0; face < 6; face++)
        {
            mImageArray[face][level]->redefine(mRenderer, GL_TEXTURE_CUBE_MAP, internalformat, size, size, 1, true);
            size = std::max(1, size >> 1);
        }
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        for (int face = 0; face < 6; face++)
        {
            mImageArray[face][level]->redefine(mRenderer, GL_TEXTURE_CUBE_MAP, GL_NONE, 0, 0, 0, true);
        }
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int face = 0; face < 6; face++)
        {
            for (int level = 0; level < levels; level++)
            {
                mImageArray[face][level]->setManagedSurface(mTexStorage, face, level);
            }
        }
    }
}

void TextureCubeMap::generateMipmaps()
{
    if (!isCubeComplete())
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    if (!mRenderer->getNonPower2TextureSupport())
    {
        if (!isPow2(mImageArray[0][0]->getWidth()))
        {
            return gl::error(GL_INVALID_OPERATION);
        }
    }

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    unsigned int q = log2(mImageArray[0][0]->getWidth());
    for (unsigned int f = 0; f < 6; f++)
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            redefineImage(f, i, mImageArray[f][0]->getInternalFormat(),
                          std::max(mImageArray[f][0]->getWidth() >> i, 1),
                          std::max(mImageArray[f][0]->getWidth() >> i, 1));
        }
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (unsigned int f = 0; f < 6; f++)
        {
            for (unsigned int i = 1; i <= q; i++)
            {
                mTexStorage->generateMipmap(f, i);

                mImageArray[f][i]->markClean();
            }
        }
    }
    else
    {
        for (unsigned int f = 0; f < 6; f++)
        {
            for (unsigned int i = 1; i <= q; i++)
            {
                mRenderer->generateMipmap(mImageArray[f][i], mImageArray[f][i - 1]);
            }
        }
    }
}

Renderbuffer *TextureCubeMap::getRenderbuffer(GLenum target, GLint level)
{
    if (!IsCubemapTextureTarget(target))
    {
        return gl::error(GL_INVALID_OPERATION, (Renderbuffer *)NULL);
    }

    unsigned int face = faceIndex(target);

    Renderbuffer *renderBuffer = mRenderbufferProxies.get(level, face);
    if (!renderBuffer)
    {
        renderBuffer = new Renderbuffer(mRenderer, id(), new RenderbufferTextureCubeMap(this, target, level));
        mRenderbufferProxies.add(level, face, renderBuffer);
    }

    return renderBuffer;
}

unsigned int TextureCubeMap::getRenderTargetSerial(GLenum faceTarget, GLint level)
{
    if (!mTexStorage || !mTexStorage->isRenderTarget())
    {
        convertToRenderTarget();
    }

    return mTexStorage ? mTexStorage->getRenderTargetSerial(faceTarget, level) : 0;
}

rx::RenderTarget *TextureCubeMap::getRenderTarget(GLenum target, GLint level)
{
    ASSERT(IsCubemapTextureTarget(target));

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();

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

    updateTexture();

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
            convertToRenderTarget();
        }
        else
        {
            createTexture();
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

void Texture3D::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLint sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
                                                                                     : GetSizedInternalFormat(format, type, clientVersion);
    redefineImage(level, sizedInternalFormat, width, height, depth);

    Texture::setImage(unpack, type, pixels, mImageArray[level]);
}

void Texture3D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height, depth);

    Texture::setCompressedImage(imageSize, pixels, mImageArray[level]);
}

void Texture3D::subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    if (Texture::subImage(xoffset, yoffset, zoffset, width, height, depth, format, type, unpack, pixels, mImageArray[level]))
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
    mTexStorage = new rx::TextureStorageInterface3D(mRenderer, levels, internalformat, mUsage, width, height, depth);
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
    unsigned int q = log2(std::max(mImageArray[0]->getWidth(), mImageArray[0]->getHeight()));
    for (unsigned int i = 1; i <= q; i++)
    {
        redefineImage(i, mImageArray[0]->getInternalFormat(),
                      std::max(mImageArray[0]->getWidth() >> i, 1),
                      std::max(mImageArray[0]->getHeight() >> i, 1),
                      std::max(mImageArray[0]->getDepth() >> i, 1));
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            mTexStorage->generateMipmap(i);

            mImageArray[i]->markClean();
        }
    }
    else
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            mRenderer->generateMipmap(mImageArray[i], mImageArray[i - 1]);
        }
    }
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
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }

        if (level < levelCount())
        {
            updateTextureLevel(level);

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            GLuint clientVersion = mRenderer->getCurrentClientVersion();

            mRenderer->copyImage(source, sourceRect,
                                 gl::GetFormat(mImageArray[0]->getInternalFormat(), clientVersion),
                                 xoffset, yoffset, zoffset, mTexStorage, level);
        }
    }
}

bool Texture3D::isSamplerComplete(const SamplerState &samplerState) const
{
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();
    GLsizei depth = mImageArray[0]->getDepth();

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
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();
    GLsizei depth = mImageArray[0]->getDepth();

    int q = log2(std::max(std::max(width, height), depth));

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

    rx::Image *baseImage = mImageArray[0];

    GLsizei width = baseImage->getWidth();
    GLsizei height = baseImage->getHeight();
    GLsizei depth = baseImage->getDepth();

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    rx::Image *levelImage = mImageArray[level];

    if (levelImage->getInternalFormat() != baseImage->getInternalFormat())
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
    if (!mTexStorage || !mTexStorage->isRenderTarget())
    {
        convertToRenderTarget();
    }

    return mTexStorage ? mTexStorage->getRenderTargetSerial(level, layer) : 0;
}

int Texture3D::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() : 0;
}

void Texture3D::createTexture()
{
    GLsizei width = mImageArray[0]->getWidth();
    GLsizei height = mImageArray[0]->getHeight();
    GLsizei depth = mImageArray[0]->getDepth();

    if (!(width > 0 && height > 0 && depth > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(width, height, depth);
    GLenum internalformat = mImageArray[0]->getInternalFormat();

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface3D(mRenderer, levels, internalformat, mUsage, width, height, depth);

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

void Texture3D::updateTexture()
{
    int levels = (isMipmapComplete() ? levelCount() : 1);

    for (int level = 0; level < levels; level++)
    {
        updateTextureLevel(level);
    }
}

void Texture3D::updateTextureLevel(int level)
{
    ASSERT(level >= 0 && level < (int)ArraySize(mImageArray) && mImageArray[level] != NULL);

    rx::Image *image = mImageArray[level];

    if (image->isDirty())
    {
        commitRect(level, 0, 0, 0, mImageArray[level]->getWidth(), mImageArray[level]->getHeight(), mImageArray[level]->getDepth());
    }
}

void Texture3D::convertToRenderTarget()
{
    rx::TextureStorageInterface3D *newTexStorage = NULL;

    if (mImageArray[0]->getWidth() != 0 && mImageArray[0]->getHeight() != 0 && mImageArray[0]->getDepth() != 0)
    {
        GLsizei width = mImageArray[0]->getWidth();
        GLsizei height = mImageArray[0]->getHeight();
        GLsizei depth = mImageArray[0]->getDepth();
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(width, height, depth);
        GLenum internalformat = mImageArray[0]->getInternalFormat();

        newTexStorage = new rx::TextureStorageInterface3D(mRenderer, levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, width, height, depth);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

rx::RenderTarget *Texture3D::getRenderTarget(GLint level, GLint layer)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();

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

    updateTexture();

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
            convertToRenderTarget();
        }
        else
        {
            createTexture();
        }
    }

    return mTexStorage;
}

void Texture3D::redefineImage(GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, mImageArray[0]->getWidth() >> level);
    const int storageHeight = std::max(1, mImageArray[0]->getHeight() >> level);
    const int storageDepth = std::max(1, mImageArray[0]->getDepth() >> level);
    const int storageFormat = mImageArray[0]->getInternalFormat();

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
        if (image->updateSurface(mTexStorage, level, xoffset, yoffset, zoffset, width, height, depth))
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

void Texture2DArray::setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLint internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels)
{
    GLuint clientVersion = mRenderer->getCurrentClientVersion();
    GLint sizedInternalFormat = IsSizedInternalFormat(internalFormat, clientVersion) ? internalFormat
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
    GLint internalformat = getInternalFormat(level);
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
    mTexStorage = new rx::TextureStorageInterface2DArray(mRenderer, levels, internalformat, mUsage, width, height, depth);
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
    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    int q = log2(std::max(getWidth(0), getHeight(0)));
    for (int i = 1; i <= q; i++)
    {
        redefineImage(i, getInternalFormat(0), std::max(getWidth(0) >> i, 1), std::max(getHeight(0) >> i, 1), getDepth(0));
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
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }

        if (level < levelCount())
        {
            updateTextureLevel(level);

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
    GLsizei width = getWidth(0);
    GLsizei height = getHeight(0);
    GLsizei depth = getDepth(0);

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

bool Texture2DArray::isMipmapComplete() const
{
    GLsizei width = getWidth(0);
    GLsizei height = getHeight(0);

    int q = log2(std::max(width, height));

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

    GLsizei width = getWidth(0);
    GLsizei height = getHeight(0);
    GLsizei depth = getDepth(0);

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

unsigned int Texture2DArray::getRenderTargetSerial( GLint level, GLint layer )
{
    if (!mTexStorage || !mTexStorage->isRenderTarget())
    {
        convertToRenderTarget();
    }

    return mTexStorage ? mTexStorage->getRenderTargetSerial(level, layer) : 0;
}

int Texture2DArray::levelCount()
{
    return mTexStorage ? mTexStorage->levelCount() : 0;
}

void Texture2DArray::createTexture()
{
    GLsizei width = getWidth(0);
    GLsizei height = getHeight(0);
    GLsizei depth = getDepth(0);

    if (width <= 0 || height <= 0 || depth <= 0)
    {
        return; // do not attempt to create native textures for nonexistant data
    }

    GLint levels = creationLevels(width, height);
    GLenum internalformat = getInternalFormat(0);

    delete mTexStorage;
    mTexStorage = new rx::TextureStorageInterface2DArray(mRenderer, levels, internalformat, mUsage, width, height, depth);

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

void Texture2DArray::updateTexture()
{
    int levels = (isMipmapComplete() ? levelCount() : 1);
    for (int level = 0; level < levels; level++)
    {
        updateTextureLevel(level);
    }
}

void Texture2DArray::updateTextureLevel(int level)
{
    for (int layer = 0; layer < mLayerCounts[level]; layer++)
    {
        rx::Image *image = mImageArray[level][layer];

        if (image->isDirty())
        {
            commitRect(level, 0, 0, layer, image->getWidth(), image->getHeight());
        }
    }
}

void Texture2DArray::convertToRenderTarget()
{
    rx::TextureStorageInterface2DArray *newTexStorage = NULL;

    GLsizei width = getWidth(0);
    GLsizei height = getHeight(0);
    GLsizei depth = getDepth(0);

    if (width != 0 && height != 0 && depth != 0)
    {
        GLint levels = mTexStorage != NULL ? mTexStorage->levelCount() : creationLevels(width, height);
        GLenum internalformat = getInternalFormat(0);

        newTexStorage = new rx::TextureStorageInterface2DArray(mRenderer, levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, width, height, depth);

        if (mTexStorage != NULL)
        {
            if (!mRenderer->copyToRenderTarget(newTexStorage, mTexStorage))
            {
                delete newTexStorage;
                return gl::error(GL_OUT_OF_MEMORY);
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

rx::RenderTarget *Texture2DArray::getRenderTarget(GLint level, GLint layer)
{
    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();

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

    updateTexture();

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
            convertToRenderTarget();
        }
        else
        {
            createTexture();
        }
    }

    return mTexStorage;
}

void Texture2DArray::redefineImage(GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, getWidth(0) >> level);
    const int storageHeight = std::max(1, getHeight(0) >> level);
    const int storageDepth = getDepth(0);
    const int storageFormat = getInternalFormat(0);

    for (int layer = 0; layer < mLayerCounts[level]; layer++)
    {
        delete mImageArray[level][layer];
    }
    delete[] mImageArray[level];

    mImageArray[level] = new rx::Image*[depth]();
    mLayerCounts[level] = depth;

    for (int layer = 0; layer < mLayerCounts[level]; layer++)
    {
        mImageArray[level][layer] = mRenderer->createImage();
        mImageArray[level][layer]->redefine(mRenderer, GL_TEXTURE_2D_ARRAY, internalformat, width, height, 1, false);
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
        if (image->updateSurface(mTexStorage, level, xoffset, yoffset, layerTarget, width, height))
        {
            image->markClean();
        }
    }
}

}
