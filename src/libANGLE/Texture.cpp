//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "libANGLE/Texture.h"
#include "libANGLE/Context.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/ImageIndex.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/renderer/Image.h"
#include "libANGLE/renderer/d3d/TextureStorage.h"

#include "libANGLE/Surface.h"

#include "common/mathutil.h"
#include "common/utilities.h"

namespace gl
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

bool IsPointSampled(const gl::SamplerState &samplerState)
{
    return (samplerState.magFilter == GL_NEAREST && (samplerState.minFilter == GL_NEAREST || samplerState.minFilter == GL_NEAREST_MIPMAP_NEAREST));
}

unsigned int Texture::mCurrentTextureSerial = 1;

Texture::Texture(rx::TextureImpl *impl, GLuint id, GLenum target)
    : RefCountObject(id),
      mTexture(impl),
      mTextureSerial(issueTextureSerial()),
      mUsage(GL_NONE),
      mImmutableLevelCount(0),
      mTarget(target),
      mBoundSurface(NULL)
{
}

Texture::~Texture()
{
    if (mBoundSurface)
    {
        mBoundSurface->releaseTexImage(EGL_BACK_BUFFER);
        mBoundSurface = NULL;
    }
    SafeDelete(mTexture);
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

GLenum Texture::getUsage() const
{
    return mUsage;
}

size_t Texture::getWidth(GLenum target, size_t level) const
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));
    return getImageDesc(ImageIdentifier(target, level)).size.width;
}

size_t Texture::getHeight(GLenum target, size_t level) const
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));
    return getImageDesc(ImageIdentifier(target, level)).size.height;
}

size_t Texture::getDepth(GLenum target, size_t level) const
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));
    return getImageDesc(ImageIdentifier(target, level)).size.depth;
}

GLenum Texture::getInternalFormat(GLenum target, size_t level) const
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));
    return getImageDesc(ImageIdentifier(target, level)).internalFormat;
}

unsigned int Texture::getTextureSerial() const
{
    return mTextureSerial;
}

unsigned int Texture::issueTextureSerial()
{
    return mCurrentTextureSerial++;
}

bool Texture::isImmutable() const
{
    return (mImmutableLevelCount > 0);
}

int Texture::immutableLevelCount()
{
    return mImmutableLevelCount;
}

Error Texture::setImage(GLenum target, size_t level, GLenum internalFormat, const Extents &size, GLenum format, GLenum type,
                        const PixelUnpackState &unpack, const uint8_t *pixels)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));

    Error error = mTexture->setImage(target, level, internalFormat, size, format, type, unpack, pixels);
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    setImageDesc(ImageIdentifier(target, level), ImageDesc(size, GetSizedInternalFormat(internalFormat, type)));

    return Error(GL_NO_ERROR);
}

Error Texture::setSubImage(GLenum target, size_t level, const Box &area, GLenum format, GLenum type,
                           const PixelUnpackState &unpack, const uint8_t *pixels)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));

    return mTexture->setSubImage(target, level, area, format, type, unpack, pixels);
}

Error Texture::setCompressedImage(GLenum target, size_t level, GLenum internalFormat, const Extents &size,
                                  const PixelUnpackState &unpack, const uint8_t *pixels)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));

    Error error = mTexture->setCompressedImage(target, level, internalFormat, size, unpack, pixels);
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    setImageDesc(ImageIdentifier(target, level), ImageDesc(size, GetSizedInternalFormat(internalFormat, GL_UNSIGNED_BYTE)));

    return Error(GL_NO_ERROR);
}

Error Texture::setCompressedSubImage(GLenum target, size_t level, const Box &area, GLenum format,
                                     const PixelUnpackState &unpack, const uint8_t *pixels)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));

    return mTexture->setCompressedSubImage(target, level, area, format, unpack, pixels);
}

Error Texture::copyImage(GLenum target, size_t level, const Rectangle &sourceArea, GLenum internalFormat,
                         const Framebuffer *source)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));

    Error error = mTexture->copyImage(target, level, sourceArea, internalFormat, source);
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    setImageDesc(ImageIdentifier(target, level), ImageDesc(Extents(sourceArea.width, sourceArea.height, 1),
                                                                   GetSizedInternalFormat(internalFormat, GL_UNSIGNED_BYTE)));

    return Error(GL_NO_ERROR);
}

