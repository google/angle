//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "libANGLE/Texture.h"

#include "common/mathutil.h"
#include "common/utilities.h"
#include "libANGLE/Config.h"
#include "libANGLE/Context.h"
#include "libANGLE/ContextState.h"
#include "libANGLE/Image.h"
#include "libANGLE/Surface.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/GLImplFactory.h"
#include "libANGLE/renderer/TextureImpl.h"

namespace gl
{

namespace
{
bool IsPointSampled(const gl::SamplerState &samplerState)
{
    return (samplerState.magFilter == GL_NEAREST &&
            (samplerState.minFilter == GL_NEAREST ||
             samplerState.minFilter == GL_NEAREST_MIPMAP_NEAREST));
}

size_t GetImageDescIndex(GLenum target, size_t level)
{
    return IsCubeMapTextureTarget(target) ? ((level * 6) + CubeMapTextureTargetToLayerIndex(target))
                                          : level;
}
}  // namespace

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

TextureState::TextureState(GLenum target)
    : target(target),
      swizzleRed(GL_RED),
      swizzleGreen(GL_GREEN),
      swizzleBlue(GL_BLUE),
      swizzleAlpha(GL_ALPHA),
      samplerState(),
      baseLevel(0),
      maxLevel(1000),
      immutableFormat(false),
      immutableLevels(0),
      usage(GL_NONE)
{
}

bool TextureState::swizzleRequired() const
{
    return swizzleRed != GL_RED || swizzleGreen != GL_GREEN || swizzleBlue != GL_BLUE ||
           swizzleAlpha != GL_ALPHA;
}

GLuint TextureState::getEffectiveBaseLevel() const
{
    if (immutableFormat)
    {
        return std::min(baseLevel, immutableLevels - 1);
    }
    // Some classes use the effective base level to index arrays with level data. By clamping the
    // effective base level to max levels these arrays need just one extra item to store properties
    // that should be returned for all out-of-range base level values, instead of needing special
    // handling for out-of-range base levels.
    return std::min(baseLevel, static_cast<GLuint>(gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS));
}

Texture::Texture(rx::GLImplFactory *factory, GLuint id, GLenum target)
    : egl::ImageSibling(id),
      mState(target),
      mTexture(factory->createTexture(mState)),
      mLabel(),
      mImageDescs((IMPLEMENTATION_MAX_TEXTURE_LEVELS + 1) *
                  (target == GL_TEXTURE_CUBE_MAP ? 6 : 1)),
      mCompletenessCache(),
      mBoundSurface(nullptr),
      mBoundStream(nullptr)
{
}

Texture::~Texture()
{
    if (mBoundSurface)
    {
        mBoundSurface->releaseTexImage(EGL_BACK_BUFFER);
        mBoundSurface = nullptr;
    }
    if (mBoundStream)
    {
        mBoundStream->releaseTextures();
        mBoundStream = nullptr;
    }
    SafeDelete(mTexture);
}

void Texture::setLabel(const std::string &label)
{
    mLabel = label;
}

const std::string &Texture::getLabel() const
{
    return mLabel;
}

GLenum Texture::getTarget() const
{
    return mState.target;
}

void Texture::setSwizzleRed(GLenum swizzleRed)
{
    mState.swizzleRed = swizzleRed;
}

GLenum Texture::getSwizzleRed() const
{
    return mState.swizzleRed;
}

void Texture::setSwizzleGreen(GLenum swizzleGreen)
{
    mState.swizzleGreen = swizzleGreen;
}

GLenum Texture::getSwizzleGreen() const
{
    return mState.swizzleGreen;
}

void Texture::setSwizzleBlue(GLenum swizzleBlue)
{
    mState.swizzleBlue = swizzleBlue;
}

GLenum Texture::getSwizzleBlue() const
{
    return mState.swizzleBlue;
}

void Texture::setSwizzleAlpha(GLenum swizzleAlpha)
{
    mState.swizzleAlpha = swizzleAlpha;
}

GLenum Texture::getSwizzleAlpha() const
{
    return mState.swizzleAlpha;
}

void Texture::setMinFilter(GLenum minFilter)
{
    mState.samplerState.minFilter = minFilter;
}

GLenum Texture::getMinFilter() const
{
    return mState.samplerState.minFilter;
}

void Texture::setMagFilter(GLenum magFilter)
{
    mState.samplerState.magFilter = magFilter;
}

GLenum Texture::getMagFilter() const
{
    return mState.samplerState.magFilter;
}

void Texture::setWrapS(GLenum wrapS)
{
    mState.samplerState.wrapS = wrapS;
}

GLenum Texture::getWrapS() const
{
    return mState.samplerState.wrapS;
}

void Texture::setWrapT(GLenum wrapT)
{
    mState.samplerState.wrapT = wrapT;
}

GLenum Texture::getWrapT() const
{
    return mState.samplerState.wrapT;
}

void Texture::setWrapR(GLenum wrapR)
{
    mState.samplerState.wrapR = wrapR;
}

GLenum Texture::getWrapR() const
{
    return mState.samplerState.wrapR;
}

void Texture::setMaxAnisotropy(float maxAnisotropy)
{
    mState.samplerState.maxAnisotropy = maxAnisotropy;
}

float Texture::getMaxAnisotropy() const
{
    return mState.samplerState.maxAnisotropy;
}

void Texture::setMinLod(GLfloat minLod)
{
    mState.samplerState.minLod = minLod;
}

GLfloat Texture::getMinLod() const
{
    return mState.samplerState.minLod;
}

void Texture::setMaxLod(GLfloat maxLod)
{
    mState.samplerState.maxLod = maxLod;
}

GLfloat Texture::getMaxLod() const
{
    return mState.samplerState.maxLod;
}

void Texture::setCompareMode(GLenum compareMode)
{
    mState.samplerState.compareMode = compareMode;
}

GLenum Texture::getCompareMode() const
{
    return mState.samplerState.compareMode;
}

void Texture::setCompareFunc(GLenum compareFunc)
{
    mState.samplerState.compareFunc = compareFunc;
}

GLenum Texture::getCompareFunc() const
{
    return mState.samplerState.compareFunc;
}

const SamplerState &Texture::getSamplerState() const
{
    return mState.samplerState;
}

void Texture::setBaseLevel(GLuint baseLevel)
{
    if (mState.baseLevel != baseLevel)
    {
        mState.baseLevel              = baseLevel;
        mCompletenessCache.cacheValid = false;
        mTexture->setBaseLevel(mState.getEffectiveBaseLevel());
    }
}

GLuint Texture::getBaseLevel() const
{
    return mState.baseLevel;
}

void Texture::setMaxLevel(GLuint maxLevel)
{
    if (mState.maxLevel != maxLevel)
    {
        mState.maxLevel               = maxLevel;
        mCompletenessCache.cacheValid = false;
    }
}

GLuint Texture::getMaxLevel() const
{
    return mState.maxLevel;
}

bool Texture::getImmutableFormat() const
{
    return mState.immutableFormat;
}

GLuint Texture::getImmutableLevels() const
{
    return mState.immutableLevels;
}

void Texture::setUsage(GLenum usage)
{
    mState.usage = usage;
    getImplementation()->setUsage(usage);
}

GLenum Texture::getUsage() const
{
    return mState.usage;
}

const TextureState &Texture::getTextureState() const
{
    return mState;
}

size_t Texture::getWidth(GLenum target, size_t level) const
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(target, level).size.width;
}

