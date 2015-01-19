//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "libANGLE/Texture.h"
#include "libANGLE/Data.h"
#include "libANGLE/formatutils.h"

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
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(ImageIndex::MakeGeneric(target, level)).size.width;
}

size_t Texture::getHeight(GLenum target, size_t level) const
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(ImageIndex::MakeGeneric(target, level)).size.height;
}

size_t Texture::getDepth(GLenum target, size_t level) const
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(ImageIndex::MakeGeneric(target, level)).size.depth;
}

GLenum Texture::getInternalFormat(GLenum target, size_t level) const
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(ImageIndex::MakeGeneric(target, level)).internalFormat;
}

bool Texture::isSamplerComplete(const SamplerState &samplerState, const Data &data) const
{
    GLenum baseTarget = getBaseImageTarget();
    size_t width = getWidth(baseTarget, samplerState.baseLevel);
    size_t height = getHeight(baseTarget, samplerState.baseLevel);
    size_t depth = getDepth(baseTarget, samplerState.baseLevel);
    if (width == 0 || height == 0 || depth == 0)
    {
        return false;
    }

    if (mTarget == GL_TEXTURE_CUBE_MAP && width != height)
    {
        return false;
    }

    GLenum internalFormat = getInternalFormat(baseTarget, samplerState.baseLevel);
    const TextureCaps &textureCaps = data.textureCaps->get(internalFormat);
    if (!textureCaps.filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    bool npotSupport = data.extensions->textureNPOT || data.clientVersion >= 3;
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

        if (!isMipmapComplete(samplerState))
        {
            return false;
        }
    }
    else
    {
        if (mTarget == GL_TEXTURE_CUBE_MAP && !isCubeComplete())
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

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture::isCubeComplete() const
{
    ASSERT(mTarget == GL_TEXTURE_CUBE_MAP);

    GLenum baseTarget = FirstCubeMapTextureTarget;
    size_t width = getWidth(baseTarget, 0);
    size_t height = getWidth(baseTarget, 0);
    if (width == 0 || width != height)
    {
        return false;
    }

    GLenum internalFormat = getInternalFormat(baseTarget, 0);
    for (GLenum face = baseTarget + 1; face <= LastCubeMapTextureTarget; face++)
    {
        if (getWidth(face, 0) != width ||
            getHeight(face, 0) != height ||
            getInternalFormat(face, 0) != internalFormat)
        {
            return false;
        }
    }

    return true;
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
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    Error error = mTexture->setImage(target, level, internalFormat, size, format, type, unpack, pixels);
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    setImageDesc(ImageIndex::MakeGeneric(target, level), ImageDesc(size, GetSizedInternalFormat(internalFormat, type)));

    return Error(GL_NO_ERROR);
}

Error Texture::setSubImage(GLenum target, size_t level, const Box &area, GLenum format, GLenum type,
                           const PixelUnpackState &unpack, const uint8_t *pixels)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    return mTexture->setSubImage(target, level, area, format, type, unpack, pixels);
}

Error Texture::setCompressedImage(GLenum target, size_t level, GLenum internalFormat, const Extents &size,
                                  const PixelUnpackState &unpack, const uint8_t *pixels)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    Error error = mTexture->setCompressedImage(target, level, internalFormat, size, unpack, pixels);
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    setImageDesc(ImageIndex::MakeGeneric(target, level), ImageDesc(size, GetSizedInternalFormat(internalFormat, GL_UNSIGNED_BYTE)));

    return Error(GL_NO_ERROR);
}

Error Texture::setCompressedSubImage(GLenum target, size_t level, const Box &area, GLenum format,
                                     const PixelUnpackState &unpack, const uint8_t *pixels)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    return mTexture->setCompressedSubImage(target, level, area, format, unpack, pixels);
}

