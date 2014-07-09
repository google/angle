#include "precompiled.h"
//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureD3D.cpp: Implementations of the Texture interfaces shared betweeen the D3D backends.

#include "common/mathutil.h"
#include "libEGL/Surface.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/main.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/renderer/BufferImpl.h"
#include "libGLESv2/renderer/RenderTarget.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/d3d/ImageD3D.h"
#include "libGLESv2/renderer/d3d/TextureD3D.h"
#include "libGLESv2/renderer/d3d/TextureStorage.h"

namespace rx
{

bool IsMipmapFiltered(const gl::SamplerState &samplerState)
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

TextureD3D::TextureD3D(rx::Renderer *renderer)
    : mRenderer(renderer),
      mUsage(GL_NONE),
      mDirtyImages(true),
      mImmutable(false)
{
}

TextureD3D::~TextureD3D()
{
}

GLint TextureD3D::getBaseLevelWidth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getWidth() : 0);
}

GLint TextureD3D::getBaseLevelHeight() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getHeight() : 0);
}

GLint TextureD3D::getBaseLevelDepth() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getDepth() : 0);
}

// Note: "base level image" is loosely defined to be any image from the base level,
// where in the base of 2D array textures and cube maps there are several. Don't use
// the base level image for anything except querying texture format and size.
GLenum TextureD3D::getBaseLevelInternalFormat() const
{
    const rx::Image *baseImage = getBaseLevelImage();
    return (baseImage ? baseImage->getInternalFormat() : GL_NONE);
}

void TextureD3D::setImage(const gl::PixelUnpackState &unpack, GLenum type, const void *pixels, rx::Image *image)
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
        gl::Buffer *pixelBuffer = unpack.pixelBuffer.get();
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

bool TextureD3D::subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels, rx::Image *image)
{
    const void *pixelData = pixels;

    // CPU readback & copy where direct GPU copy is not supported
    if (unpack.pixelBuffer.id() != 0)
    {
        gl::Buffer *pixelBuffer = unpack.pixelBuffer.get();
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

void TextureD3D::setCompressedImage(GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(0, 0, 0, image->getWidth(), image->getHeight(), image->getDepth(), pixels);
        mDirtyImages = true;
    }
}

bool TextureD3D::subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                 GLenum format, GLsizei imageSize, const void *pixels, rx::Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(xoffset, yoffset, zoffset, width, height, depth, pixels);
        mDirtyImages = true;
    }

    return true;
}

bool TextureD3D::isFastUnpackable(const gl::PixelUnpackState &unpack, GLenum sizedInternalFormat)
{
    return unpack.pixelBuffer.id() != 0 && mRenderer->supportsFastCopyBufferToTexture(sizedInternalFormat);
}