Error Texture::copySubImage(GLenum target, size_t level, const Offset &destOffset, const Rectangle &sourceArea,
                            const Framebuffer *source)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubemapTextureTarget(target)));

    return mTexture->copySubImage(target, level, destOffset, sourceArea, source);
}

Error Texture::setStorage(GLenum target, size_t levels, GLenum internalFormat, const Extents &size)
{
    ASSERT(target == mTarget);

    Error error = mTexture->setStorage(target, levels, internalFormat, size);
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    mImmutableLevelCount = levels;
    clearImageDescs();
    setImageDescChain(levels, size, internalFormat);

    return Error(GL_NO_ERROR);
}


Error Texture::generateMipmaps()
{
    Error error = mTexture->generateMipmaps();
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    ImageIdentifier baseLevel(mTarget == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X : mTarget, 0);
    const ImageDesc &baseImageInfo = getImageDesc(baseLevel);
    size_t mipLevels = log2(std::max(std::max(baseImageInfo.size.width, baseImageInfo.size.height), baseImageInfo.size.depth)) + 1;
    setImageDescChain(mipLevels, baseImageInfo.size, baseImageInfo.internalFormat);

    return Error(GL_NO_ERROR);
}

void Texture::setImageDescChain(size_t levels, Extents baseSize, GLenum sizedInternalFormat)
{
    for (size_t level = 0; level < levels; level++)
    {
        Extents levelSize(std::max<size_t>(baseSize.width >> level, 1),
                          std::max<size_t>(baseSize.height >> level, 1),
                          (mTarget == GL_TEXTURE_2D_ARRAY) ? baseSize.depth : std::max<size_t>(baseSize.depth >> level, 1));
        ImageDesc levelInfo(levelSize, sizedInternalFormat);

        if (mTarget == GL_TEXTURE_CUBE_MAP)
        {
            for (size_t face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; face++)
            {
                setImageDesc(ImageIdentifier(face, level), levelInfo);
            }
        }
        else
        {
            setImageDesc(ImageIdentifier(mTarget, level), levelInfo);
        }
    }
}

Texture::ImageIdentifier::ImageIdentifier()
    : ImageIdentifier(GL_TEXTURE_2D, 0)
{
}

Texture::ImageIdentifier::ImageIdentifier(GLenum target, size_t level)
    : target(target),
      level(level)
{
}

bool Texture::ImageIdentifier::operator<(const ImageIdentifier &other) const
{
    return (target != other.target) ? target < other.target : level < other.level;
}

Texture::ImageDesc::ImageDesc()
    : ImageDesc(Extents(0, 0, 0), GL_NONE)
{
}

Texture::ImageDesc::ImageDesc(const Extents &size, GLenum internalFormat)
    : size(size),
      internalFormat(internalFormat)
{
}

const Texture::ImageDesc &Texture::getImageDesc(const ImageIdentifier& index) const
{
    static const Texture::ImageDesc defaultDesc;
    ImageDescMap::const_iterator iter = mImageDescs.find(index);
    return (iter != mImageDescs.end()) ? iter->second : defaultDesc;
}

void Texture::setImageDesc(const ImageIdentifier& index, const ImageDesc &desc)
{
    mImageDescs[index] = desc;
}

void Texture::clearImageDescs()
{
    mImageDescs.clear();
}

void Texture::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();
    mTexture->bindTexImage(surface);
    mBoundSurface = surface;
}

void Texture::releaseTexImage()
{
    if (mBoundSurface)
    {
        mBoundSurface = NULL;
        mTexture->releaseTexImage();
    }
}

Texture2D::Texture2D(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_2D)
{
}