size_t Texture::getHeight(GLenum target, size_t level) const
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(target, level).size.height;
}

size_t Texture::getDepth(GLenum target, size_t level) const
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(target, level).size.depth;
}

GLenum Texture::getInternalFormat(GLenum target, size_t level) const
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return getImageDesc(target, level).internalFormat;
}

bool Texture::isSamplerComplete(const SamplerState &samplerState, const ContextState &data) const
{
    const ImageDesc &baseImageDesc =
        getImageDesc(getBaseImageTarget(), mState.getEffectiveBaseLevel());
    const TextureCaps &textureCaps = data.textureCaps->get(baseImageDesc.internalFormat);
    if (!mCompletenessCache.cacheValid ||
        mCompletenessCache.samplerState != samplerState ||
        mCompletenessCache.filterable != textureCaps.filterable ||
        mCompletenessCache.clientVersion != data.clientVersion ||
        mCompletenessCache.supportsNPOT != data.extensions->textureNPOT)
    {
        mCompletenessCache.cacheValid = true;
        mCompletenessCache.samplerState = samplerState;
        mCompletenessCache.filterable = textureCaps.filterable;
        mCompletenessCache.clientVersion = data.clientVersion;
        mCompletenessCache.supportsNPOT = data.extensions->textureNPOT;
        mCompletenessCache.samplerComplete = computeSamplerCompleteness(samplerState, data);
    }
    return mCompletenessCache.samplerComplete;
}