Error Texture::copyImage(GLenum target, size_t level, const Rectangle &sourceArea, GLenum internalFormat,
                         const Framebuffer *source)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    Error error = mTexture->copyImage(target, level, sourceArea, internalFormat, source);
    if (error.isError())
    {
        return error;
    }

    releaseTexImage();

    setImageDesc(ImageIndex::MakeGeneric(target, level), ImageDesc(Extents(sourceArea.width, sourceArea.height, 1),
                                                                   GetSizedInternalFormat(internalFormat, GL_UNSIGNED_BYTE)));

    return Error(GL_NO_ERROR);
}

Error Texture::copySubImage(GLenum target, size_t level, const Offset &destOffset, const Rectangle &sourceArea,
                            const Framebuffer *source)
{
    ASSERT(target == mTarget || (mTarget == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

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

    ImageIndex baseLevel = ImageIndex::MakeGeneric(getBaseImageTarget(), 0);
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
            for (size_t face = FirstCubeMapTextureTarget; face <= LastCubeMapTextureTarget; face++)
            {
                setImageDesc(ImageIndex::MakeGeneric(face, level), levelInfo);
            }
        }
        else
        {
            setImageDesc(ImageIndex::MakeGeneric(mTarget, level), levelInfo);
        }
    }
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

const Texture::ImageDesc &Texture::getImageDesc(const ImageIndex &index) const
{
    static const Texture::ImageDesc defaultDesc;
    ImageDescMap::const_iterator iter = mImageDescs.find(index);
    return (iter != mImageDescs.end()) ? iter->second : defaultDesc;
}

void Texture::setImageDesc(const ImageIndex &index, const ImageDesc &desc)
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

GLenum Texture::getBaseImageTarget() const
{
    return mTarget == GL_TEXTURE_CUBE_MAP ? FirstCubeMapTextureTarget : mTarget;
}

size_t Texture::getExpectedMipLevels() const
{
    GLenum baseTarget = getBaseImageTarget();
    size_t width = getWidth(baseTarget, 0);
    size_t height = getHeight(baseTarget, 0);
    if (mTarget == GL_TEXTURE_3D)
    {
        size_t depth = getDepth(baseTarget, 0);
        return log2(std::max(std::max(width, height), depth)) + 1;
    }
    else
    {
        return log2(std::max(width, height)) + 1;
    }
}

bool Texture::isMipmapComplete(const gl::SamplerState &samplerState) const
{
    size_t expectedMipLevels = getExpectedMipLevels();

    size_t maxLevel = std::min<size_t>(expectedMipLevels, samplerState.maxLevel + 1);

    for (size_t level = samplerState.baseLevel; level < maxLevel; level++)
    {
        if (mTarget == GL_TEXTURE_CUBE_MAP)
        {
            for (GLenum face = FirstCubeMapTextureTarget; face <= LastCubeMapTextureTarget; face++)
            {
                if (!isLevelComplete(face, level, samplerState))
                {
                    return false;
                }
            }
        }
        else
        {
            if (!isLevelComplete(mTarget, level, samplerState))
            {
                return false;
            }
        }
    }

    return true;
}


bool Texture::isLevelComplete(GLenum target, size_t level,
                              const gl::SamplerState &samplerState) const
{
    ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (isImmutable())
    {
        return true;
    }

    size_t width = getWidth(target, samplerState.baseLevel);
    size_t height = getHeight(target, samplerState.baseLevel);
    size_t depth = getHeight(target, samplerState.baseLevel);
    if (width == 0 || height == 0 || depth == 0)
    {
        return false;
    }

    // The base image level is complete if the width and height are positive
    if (level == 0)
    {
        return true;
    }

    if (getInternalFormat(target, level) != getInternalFormat(target, samplerState.baseLevel))
    {
        return false;
    }

    if (getWidth(target, level) != std::max<size_t>(1, width >> level))
    {
        return false;
    }

    if (getHeight(target, level) != std::max<size_t>(1, height >> level))
    {
        return false;
    }

    if (mTarget == GL_TEXTURE_3D)
    {
        if (getDepth(target, level) != std::max<size_t>(1, depth >> level))
        {
            return false;
        }
    }
    else if (mTarget == GL_TEXTURE_2D_ARRAY)
    {
        if (getDepth(target, level) != depth)
        {
            return false;
        }
    }

    return true;
}

}