Texture2D::~Texture2D()
{
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete(const SamplerState &samplerState, const Data &data) const
{
    size_t width = getWidth(GL_TEXTURE_2D, 0);
    size_t height = getHeight(GL_TEXTURE_2D, 0);
    if (width == 0 || height == 0)
    {
        return false;
    }

    GLenum internalFormat = getInternalFormat(GL_TEXTURE_2D, 0);
    if (!data.textureCaps->get(internalFormat).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    bool npotSupport = data.extensions->textureNPOT;
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
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);
    if (formatInfo.depthBits > 0 && data.clientVersion > 2)
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

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isMipmapComplete() const
{
    size_t width = getWidth(GL_TEXTURE_2D, 0);
    size_t height = getHeight(GL_TEXTURE_2D, 0);

    size_t expectedMipLevels = log2(std::max(width, height)) + 1;
    for (size_t level = 0; level < expectedMipLevels; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2D::isLevelComplete(size_t level) const
{
    ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (isImmutable())
    {
        return true;
    }

    size_t width = getWidth(GL_TEXTURE_2D, 0);
    size_t height = getHeight(GL_TEXTURE_2D, 0);
    if (width == 0 || height == 0)
    {
        return false;
    }

    // The base image level is complete if the width and height are positive
    if (level == 0)
    {
        return true;
    }

    if (getInternalFormat(GL_TEXTURE_2D, level) != getInternalFormat(GL_TEXTURE_2D, 0))
    {
        return false;
    }

    if (getWidth(GL_TEXTURE_2D, level) != std::max<size_t>(1, width >> level))
    {
        return false;
    }

    if (getHeight(GL_TEXTURE_2D, level) != std::max<size_t>(1, height >> level))
    {
        return false;
    }

    return true;
}

TextureCubeMap::TextureCubeMap(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_CUBE_MAP)
{
}

TextureCubeMap::~TextureCubeMap()
{
}

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isCubeComplete() const
{
    size_t width = getWidth(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
    size_t height = getWidth(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
    if (width == 0 || width != height)
    {
        return false;
    }

    GLenum internalFormat = getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
    for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X + 1; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; face++)
    {
        if (getWidth(face, 0)          != width  ||
            getHeight(face, 0)         != height ||
            getInternalFormat(face, 0) != internalFormat)
        {
            return false;
        }
    }

    return true;
}

// Tests for texture sampling completeness
bool TextureCubeMap::isSamplerComplete(const SamplerState &samplerState, const Data &data) const
{
    bool mipmapping = IsMipmapFiltered(samplerState);

    GLenum internalFormat = getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
    if (!data.textureCaps->get(internalFormat).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    size_t size = getWidth(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0);
    if (!gl::isPow2(size) && !data.extensions->textureNPOT)
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
        if (!isMipmapComplete())   // Also tests for isCubeComplete()
        {
            return false;
        }
    }

    return true;
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

bool TextureCubeMap::isMipmapComplete() const
{
    if (isImmutable())
    {
        return true;
    }

    if (!isCubeComplete())
    {
        return false;
    }

    size_t width = getWidth(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0);
    size_t height = getHeight(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0);

    size_t expectedMipLevels = log2(std::max(width, height)) + 1;
    for (GLenum face = GL_TEXTURE_CUBE_MAP_NEGATIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; face++)
    {
        for (size_t level = 1; level < expectedMipLevels; level++)
        {
            if (!isFaceLevelComplete(face, level))
            {
                return false;
            }
        }
    }

    return true;
}

bool TextureCubeMap::isFaceLevelComplete(GLenum target, size_t level) const
{
    ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS && IsCubemapTextureTarget(target));

    if (isImmutable())
    {
        return true;
    }

    size_t baseSize = getWidth(target, 0);
    if (baseSize == 0)
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
    if (getInternalFormat(target, level) != getInternalFormat(target, 0))
    {
        return false;
    }

    if (getWidth(target, level) != std::max<size_t>(1, baseSize >> level))
    {
        return false;
    }

    return true;
}


Texture3D::Texture3D(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_3D)
{
}

Texture3D::~Texture3D()
{
}

bool Texture3D::isSamplerComplete(const SamplerState &samplerState, const Data &data) const
{
    size_t width = getWidth(GL_TEXTURE_3D, 0);
    size_t height = getHeight(GL_TEXTURE_3D, 0);
    size_t depth = getDepth(GL_TEXTURE_3D, 0);
    if (width == 0 || height == 0 || depth == 0)
    {
        return false;
    }

    GLenum internalFormat = getInternalFormat(GL_TEXTURE_3D, 0);
    if (!data.textureCaps->get(internalFormat).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (IsMipmapFiltered(samplerState) && !isMipmapComplete())
    {
        return false;
    }

    return true;
}

bool Texture3D::isMipmapComplete() const
{
    size_t width = getWidth(GL_TEXTURE_2D_ARRAY, 0);
    size_t height = getHeight(GL_TEXTURE_2D_ARRAY, 0);
    size_t depth = getDepth(GL_TEXTURE_3D, 0);

    size_t expectedMipLevels = log2(std::max(std::max(width, height), depth)) + 1;
    for (size_t level = 0; level < expectedMipLevels; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture3D::isLevelComplete(size_t level) const
{
    ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (isImmutable())
    {
        return true;
    }

    size_t width = getWidth(GL_TEXTURE_3D, level);
    size_t height = getHeight(GL_TEXTURE_3D, level);
    size_t depth = getDepth(GL_TEXTURE_3D, level);
    if (width == 0 || height == 0 || depth == 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    if (getInternalFormat(GL_TEXTURE_3D, level) != getInternalFormat(GL_TEXTURE_3D, 0))
    {
        return false;
    }

    if (getWidth(GL_TEXTURE_3D, level) != std::max<size_t>(1, width >> level))
    {
        return false;
    }

    if (getHeight(GL_TEXTURE_3D, level) != std::max<size_t>(1, height >> level))
    {
        return false;
    }

    if (getDepth(GL_TEXTURE_3D, level) != std::max<size_t>(1, depth >> level))
    {
        return false;
    }

    return true;
}

Texture2DArray::Texture2DArray(rx::TextureImpl *impl, GLuint id)
    : Texture(impl, id, GL_TEXTURE_2D_ARRAY)
{
}

Texture2DArray::~Texture2DArray()
{
}

bool Texture2DArray::isSamplerComplete(const SamplerState &samplerState, const Data &data) const
{
    size_t width = getWidth(GL_TEXTURE_2D_ARRAY, 0);
    size_t height = getHeight(GL_TEXTURE_2D_ARRAY, 0);
    size_t depth = getDepth(GL_TEXTURE_2D_ARRAY, 0);
    if (width == 0 || height == 0 || depth == 0)
    {
        return false;
    }

    GLenum internalFormat = getInternalFormat(GL_TEXTURE_2D_ARRAY, 0);
    if (!data.textureCaps->get(internalFormat).filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    if (IsMipmapFiltered(samplerState) && !isMipmapComplete())
    {
        return false;
    }

    return true;
}

bool Texture2DArray::isMipmapComplete() const
{
    size_t width = getWidth(GL_TEXTURE_2D_ARRAY, 0);
    size_t height = getHeight(GL_TEXTURE_2D_ARRAY, 0);

    size_t expectedMipLevels = log2(std::max(width, height)) + 1;
    for (size_t level = 1; level < expectedMipLevels; level++)
    {
        if (!isLevelComplete(level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2DArray::isLevelComplete(size_t level) const
{
    ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (isImmutable())
    {
        return true;
    }

    size_t width = getWidth(GL_TEXTURE_2D_ARRAY, level);
    size_t height = getHeight(GL_TEXTURE_2D_ARRAY, level);
    size_t layers = getDepth(GL_TEXTURE_2D_ARRAY, 0);
    if (width == 0 || height == 0 || layers == 0)
    {
        return false;
    }

    if (level == 0)
    {
        return true;
    }

    if (getInternalFormat(GL_TEXTURE_2D_ARRAY, level) != getInternalFormat(GL_TEXTURE_2D_ARRAY, 0))
    {
        return false;
    }

    if (getWidth(GL_TEXTURE_2D_ARRAY, level) != std::max<size_t>(1, width >> level))
    {
        return false;
    }

    if (getHeight(GL_TEXTURE_2D_ARRAY, level) != std::max<size_t>(1, height >> level))
    {
        return false;
    }

    if (getDepth(GL_TEXTURE_2D_ARRAY, level) != layers)
    {
        return false;
    }

    return true;
}

}