bool Texture::isMipmapComplete() const
{
    return computeMipmapCompleteness();
}

// Tests for cube texture completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture::isCubeComplete() const
{
    ASSERT(mState.target == GL_TEXTURE_CUBE_MAP);

    const ImageDesc &baseImageDesc = getImageDesc(FirstCubeMapTextureTarget, 0);
    if (baseImageDesc.size.width == 0 || baseImageDesc.size.width != baseImageDesc.size.height)
    {
        return false;
    }

    for (GLenum face = FirstCubeMapTextureTarget + 1; face <= LastCubeMapTextureTarget; face++)
    {
        const ImageDesc &faceImageDesc = getImageDesc(face, 0);
        if (faceImageDesc.size.width != baseImageDesc.size.width ||
            faceImageDesc.size.height != baseImageDesc.size.height ||
            faceImageDesc.internalFormat != baseImageDesc.internalFormat)
        {
            return false;
        }
    }

    return true;
}

size_t Texture::getMipCompleteLevels() const
{
    const ImageDesc &baseImageDesc = getImageDesc(getBaseImageTarget(), 0);
    if (mState.target == GL_TEXTURE_3D)
    {
        const int maxDim = std::max(std::max(baseImageDesc.size.width, baseImageDesc.size.height),
                                    baseImageDesc.size.depth);
        return log2(maxDim) + 1;
    }
    else
    {
        return log2(std::max(baseImageDesc.size.width, baseImageDesc.size.height)) + 1;
    }
}

egl::Surface *Texture::getBoundSurface() const
{
    return mBoundSurface;
}

egl::Stream *Texture::getBoundStream() const
{
    return mBoundStream;
}

Error Texture::setImage(const PixelUnpackState &unpackState,
                        GLenum target,
                        size_t level,
                        GLenum internalFormat,
                        const Extents &size,
                        GLenum format,
                        GLenum type,
                        const uint8_t *pixels)
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    // Release from previous calls to eglBindTexImage, to avoid calling the Impl after
    releaseTexImageInternal();
    orphanImages();

    Error error =
        mTexture->setImage(target, level, internalFormat, size, format, type, unpackState, pixels);
    if (error.isError())
    {
        return error;
    }

    setImageDesc(target, level, ImageDesc(size, GetSizedInternalFormat(internalFormat, type)));

    return Error(GL_NO_ERROR);
}

Error Texture::setSubImage(const PixelUnpackState &unpackState,
                           GLenum target,
                           size_t level,
                           const Box &area,
                           GLenum format,
                           GLenum type,
                           const uint8_t *pixels)
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));
    return mTexture->setSubImage(target, level, area, format, type, unpackState, pixels);
}

Error Texture::setCompressedImage(const PixelUnpackState &unpackState,
                                  GLenum target,
                                  size_t level,
                                  GLenum internalFormat,
                                  const Extents &size,
                                  size_t imageSize,
                                  const uint8_t *pixels)
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    // Release from previous calls to eglBindTexImage, to avoid calling the Impl after
    releaseTexImageInternal();
    orphanImages();

    Error error = mTexture->setCompressedImage(target, level, internalFormat, size, unpackState,
                                               imageSize, pixels);
    if (error.isError())
    {
        return error;
    }

    setImageDesc(target, level, ImageDesc(size, GetSizedInternalFormat(internalFormat, GL_UNSIGNED_BYTE)));

    return Error(GL_NO_ERROR);
}

Error Texture::setCompressedSubImage(const PixelUnpackState &unpackState,
                                     GLenum target,
                                     size_t level,
                                     const Box &area,
                                     GLenum format,
                                     size_t imageSize,
                                     const uint8_t *pixels)
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    return mTexture->setCompressedSubImage(target, level, area, format, unpackState, imageSize,
                                           pixels);
}

Error Texture::copyImage(GLenum target, size_t level, const Rectangle &sourceArea, GLenum internalFormat,
                         const Framebuffer *source)
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    // Release from previous calls to eglBindTexImage, to avoid calling the Impl after
    releaseTexImageInternal();
    orphanImages();

    Error error = mTexture->copyImage(target, level, sourceArea, internalFormat, source);
    if (error.isError())
    {
        return error;
    }

    setImageDesc(target, level, ImageDesc(Extents(sourceArea.width, sourceArea.height, 1),
                                          GetSizedInternalFormat(internalFormat, GL_UNSIGNED_BYTE)));

    return Error(GL_NO_ERROR);
}