bool TextureD3D::fastUnpackPixels(const gl::PixelUnpackState &unpack, const void *pixels, const gl::Box &destArea,
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

GLint TextureD3D::creationLevels(GLsizei width, GLsizei height, GLsizei depth) const
{
    if ((gl::isPow2(width) && gl::isPow2(height) && gl::isPow2(depth)) || mRenderer->getRendererExtensions().textureNPOT)
    {
        // Maximum number of levels
        return gl::log2(std::max(std::max(width, height), depth)) + 1;
    }
    else
    {
        // OpenGL ES 2.0 without GL_OES_texture_npot does not permit NPOT mipmaps.
        return 1;
    }
}

int TextureD3D::mipLevels() const
{
    return gl::log2(std::max(std::max(getBaseLevelWidth(), getBaseLevelHeight()), getBaseLevelDepth())) + 1;
}


TextureD3D_2D::TextureD3D_2D(rx::Renderer *renderer)
    : TextureD3D(renderer),
      Texture2DImpl(),
      mTexStorage(NULL)
{
    for (int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        mImageArray[i] = ImageD3D::makeImageD3D(renderer->createImage());
    }
}

TextureD3D_2D::~TextureD3D_2D()
{
    SafeDelete(mTexStorage);

    for (int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; ++i)
    {
        delete mImageArray[i];
    }
}

TextureD3D_2D *TextureD3D_2D::makeTextureD3D_2D(Texture2DImpl *texture)
{
    ASSERT(HAS_DYNAMIC_TYPE(TextureD3D_2D*, texture));
    return static_cast<TextureD3D_2D*>(texture);
}

TextureStorageInterface *TextureD3D_2D::getNativeTexture()
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

Image *TextureD3D_2D::getImage(int level) const
{
    ASSERT(level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);
    return mImageArray[level];
}

void TextureD3D_2D::setUsage(GLenum usage)
{
    mUsage = usage;
}

void TextureD3D_2D::resetDirty()
{
    mDirtyImages = false;
}

GLsizei TextureD3D_2D::getWidth(GLint level) const
{
    if (level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getWidth();
    else
        return 0;
}

GLsizei TextureD3D_2D::getHeight(GLint level) const
{
    if (level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getHeight();
    else
        return 0;
}

GLenum TextureD3D_2D::getInternalFormat(GLint level) const
{
    if (level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureD3D_2D::getActualFormat(GLint level) const
{
    if (level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level]->getActualFormat();
    else
        return GL_NONE;
}

bool TextureD3D_2D::isDepth(GLint level) const
{
    return gl::GetDepthBits(getInternalFormat(level)) > 0;
}

void TextureD3D_2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels)
{
    GLenum sizedInternalFormat = gl::IsSizedInternalFormat(internalFormat) ? internalFormat
                                                                       : gl::GetSizedInternalFormat(format, type);
    bool fastUnpacked = false;

    // Attempt a fast gpu copy of the pixel data to the surface
    if (isFastUnpackable(unpack, sizedInternalFormat) && isLevelComplete(level))
    {
        // Will try to create RT storage if it does not exist
        rx::RenderTarget *destRenderTarget = getRenderTarget(level);
        gl::Box destArea(0, 0, 0, getWidth(level), getHeight(level), 1);

        if (destRenderTarget && fastUnpackPixels(unpack, pixels, destArea, sizedInternalFormat, type, destRenderTarget))
        {
            // Ensure we don't overwrite our newly initialized data
            mImageArray[level]->markClean();

            fastUnpacked = true;
        }
    }

    if (!fastUnpacked)
    {
        TextureD3D::setImage(unpack, type, pixels, mImageArray[level]);
    }
}

void TextureD3D_2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    TextureD3D::setCompressedImage(imageSize, pixels, mImageArray[level]);
}

void TextureD3D_2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels)
{
    bool fastUnpacked = false;

    if (isFastUnpackable(unpack, getInternalFormat(level)) && isLevelComplete(level))
    {
        rx::RenderTarget *renderTarget = getRenderTarget(level);
        gl::Box destArea(xoffset, yoffset, 0, width, height, 1);

        if (renderTarget && fastUnpackPixels(unpack, pixels, destArea, getInternalFormat(level), type, renderTarget))
        {
            // Ensure we don't overwrite our newly initialized data
            mImageArray[level]->markClean();

            fastUnpacked = true;
        }
    }

    if (!fastUnpacked && TextureD3D::subImage(xoffset, yoffset, 0, width, height, 1, format, type, unpack, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void TextureD3D_2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (TextureD3D::subImageCompressed(xoffset, yoffset, 0, width, height, 1, format, imageSize, pixels, mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void TextureD3D_2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source)
{
    if (!mImageArray[level]->isRenderableFormat())
    {
        mImageArray[level]->copy(0, 0, 0, x, y, width, height, source);
        mDirtyImages = true;
    }
    else
    {
        ensureRenderTarget();
        mImageArray[level]->markClean();

        if (width != 0 && height != 0 && isValidLevel(level))
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

void TextureD3D_2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source)
{
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

        if (isValidLevel(level))
        {
            updateStorageLevel(level);

            gl::Rectangle sourceRect;
            sourceRect.x = x;
            sourceRect.width = width;
            sourceRect.y = y;
            sourceRect.height = height;

            mRenderer->copyImage(source, sourceRect,
                                 gl::GetFormat(getBaseLevelInternalFormat()),
                                 xoffset, yoffset, mTexStorage, level);
        }
    }
}

void TextureD3D_2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    for (int level = 0; level < levels; level++)
    {
        GLsizei levelWidth = std::max(1, width >> level);
        GLsizei levelHeight = std::max(1, height >> level);
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, internalformat, levelWidth, levelHeight, 1, true);
    }

    for (int level = levels; level < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, GL_NONE, 0, 0, 0, true);
    }

    mImmutable = true;

    setCompleteTexStorage(new rx::TextureStorageInterface2D(mRenderer, internalformat, IsRenderTargetUsage(mUsage), width, height, levels));
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool TextureD3D_2D::isSamplerComplete(const gl::SamplerState &samplerState) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    if (!mRenderer->getRendererTextureCaps().get(getInternalFormat(0)).filtering)
    {
        if (samplerState.magFilter != GL_NEAREST ||
            (samplerState.minFilter != GL_NEAREST && samplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    // TODO(geofflang): use context's extensions
    bool npotSupport = mRenderer->getRendererExtensions().textureNPOT;

    if (!npotSupport)
    {
        if ((samplerState.wrapS != GL_CLAMP_TO_EDGE && !gl::isPow2(width)) ||
            (samplerState.wrapT != GL_CLAMP_TO_EDGE && !gl::isPow2(height)))
        {
            return false;
        }
    }

    if (IsMipmapFiltered(samplerState))
    {
        if (!npotSupport)
        {
            if (!gl::isPow2(width) || !gl::isPow2(height))
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
    if (gl::GetDepthBits(getInternalFormat(0)) > 0 && mRenderer->getCurrentClientVersion() > 2)
    {
        if (samplerState.compareMode == GL_NONE)
        {
            if ((samplerState.minFilter != GL_NEAREST && samplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST) ||
                samplerState.magFilter != GL_NEAREST)
            {
                return false;
            }
        }
    }

    return true;
}

void TextureD3D_2D::bindTexImage(egl::Surface *surface)
{
    GLenum internalformat = surface->getFormat();

    mImageArray[0]->redefine(mRenderer, GL_TEXTURE_2D, internalformat, surface->getWidth(), surface->getHeight(), 1, true);

    if (mTexStorage)
    {
        SafeDelete(mTexStorage);
    }
    mTexStorage = new rx::TextureStorageInterface2D(mRenderer, surface->getSwapChain());

    mDirtyImages = true;
}

void TextureD3D_2D::releaseTexImage()
{
    if (mTexStorage)
    {
        SafeDelete(mTexStorage);
    }

    for (int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
    {
        mImageArray[i]->redefine(mRenderer, GL_TEXTURE_2D, GL_NONE, 0, 0, 0, true);
    }
}

void TextureD3D_2D::generateMipmaps()
{
    int levelCount = mipLevels();

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (int level = 1; level < levelCount; level++)
        {
            mTexStorage->generateMipmap(level);

            mImageArray[level]->markClean();
        }
    }
    else
    {
        for (int level = 1; level < levelCount; level++)
        {
            mRenderer->generateMipmap(mImageArray[level], mImageArray[level - 1]);
        }
    }
}

unsigned int TextureD3D_2D::getRenderTargetSerial(GLint level)
{
    return (ensureRenderTarget() ? mTexStorage->getRenderTargetSerial(level) : 0);
}

RenderTarget *TextureD3D_2D::getRenderTarget(GLint level)
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

    return mTexStorage->getRenderTarget(level);
}

RenderTarget *TextureD3D_2D::getDepthSencil(GLint level)
{
    // ensure the underlying texture is created
    if (!ensureRenderTarget())
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

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureD3D_2D::isMipmapComplete() const
{
    int levelCount = mipLevels();

    for (int level = 0; level < levelCount; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool TextureD3D_2D::isValidLevel(int level) const
{
    return (mTexStorage ? (level >= 0 && level < mTexStorage->getLevelCount()) : false);
}

bool TextureD3D_2D::isLevelComplete(int level) const
{
    if (isImmutable())
    {
        return true;
    }

    const rx::Image *baseImage = getBaseLevelImage();

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

// Constructs a native texture resource from the texture images
void TextureD3D_2D::initializeStorage(bool renderTarget)
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

rx::TextureStorageInterface2D *TextureD3D_2D::createCompleteStorage(bool renderTarget) const
{
    GLsizei width = getBaseLevelWidth();
    GLsizei height = getBaseLevelHeight();

    ASSERT(width > 0 && height > 0);

    // use existing storage level count, when previously specified by TexStorage*D
    GLint levels = (mTexStorage ? mTexStorage->getLevelCount() : creationLevels(width, height, 1));

    return new rx::TextureStorageInterface2D(mRenderer, getBaseLevelInternalFormat(), renderTarget, width, height, levels);
}

void TextureD3D_2D::setCompleteTexStorage(TextureStorageInterface2D *newCompleteTexStorage)
{
    SafeDelete(mTexStorage);
    mTexStorage = newCompleteTexStorage;

    if (mTexStorage && mTexStorage->isManaged())
    {
        for (int level = 0; level < mTexStorage->getLevelCount(); level++)
        {
            mImageArray[level]->setManagedSurface(mTexStorage, level);
        }
    }

    mDirtyImages = true;
}

void TextureD3D_2D::updateStorage()
{
    ASSERT(mTexStorage != NULL);
    GLint storageLevels = mTexStorage->getLevelCount();
    for (int level = 0; level < storageLevels; level++)
    {
        if (mImageArray[level]->isDirty() && isLevelComplete(level))
        {
            updateStorageLevel(level);
        }
    }
}

bool TextureD3D_2D::ensureRenderTarget()
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

TextureStorageInterface *TextureD3D_2D::getBaseLevelStorage()
{
    return mTexStorage;
}

const ImageD3D *TextureD3D_2D::getBaseLevelImage() const
{
    return mImageArray[0];
}

void TextureD3D_2D::updateStorageLevel(int level)
{
    ASSERT(level <= (int)ArraySize(mImageArray) && mImageArray[level] != NULL);
    ASSERT(isLevelComplete(level));

    if (mImageArray[level]->isDirty())
    {
        commitRect(level, 0, 0, getWidth(level), getHeight(level));
    }
}

void TextureD3D_2D::redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height)
{
    // If there currently is a corresponding storage texture image, it has these parameters
    const int storageWidth = std::max(1, getBaseLevelWidth() >> level);
    const int storageHeight = std::max(1, getBaseLevelHeight() >> level);
    const GLenum storageFormat = getBaseLevelInternalFormat();

    mImageArray[level]->redefine(mRenderer, GL_TEXTURE_2D, internalformat, width, height, 1, false);

    if (mTexStorage)
    {
        const int storageLevels = mTexStorage->getLevelCount();

        if ((level >= storageLevels && storageLevels != 0) ||
            width != storageWidth ||
            height != storageHeight ||
            internalformat != storageFormat)   // Discard mismatched storage
        {
            for (int i = 0; i < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
            {
                mImageArray[i]->markDirty();
            }

            SafeDelete(mTexStorage);
            mDirtyImages = true;
        }
    }
}

void TextureD3D_2D::commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    if (isValidLevel(level))
    {
        rx::ImageD3D *image = mImageArray[level];
        if (image->copyToStorage(mTexStorage, level, xoffset, yoffset, width, height))
        {
            image->markClean();
        }
    }
}

}