Error Texture::copySubImage(GLenum target, size_t level, const Offset &destOffset, const Rectangle &sourceArea,
                            const Framebuffer *source)
{
    ASSERT(target == mState.target ||
           (mState.target == GL_TEXTURE_CUBE_MAP && IsCubeMapTextureTarget(target)));

    return mTexture->copySubImage(target, level, destOffset, sourceArea, source);
}

Error Texture::setStorage(GLenum target, size_t levels, GLenum internalFormat, const Extents &size)
{
    ASSERT(target == mState.target);

    // Release from previous calls to eglBindTexImage, to avoid calling the Impl after
    releaseTexImageInternal();
    orphanImages();

    Error error = mTexture->setStorage(target, levels, internalFormat, size);
    if (error.isError())
    {
        return error;
    }

    mState.immutableFormat = true;
    mState.immutableLevels = static_cast<GLuint>(levels);
    clearImageDescs();
    setImageDescChain(levels, size, internalFormat);

    return Error(GL_NO_ERROR);
}

Error Texture::generateMipmaps()
{
    // Release from previous calls to eglBindTexImage, to avoid calling the Impl after
    releaseTexImageInternal();

    // EGL_KHR_gl_image states that images are only orphaned when generating mipmaps if the texture
    // is not mip complete.
    if (!isMipmapComplete())
    {
        orphanImages();
    }

    Error error = mTexture->generateMipmaps();
    if (error.isError())
    {
        return error;
    }

    const ImageDesc &baseImageInfo = getImageDesc(getBaseImageTarget(), 0);
    size_t mipLevels = log2(std::max(std::max(baseImageInfo.size.width, baseImageInfo.size.height), baseImageInfo.size.depth)) + 1;
    setImageDescChain(mipLevels, baseImageInfo.size, baseImageInfo.internalFormat);

    return Error(GL_NO_ERROR);
}

void Texture::setImageDescChain(size_t levels, Extents baseSize, GLenum sizedInternalFormat)
{
    for (int level = 0; level < static_cast<int>(levels); level++)
    {
        Extents levelSize(
            std::max<int>(baseSize.width >> level, 1), std::max<int>(baseSize.height >> level, 1),
            (mState.target == GL_TEXTURE_2D_ARRAY) ? baseSize.depth
                                                   : std::max<int>(baseSize.depth >> level, 1));
        ImageDesc levelInfo(levelSize, sizedInternalFormat);

        if (mState.target == GL_TEXTURE_CUBE_MAP)
        {
            for (GLenum face = FirstCubeMapTextureTarget; face <= LastCubeMapTextureTarget; face++)
            {
                setImageDesc(face, level, levelInfo);
            }
        }
        else
        {
            setImageDesc(mState.target, level, levelInfo);
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

const Texture::ImageDesc &Texture::getImageDesc(GLenum target, size_t level) const
{
    size_t descIndex = GetImageDescIndex(target, level);
    ASSERT(descIndex < mImageDescs.size());
    return mImageDescs[descIndex];
}

void Texture::setImageDesc(GLenum target, size_t level, const ImageDesc &desc)
{
    size_t descIndex = GetImageDescIndex(target, level);
    ASSERT(descIndex < mImageDescs.size());
    mImageDescs[descIndex] = desc;
    mCompletenessCache.cacheValid = false;
}

void Texture::clearImageDesc(GLenum target, size_t level)
{
    setImageDesc(target, level, ImageDesc());
}

void Texture::clearImageDescs()
{
    for (size_t descIndex = 0; descIndex < mImageDescs.size(); descIndex++)
    {
        mImageDescs[descIndex] = ImageDesc();
    }
    mCompletenessCache.cacheValid = false;
}

void Texture::bindTexImageFromSurface(egl::Surface *surface)
{
    ASSERT(surface);

    if (mBoundSurface)
    {
        releaseTexImageFromSurface();
    }

    mTexture->bindTexImage(surface);
    mBoundSurface = surface;

    // Set the image info to the size and format of the surface
    ASSERT(mState.target == GL_TEXTURE_2D);
    Extents size(surface->getWidth(), surface->getHeight(), 1);
    ImageDesc desc(size, surface->getConfig()->renderTargetFormat);
    setImageDesc(mState.target, 0, desc);
}

void Texture::releaseTexImageFromSurface()
{
    ASSERT(mBoundSurface);
    mBoundSurface = nullptr;
    mTexture->releaseTexImage();

    // Erase the image info for level 0
    ASSERT(mState.target == GL_TEXTURE_2D);
    clearImageDesc(mState.target, 0);
}

void Texture::bindStream(egl::Stream *stream)
{
    ASSERT(stream);

    // It should not be possible to bind a texture already bound to another stream
    ASSERT(mBoundStream == nullptr);

    mBoundStream = stream;

    ASSERT(mState.target == GL_TEXTURE_EXTERNAL_OES);
}

void Texture::releaseStream()
{
    ASSERT(mBoundStream);
    mBoundStream = nullptr;
}

void Texture::acquireImageFromStream(const egl::Stream::GLTextureDescription &desc)
{
    ASSERT(mBoundStream != nullptr);
    mTexture->setImageExternal(mState.target, mBoundStream, desc);

    Extents size(desc.width, desc.height, 1);
    setImageDesc(mState.target, 0, ImageDesc(size, desc.internalFormat));
}

void Texture::releaseImageFromStream()
{
    ASSERT(mBoundStream != nullptr);
    mTexture->setImageExternal(mState.target, nullptr, egl::Stream::GLTextureDescription());

    // Set to incomplete
    clearImageDesc(mState.target, 0);
}

void Texture::releaseTexImageInternal()
{
    if (mBoundSurface)
    {
        // Notify the surface
        mBoundSurface->releaseTexImageFromTexture();

        // Then, call the same method as from the surface
        releaseTexImageFromSurface();
    }
}

Error Texture::setEGLImageTarget(GLenum target, egl::Image *imageTarget)
{
    ASSERT(target == mState.target);
    ASSERT(target == GL_TEXTURE_2D);

    // Release from previous calls to eglBindTexImage, to avoid calling the Impl after
    releaseTexImageInternal();
    orphanImages();

    Error error = mTexture->setEGLImageTarget(target, imageTarget);
    if (error.isError())
    {
        return error;
    }

    setTargetImage(imageTarget);

    Extents size(static_cast<int>(imageTarget->getWidth()),
                 static_cast<int>(imageTarget->getHeight()), 1);
    GLenum internalFormat = imageTarget->getInternalFormat();
    GLenum type           = GetInternalFormatInfo(internalFormat).type;

    clearImageDescs();
    setImageDesc(target, 0, ImageDesc(size, GetSizedInternalFormat(internalFormat, type)));

    return Error(GL_NO_ERROR);
}

GLenum Texture::getBaseImageTarget() const
{
    return mState.target == GL_TEXTURE_CUBE_MAP ? FirstCubeMapTextureTarget : mState.target;
}

bool Texture::computeSamplerCompleteness(const SamplerState &samplerState,
                                         const ContextState &data) const
{
    if (mState.baseLevel > mState.maxLevel)
    {
        return false;
    }
    const ImageDesc &baseImageDesc =
        getImageDesc(getBaseImageTarget(), mState.getEffectiveBaseLevel());
    if (baseImageDesc.size.width == 0 || baseImageDesc.size.height == 0 || baseImageDesc.size.depth == 0)
    {
        return false;
    }
    // The cases where the texture is incomplete because base level is out of range should be
    // handled by the above condition.
    ASSERT(mState.baseLevel < gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS || mState.immutableFormat);

    if (mState.target == GL_TEXTURE_CUBE_MAP &&
        baseImageDesc.size.width != baseImageDesc.size.height)
    {
        return false;
    }

    const TextureCaps &textureCaps = data.textureCaps->get(baseImageDesc.internalFormat);
    if (!textureCaps.filterable && !IsPointSampled(samplerState))
    {
        return false;
    }

    bool npotSupport = data.extensions->textureNPOT || data.clientVersion >= 3;
    if (!npotSupport)
    {
        if ((samplerState.wrapS != GL_CLAMP_TO_EDGE && !gl::isPow2(baseImageDesc.size.width)) ||
            (samplerState.wrapT != GL_CLAMP_TO_EDGE && !gl::isPow2(baseImageDesc.size.height)))
        {
            return false;
        }
    }

    if (IsMipmapFiltered(samplerState))
    {
        if (!npotSupport)
        {
            if (!gl::isPow2(baseImageDesc.size.width) || !gl::isPow2(baseImageDesc.size.height))
            {
                return false;
            }
        }

        if (!computeMipmapCompleteness())
        {
            return false;
        }
    }
    else
    {
        if (mState.target == GL_TEXTURE_CUBE_MAP && !isCubeComplete())
        {
            return false;
        }
    }

    // OpenGLES 3.0.2 spec section 3.8.13 states that a texture is not mipmap complete if:
    // The internalformat specified for the texture arrays is a sized internal depth or
    // depth and stencil format (see table 3.13), the value of TEXTURE_COMPARE_-
    // MODE is NONE, and either the magnification filter is not NEAREST or the mini-
    // fication filter is neither NEAREST nor NEAREST_MIPMAP_NEAREST.
    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(baseImageDesc.internalFormat);
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

bool Texture::computeMipmapCompleteness() const
{
    size_t expectedMipLevels = getMipCompleteLevels();

    size_t maxLevel = std::min<size_t>(expectedMipLevels, mState.maxLevel + 1);

    for (size_t level = mState.getEffectiveBaseLevel(); level < maxLevel; level++)
    {
        if (mState.target == GL_TEXTURE_CUBE_MAP)
        {
            for (GLenum face = FirstCubeMapTextureTarget; face <= LastCubeMapTextureTarget; face++)
            {
                if (!computeLevelCompleteness(face, level))
                {
                    return false;
                }
            }
        }
        else
        {
            if (!computeLevelCompleteness(mState.target, level))
            {
                return false;
            }
        }
    }

    return true;
}

bool Texture::computeLevelCompleteness(GLenum target, size_t level) const
{
    ASSERT(level < IMPLEMENTATION_MAX_TEXTURE_LEVELS);

    if (mState.immutableFormat)
    {
        return true;
    }

    const ImageDesc &baseImageDesc =
        getImageDesc(getBaseImageTarget(), mState.getEffectiveBaseLevel());
    if (baseImageDesc.size.width == 0 || baseImageDesc.size.height == 0 || baseImageDesc.size.depth == 0)
    {
        return false;
    }

    const ImageDesc &levelImageDesc = getImageDesc(target, level);
    if (levelImageDesc.size.width == 0 || levelImageDesc.size.height == 0 ||
        levelImageDesc.size.depth == 0)
    {
        return false;
    }

    if (levelImageDesc.internalFormat != baseImageDesc.internalFormat)
    {
        return false;
    }

    ASSERT(level >= mState.getEffectiveBaseLevel());
    const size_t relativeLevel = level - mState.getEffectiveBaseLevel();
    if (levelImageDesc.size.width != std::max(1, baseImageDesc.size.width >> relativeLevel))
    {
        return false;
    }

    if (levelImageDesc.size.height != std::max(1, baseImageDesc.size.height >> relativeLevel))
    {
        return false;
    }

    if (mState.target == GL_TEXTURE_3D)
    {
        if (levelImageDesc.size.depth != std::max(1, baseImageDesc.size.depth >> relativeLevel))
        {
            return false;
        }
    }
    else if (mState.target == GL_TEXTURE_2D_ARRAY)
    {
        if (levelImageDesc.size.depth != baseImageDesc.size.depth)
        {
            return false;
        }
    }

    return true;
}

Texture::SamplerCompletenessCache::SamplerCompletenessCache()
    : cacheValid(false),
      samplerState(),
      filterable(false),
      clientVersion(0),
      supportsNPOT(false),
      samplerComplete(false)
{
}

Extents Texture::getAttachmentSize(const gl::FramebufferAttachment::Target &target) const
{
    return getImageDesc(target.textureIndex().type, target.textureIndex().mipIndex).size;
}

GLenum Texture::getAttachmentInternalFormat(const gl::FramebufferAttachment::Target &target) const
{
    return getInternalFormat(target.textureIndex().type, target.textureIndex().mipIndex);
}

GLsizei Texture::getAttachmentSamples(const gl::FramebufferAttachment::Target &/*target*/) const
{
    // Multisample textures not currently supported
    return 0;
}

void Texture::onAttach()
{
    addRef();
}

void Texture::onDetach()
{
    release();
}

GLuint Texture::getId() const
{
    return id();
}

rx::FramebufferAttachmentObjectImpl *Texture::getAttachmentImpl() const
{
    return mTexture;
}
}
