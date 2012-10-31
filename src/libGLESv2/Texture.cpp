//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "libGLESv2/Texture.h"

#include <algorithm>

#include "common/debug.h"

#include "libEGL/Display.h"

#include "libGLESv2/main.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/Blit.h"
#include "libGLESv2/Framebuffer.h"

namespace gl
{

namespace
{
struct L8
{
    unsigned char L;

    static void average(L8 *dst, const L8 *src1, const L8 *src2)
    {
        dst->L = ((src1->L ^ src2->L) >> 1) + (src1->L & src2->L);
    }
};

struct A8L8
{
    unsigned char L;
    unsigned char A;

    static void average(A8L8 *dst, const A8L8 *src1, const A8L8 *src2)
    {
        *(unsigned short*)dst = (((*(unsigned short*)src1 ^ *(unsigned short*)src2) & 0xFEFE) >> 1) + (*(unsigned short*)src1 & *(unsigned short*)src2);
    }
};

struct A8R8G8B8
{
    unsigned char B;
    unsigned char G;
    unsigned char R;
    unsigned char A;

    static void average(A8R8G8B8 *dst, const A8R8G8B8 *src1, const A8R8G8B8 *src2)
    {
        *(unsigned int*)dst = (((*(unsigned int*)src1 ^ *(unsigned int*)src2) & 0xFEFEFEFE) >> 1) + (*(unsigned int*)src1 & *(unsigned int*)src2);
    }
};

struct A16B16G16R16F
{
    unsigned short R;
    unsigned short G;
    unsigned short B;
    unsigned short A;

    static void average(A16B16G16R16F *dst, const A16B16G16R16F *src1, const A16B16G16R16F *src2)
    {
        dst->R = float32ToFloat16((float16ToFloat32(src1->R) + float16ToFloat32(src2->R)) * 0.5f);
        dst->G = float32ToFloat16((float16ToFloat32(src1->G) + float16ToFloat32(src2->G)) * 0.5f);
        dst->B = float32ToFloat16((float16ToFloat32(src1->B) + float16ToFloat32(src2->B)) * 0.5f);
        dst->A = float32ToFloat16((float16ToFloat32(src1->A) + float16ToFloat32(src2->A)) * 0.5f);
    }
};

struct A32B32G32R32F
{
    float R;
    float G;
    float B;
    float A;

    static void average(A32B32G32R32F *dst, const A32B32G32R32F *src1, const A32B32G32R32F *src2)
    {
        dst->R = (src1->R + src2->R) * 0.5f;
        dst->G = (src1->G + src2->G) * 0.5f;
        dst->B = (src1->B + src2->B) * 0.5f;
        dst->A = (src1->A + src2->A) * 0.5f;
    }
};

template <typename T>
void GenerateMip(unsigned int sourceWidth, unsigned int sourceHeight,
                 const unsigned char *sourceData, int sourcePitch,
                 unsigned char *destData, int destPitch)
{
    unsigned int mipWidth = std::max(1U, sourceWidth >> 1);
    unsigned int mipHeight = std::max(1U, sourceHeight >> 1);

    if (sourceHeight == 1)
    {
        ASSERT(sourceWidth != 1);

        const T *src = (const T*)sourceData;
        T *dst = (T*)destData;

        for (unsigned int x = 0; x < mipWidth; x++)
        {
            T::average(&dst[x], &src[x * 2], &src[x * 2 + 1]);
        }
    }
    else if (sourceWidth == 1)
    {
        ASSERT(sourceHeight != 1);

        for (unsigned int y = 0; y < mipHeight; y++)
        {
            const T *src0 = (const T*)(sourceData + y * 2 * sourcePitch);
            const T *src1 = (const T*)(sourceData + y * 2 * sourcePitch + sourcePitch);
            T *dst = (T*)(destData + y * destPitch);

            T::average(dst, src0, src1);
        }
    }
    else
    {
        for (unsigned int y = 0; y < mipHeight; y++)
        {
            const T *src0 = (const T*)(sourceData + y * 2 * sourcePitch);
            const T *src1 = (const T*)(sourceData + y * 2 * sourcePitch + sourcePitch);
            T *dst = (T*)(destData + y * destPitch);

            for (unsigned int x = 0; x < mipWidth; x++)
            {
                T tmp0;
                T tmp1;

                T::average(&tmp0, &src0[x * 2], &src0[x * 2 + 1]);
                T::average(&tmp1, &src1[x * 2], &src1[x * 2 + 1]);
                T::average(&dst[x], &tmp0, &tmp1);
            }
        }
    }
}

void GenerateMip(IDirect3DSurface9 *destSurface, IDirect3DSurface9 *sourceSurface)
{
    D3DSURFACE_DESC destDesc;
    HRESULT result = destSurface->GetDesc(&destDesc);
    ASSERT(SUCCEEDED(result));

    D3DSURFACE_DESC sourceDesc;
    result = sourceSurface->GetDesc(&sourceDesc);
    ASSERT(SUCCEEDED(result));

    ASSERT(sourceDesc.Format == destDesc.Format);
    ASSERT(sourceDesc.Width == 1 || sourceDesc.Width / 2 == destDesc.Width);
    ASSERT(sourceDesc.Height == 1 || sourceDesc.Height / 2 == destDesc.Height);

    D3DLOCKED_RECT sourceLocked = {0};
    result = sourceSurface->LockRect(&sourceLocked, NULL, D3DLOCK_READONLY);
    ASSERT(SUCCEEDED(result));

    D3DLOCKED_RECT destLocked = {0};
    result = destSurface->LockRect(&destLocked, NULL, 0);
    ASSERT(SUCCEEDED(result));

    const unsigned char *sourceData = reinterpret_cast<const unsigned char*>(sourceLocked.pBits);
    unsigned char *destData = reinterpret_cast<unsigned char*>(destLocked.pBits);

    if (sourceData && destData)
    {
        switch (sourceDesc.Format)
        {
          case D3DFMT_L8:
            GenerateMip<L8>(sourceDesc.Width, sourceDesc.Height, sourceData, sourceLocked.Pitch, destData, destLocked.Pitch);
            break;
          case D3DFMT_A8L8:
            GenerateMip<A8L8>(sourceDesc.Width, sourceDesc.Height, sourceData, sourceLocked.Pitch, destData, destLocked.Pitch);
            break;
          case D3DFMT_A8R8G8B8:
          case D3DFMT_X8R8G8B8:
            GenerateMip<A8R8G8B8>(sourceDesc.Width, sourceDesc.Height, sourceData, sourceLocked.Pitch, destData, destLocked.Pitch);
            break;
          case D3DFMT_A16B16G16R16F:
            GenerateMip<A16B16G16R16F>(sourceDesc.Width, sourceDesc.Height, sourceData, sourceLocked.Pitch, destData, destLocked.Pitch);
            break;
          case D3DFMT_A32B32G32R32F:
            GenerateMip<A32B32G32R32F>(sourceDesc.Width, sourceDesc.Height, sourceData, sourceLocked.Pitch, destData, destLocked.Pitch);
            break;
          default:
            UNREACHABLE();
            break;
        }

        destSurface->UnlockRect();
        sourceSurface->UnlockRect();
    }
}
}

Texture::Texture(GLuint id) : RefCountObject(id)
{
    mSamplerState.minFilter = GL_NEAREST_MIPMAP_LINEAR;
    mSamplerState.magFilter = GL_LINEAR;
    mSamplerState.wrapS = GL_REPEAT;
    mSamplerState.wrapT = GL_REPEAT;
    mSamplerState.maxAnisotropy = 1.0f;
    mSamplerState.lodOffset = 0;
    mDirtyParameters = true;
    mUsage = GL_NONE;
    
    mDirtyImages = true;

    mImmutable = false;
}

Texture::~Texture()
{
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
        {
            if (mSamplerState.minFilter != filter)
            {
                mSamplerState.minFilter = filter;
                mDirtyParameters = true;
            }
            return true;
        }
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
        {
            if (mSamplerState.magFilter != filter)
            {
                mSamplerState.magFilter = filter;
                mDirtyParameters = true;
            }
            return true;
        }
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
        {
            if (mSamplerState.wrapS != wrap)
            {
                mSamplerState.wrapS = wrap;
                mDirtyParameters = true;
            }
            return true;
        }
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
        {
            if (mSamplerState.wrapT != wrap)
            {
                mSamplerState.wrapT = wrap;
                mDirtyParameters = true;
            }
            return true;
        }
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
    if (mSamplerState.maxAnisotropy != textureMaxAnisotropy)
    {
        mSamplerState.maxAnisotropy = textureMaxAnisotropy;
        mDirtyParameters = true;
    }
    return true;
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

float Texture::getMaxAnisotropy() const
{
    return mSamplerState.maxAnisotropy;
}

int Texture::getLodOffset()
{
    TextureStorage *texture = getStorage(false);
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

bool Texture::isMipmapFiltered() const
{
    switch (mSamplerState.minFilter)
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

void Texture::setImage(GLint unpackAlignment, const void *pixels, Image *image)
{
    if (pixels != NULL)
    {
        image->loadData(0, 0, image->getWidth(), image->getHeight(), unpackAlignment, pixels);
        mDirtyImages = true;
    }
}

void Texture::setCompressedImage(GLsizei imageSize, const void *pixels, Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(0, 0, image->getWidth(), image->getHeight(), pixels);
        mDirtyImages = true;
    }
}

bool Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *image)
{
    if (pixels != NULL)
    {
        image->loadData(xoffset, yoffset, width, height, unpackAlignment, pixels);
        mDirtyImages = true;
    }

    return true;
}

bool Texture::subImageCompressed(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels, Image *image)
{
    if (pixels != NULL)
    {
        image->loadCompressedData(xoffset, yoffset, width, height, pixels);
        mDirtyImages = true;
    }

    return true;
}

// D3D9_REPLACE
IDirect3DBaseTexture9 *Texture::getD3DTexture()
{
    // ensure the underlying texture is created
    if (getStorage(false) == NULL)
    {
        return NULL;
    }

    updateTexture();

    return getBaseTexture();
}

bool Texture::hasDirtyParameters() const
{
    return mDirtyParameters;
}

bool Texture::hasDirtyImages() const
{
    return mDirtyImages;
}

void Texture::resetDirty()
{
    mDirtyParameters = false;
    mDirtyImages = false;
}

unsigned int Texture::getTextureSerial()
{
    TextureStorage *texture = getStorage(false);
    return texture ? texture->getTextureSerial() : 0;
}

unsigned int Texture::getRenderTargetSerial(GLenum target)
{
    TextureStorage *texture = getStorage(true);
    return texture ? texture->getRenderTargetSerial(target) : 0;
}

bool Texture::isImmutable() const
{
    return mImmutable;
}

GLint Texture::creationLevels(GLsizei width, GLsizei height) const
{
    if ((isPow2(width) && isPow2(height)) || getContext()->supportsNonPower2Texture())
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

int Texture::levelCount()
{
    return getBaseTexture() ? getBaseTexture()->GetLevelCount() - getLodOffset() : 0;
}

Blit *Texture::getBlitter()
{
    Context *context = getContext();
    return context->getBlitter();
}

bool Texture::copyToRenderTarget(IDirect3DSurface9 *dest, IDirect3DSurface9 *source, bool fromManaged)
{
    if (source && dest)
    {
        HRESULT result = D3DERR_OUTOFVIDEOMEMORY;
        renderer::Renderer9 *renderer = getDisplay()->getRenderer();
        IDirect3DDevice9 *device = renderer->getDevice(); // D3D9_REPLACE

        if (fromManaged)
        {
            D3DSURFACE_DESC desc;
            source->GetDesc(&desc);

            IDirect3DSurface9 *surf = 0;
            result = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &surf, NULL);

            if (SUCCEEDED(result))
            {
                Image::CopyLockableSurfaces(surf, source);
                result = device->UpdateSurface(surf, NULL, dest, NULL);
                surf->Release();
            }
        }
        else
        {
            renderer->endScene();
            result = device->StretchRect(source, NULL, dest, NULL, D3DTEXF_NONE);
        }

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            return false;
        }
    }

    return true;
}

Texture2D::Texture2D(GLuint id) : Texture(id)
{
    mTexStorage = NULL;
    mSurface = NULL;
    mColorbufferProxy = NULL;
    mProxyRefs = 0;
}

Texture2D::~Texture2D()
{
    mColorbufferProxy = NULL;

    delete mTexStorage;
    mTexStorage = NULL;
    
    if (mSurface)
    {
        mSurface->setBoundTexture(NULL);
        mSurface = NULL;
    }
}

// We need to maintain a count of references to renderbuffers acting as 
// proxies for this texture, so that we do not attempt to use a pointer 
// to a renderbuffer proxy which has been deleted.
void Texture2D::addProxyRef(const Renderbuffer *proxy)
{
    mProxyRefs++;
}

void Texture2D::releaseProxy(const Renderbuffer *proxy)
{
    if (mProxyRefs > 0)
        mProxyRefs--;

    if (mProxyRefs == 0)
        mColorbufferProxy = NULL;
}

GLenum Texture2D::getTarget() const
{
    return GL_TEXTURE_2D;
}

GLsizei Texture2D::getWidth(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level].getWidth();
    else
        return 0;
}

GLsizei Texture2D::getHeight(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level].getHeight();
    else
        return 0;
}

GLenum Texture2D::getInternalFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level].getInternalFormat();
    else
        return GL_NONE;
}

GLenum Texture2D::getActualFormat(GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[level].getActualFormat();
    else
        return D3DFMT_UNKNOWN;
}

void Texture2D::redefineImage(GLint level, GLint internalformat, GLsizei width, GLsizei height)
{
    releaseTexImage();

    bool redefined = mImageArray[level].redefine(internalformat, width, height, false);

    if (mTexStorage && redefined)
    {
        for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
        {
            mImageArray[i].markDirty();
        }

        delete mTexStorage;
        mTexStorage = NULL;
        mDirtyImages = true;
    }
}

void Texture2D::setImage(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    GLint internalformat = ConvertSizedInternalFormat(format, type);
    redefineImage(level, internalformat, width, height);

    Texture::setImage(unpackAlignment, pixels, &mImageArray[level]);
}

void Texture2D::bindTexImage(egl::Surface *surface)
{
    releaseTexImage();

    GLint internalformat = surface->getFormat();

    mImageArray[0].redefine(internalformat, surface->getWidth(), surface->getHeight(), true);

    delete mTexStorage;
    renderer::SwapChain *swapchain = surface->getSwapChain();  // D3D9_REPLACE
    mTexStorage = new TextureStorage2D(swapchain);

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
            mImageArray[i].redefine(GL_RGBA8_OES, 0, 0, true);
        }
    }
}

void Texture2D::setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(level, format, width, height);

    Texture::setCompressedImage(imageSize, pixels, &mImageArray[level]);
}

void Texture2D::commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    ASSERT(mImageArray[level].getSurface() != NULL);

    if (level < levelCount())
    {
        IDirect3DSurface9 *destLevel = mTexStorage->getSurfaceLevel(level, true);

        if (destLevel)
        {
            Image *image = &mImageArray[level];
            image->updateSurface(destLevel, xoffset, yoffset, width, height);

            destLevel->Release();
            image->markClean();
        }
    }
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    if (Texture::subImage(xoffset, yoffset, width, height, format, type, unpackAlignment, pixels, &mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void Texture2D::subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (Texture::subImageCompressed(xoffset, yoffset, width, height, format, imageSize, pixels, &mImageArray[level]))
    {
        commitRect(level, xoffset, yoffset, width, height);
    }
}

void Texture2D::copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    IDirect3DSurface9 *renderTarget = source->getRenderTarget();

    if (!renderTarget)
    {
        ERR("Failed to retrieve the render target.");
        return error(GL_OUT_OF_MEMORY);
    }

    GLint internalformat = ConvertSizedInternalFormat(format, GL_UNSIGNED_BYTE);
    redefineImage(level, internalformat, width, height);
   
    if (!mImageArray[level].isRenderableFormat())
    {
        mImageArray[level].copy(0, 0, x, y, width, height, renderTarget);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        mImageArray[level].markClean();

        if (width != 0 && height != 0 && level < levelCount())
        {
            RECT sourceRect;
            sourceRect.left = x;
            sourceRect.right = x + width;
            sourceRect.top = y;
            sourceRect.bottom = y + height;
            
            IDirect3DSurface9 *dest = mTexStorage->getSurfaceLevel(level, true);

            if (dest)
            {
                getBlitter()->copy(renderTarget, sourceRect, format, 0, 0, dest);
                dest->Release();
            }
        }
    }

    renderTarget->Release();
}

void Texture2D::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    if (xoffset + width > mImageArray[level].getWidth() || yoffset + height > mImageArray[level].getHeight())
    {
        return error(GL_INVALID_VALUE);
    }

    IDirect3DSurface9 *renderTarget = source->getRenderTarget();

    if (!renderTarget)
    {
        ERR("Failed to retrieve the render target.");
        return error(GL_OUT_OF_MEMORY);
    }

    if (!mImageArray[level].isRenderableFormat() || (!mTexStorage && !isSamplerComplete()))
    {
        mImageArray[level].copy(xoffset, yoffset, x, y, width, height, renderTarget);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        updateTexture();

        if (level < levelCount())
        {
            RECT sourceRect;
            sourceRect.left = x;
            sourceRect.right = x + width;
            sourceRect.top = y;
            sourceRect.bottom = y + height;

            IDirect3DSurface9 *dest = mTexStorage->getSurfaceLevel(level, true);

            if (dest)
            {
                getBlitter()->copy(renderTarget, sourceRect, 
                                   gl::ExtractFormat(mImageArray[0].getInternalFormat()),
                                   xoffset, yoffset, dest);
                dest->Release();
            }
        }
    }

    renderTarget->Release();
}

void Texture2D::storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height)
{
    delete mTexStorage;
    mTexStorage = new TextureStorage2D(levels, internalformat, mUsage, false, width, height);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        mImageArray[level].redefine(internalformat, width, height, true);
        width = std::max(1, width >> 1);
        height = std::max(1, height >> 1);
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        mImageArray[level].redefine(GL_NONE, 0, 0, true);
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int level = 0; level < levels; level++)
        {
            IDirect3DSurface9 *surface = mTexStorage->getSurfaceLevel(level, false);
            mImageArray[level].setManagedSurface(surface);
        }
    }
}

// Tests for 2D texture sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 85.
bool Texture2D::isSamplerComplete() const
{
    GLsizei width = mImageArray[0].getWidth();
    GLsizei height = mImageArray[0].getHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    bool mipmapping = isMipmapFiltered();

    if ((IsFloat32Format(getInternalFormat(0)) && !getContext()->supportsFloat32LinearFilter()) ||
        (IsFloat16Format(getInternalFormat(0)) && !getContext()->supportsFloat16LinearFilter()))
    {
        if (mSamplerState.magFilter != GL_NEAREST ||
            (mSamplerState.minFilter != GL_NEAREST && mSamplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    bool npotSupport = getContext()->supportsNonPower2Texture();

    if (!npotSupport)
    {
        if ((mSamplerState.wrapS != GL_CLAMP_TO_EDGE && !isPow2(width)) ||
            (mSamplerState.wrapT != GL_CLAMP_TO_EDGE && !isPow2(height)))
        {
            return false;
        }
    }

    if (mipmapping)
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

    return true;
}

// Tests for 2D texture (mipmap) completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isMipmapComplete() const
{
    if (isImmutable())
    {
        return true;
    }

    GLsizei width = mImageArray[0].getWidth();
    GLsizei height = mImageArray[0].getHeight();

    if (width <= 0 || height <= 0)
    {
        return false;
    }

    int q = log2(std::max(width, height));

    for (int level = 1; level <= q; level++)
    {
        if (mImageArray[level].getInternalFormat() != mImageArray[0].getInternalFormat())
        {
            return false;
        }

        if (mImageArray[level].getWidth() != std::max(1, width >> level))
        {
            return false;
        }

        if (mImageArray[level].getHeight() != std::max(1, height >> level))
        {
            return false;
        }
    }

    return true;
}

bool Texture2D::isCompressed(GLint level) const
{
    return IsCompressed(getInternalFormat(level));
}

bool Texture2D::isDepth(GLint level) const
{
    return IsDepthTexture(getInternalFormat(level));
}

IDirect3DBaseTexture9 *Texture2D::getBaseTexture() const
{
    return mTexStorage ? mTexStorage->getBaseTexture() : NULL;
}

// Constructs a native texture resource from the texture images
void Texture2D::createTexture()
{
    GLsizei width = mImageArray[0].getWidth();
    GLsizei height = mImageArray[0].getHeight();

    if (!(width > 0 && height > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(width, height);
    GLenum internalformat = mImageArray[0].getInternalFormat();

    delete mTexStorage;
    mTexStorage = new TextureStorage2D(levels, internalformat, mUsage, false, width, height);
    
    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int level = 0; level < levels; level++)
        {
            IDirect3DSurface9 *surface = mTexStorage->getSurfaceLevel(level, false);
            mImageArray[level].setManagedSurface(surface);
        }
    }

    mDirtyImages = true;
}

void Texture2D::updateTexture()
{
    bool mipmapping = (isMipmapFiltered() && isMipmapComplete());

    int levels = (mipmapping ? levelCount() : 1);

    for (int level = 0; level < levels; level++)
    {
        Image *image = &mImageArray[level];

        if (image->isDirty())
        {
            commitRect(level, 0, 0, mImageArray[level].getWidth(), mImageArray[level].getHeight());
        }
    }
}

void Texture2D::convertToRenderTarget()
{
    TextureStorage2D *newTexStorage = NULL;

    if (mImageArray[0].getWidth() != 0 && mImageArray[0].getHeight() != 0)
    {
        GLsizei width = mImageArray[0].getWidth();
        GLsizei height = mImageArray[0].getHeight();
        GLint levels = creationLevels(width, height);
        GLenum internalformat = mImageArray[0].getInternalFormat();

        newTexStorage = new TextureStorage2D(levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, true, width, height);

        if (mTexStorage != NULL)
        {
            int levels = levelCount();
            for (int i = 0; i < levels; i++)
            {
                IDirect3DSurface9 *source = mTexStorage->getSurfaceLevel(i, false);
                IDirect3DSurface9 *dest = newTexStorage->getSurfaceLevel(i, true);

                if (!copyToRenderTarget(dest, source, mTexStorage->isManaged()))
                {   
                   delete newTexStorage;
                   if (source) source->Release();
                   if (dest) dest->Release();
                   return error(GL_OUT_OF_MEMORY);
                }

                if (source) source->Release();
                if (dest) dest->Release();
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

void Texture2D::generateMipmaps()
{
    if (!getContext()->supportsNonPower2Texture())
    {
        if (!isPow2(mImageArray[0].getWidth()) || !isPow2(mImageArray[0].getHeight()))
        {
            return error(GL_INVALID_OPERATION);
        }
    }

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    unsigned int q = log2(std::max(mImageArray[0].getWidth(), mImageArray[0].getHeight()));
    for (unsigned int i = 1; i <= q; i++)
    {
        redefineImage(i, mImageArray[0].getInternalFormat(), 
                         std::max(mImageArray[0].getWidth() >> i, 1),
                         std::max(mImageArray[0].getHeight() >> i, 1));
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            IDirect3DSurface9 *upper = mTexStorage->getSurfaceLevel(i - 1, false);
            IDirect3DSurface9 *lower = mTexStorage->getSurfaceLevel(i, true);

            if (upper != NULL && lower != NULL)
            {
                getBlitter()->boxFilter(upper, lower);
            }

            if (upper != NULL) upper->Release();
            if (lower != NULL) lower->Release();

            mImageArray[i].markClean();
        }
    }
    else
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            if (mImageArray[i].getSurface() == NULL)
            {
                return error(GL_OUT_OF_MEMORY);
            }

            GenerateMip(mImageArray[i].getSurface(), mImageArray[i - 1].getSurface());

            mImageArray[i].markDirty();
        }
    }
}

Renderbuffer *Texture2D::getRenderbuffer(GLenum target)
{
    if (target != GL_TEXTURE_2D)
    {
        return error(GL_INVALID_OPERATION, (Renderbuffer *)NULL);
    }

    if (mColorbufferProxy == NULL)
    {
        mColorbufferProxy = new Renderbuffer(id(), new RenderbufferTexture2D(this, target));
    }

    return mColorbufferProxy;
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *Texture2D::getRenderTarget(GLenum target)
{
    ASSERT(target == GL_TEXTURE_2D);

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();
    
    // ensure this is NOT a depth texture
    if (isDepth(0))
    {
        return NULL;
    }
    return mTexStorage->getSurfaceLevel(0, false);
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *Texture2D::getDepthStencil(GLenum target)
{
    ASSERT(target == GL_TEXTURE_2D);

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();

    // ensure this is actually a depth texture
    if (!isDepth(0))
    {
        return NULL;
    }
    return mTexStorage->getSurfaceLevel(0, false);
}

TextureStorage *Texture2D::getStorage(bool renderTarget)
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

TextureCubeMap::TextureCubeMap(GLuint id) : Texture(id)
{
    mTexStorage = NULL;
    for (int i = 0; i < 6; i++)
    {
        mFaceProxies[i] = NULL;
        mFaceProxyRefs[i] = 0;
    }
}

TextureCubeMap::~TextureCubeMap()
{
    for (int i = 0; i < 6; i++)
    {
        mFaceProxies[i] = NULL;
    }

    delete mTexStorage;
    mTexStorage = NULL;
}

// We need to maintain a count of references to renderbuffers acting as 
// proxies for this texture, so that the texture is not deleted while 
// proxy references still exist. If the reference count drops to zero,
// we set our proxy pointer NULL, so that a new attempt at referencing
// will cause recreation.
void TextureCubeMap::addProxyRef(const Renderbuffer *proxy)
{
    for (int i = 0; i < 6; i++)
    {
        if (mFaceProxies[i] == proxy)
            mFaceProxyRefs[i]++;
    }
}

void TextureCubeMap::releaseProxy(const Renderbuffer *proxy)
{
    for (int i = 0; i < 6; i++)
    {
        if (mFaceProxies[i] == proxy)
        {
            if (mFaceProxyRefs[i] > 0)
                mFaceProxyRefs[i]--;

            if (mFaceProxyRefs[i] == 0)
                mFaceProxies[i] = NULL;
        }
    }
}

GLenum TextureCubeMap::getTarget() const
{
    return GL_TEXTURE_CUBE_MAP;
}

GLsizei TextureCubeMap::getWidth(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level].getWidth();
    else
        return 0;
}

GLsizei TextureCubeMap::getHeight(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level].getHeight();
    else
        return 0;
}

GLenum TextureCubeMap::getInternalFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level].getInternalFormat();
    else
        return GL_NONE;
}

GLenum TextureCubeMap::getActualFormat(GLenum target, GLint level) const
{
    if (level < IMPLEMENTATION_MAX_TEXTURE_LEVELS)
        return mImageArray[faceIndex(target)][level].getActualFormat();
    else
        return D3DFMT_UNKNOWN;
}

void TextureCubeMap::setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(0, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(1, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(2, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(3, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(4, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    setImage(5, level, width, height, format, type, unpackAlignment, pixels);
}

void TextureCubeMap::setCompressedImage(GLenum face, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels)
{
    // compressed formats don't have separate sized internal formats-- we can just use the compressed format directly
    redefineImage(faceIndex(face), level, format, width, height);

    Texture::setCompressedImage(imageSize, pixels, &mImageArray[faceIndex(face)][level]);
}

void TextureCubeMap::commitRect(int face, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    ASSERT(mImageArray[face][level].getSurface() != NULL);

    if (level < levelCount())
    {
        IDirect3DSurface9 *destLevel = mTexStorage->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, true);
        ASSERT(destLevel != NULL);

        if (destLevel != NULL)
        {
            Image *image = &mImageArray[face][level];
            image->updateSurface(destLevel, xoffset, yoffset, width, height);

            destLevel->Release();
            image->markClean();
        }
    }
}

void TextureCubeMap::subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    if (Texture::subImage(xoffset, yoffset, width, height, format, type, unpackAlignment, pixels, &mImageArray[faceIndex(target)][level]))
    {
        commitRect(faceIndex(target), level, xoffset, yoffset, width, height);
    }
}

void TextureCubeMap::subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels)
{
    if (Texture::subImageCompressed(xoffset, yoffset, width, height, format, imageSize, pixels, &mImageArray[faceIndex(target)][level]))
    {
        commitRect(faceIndex(target), level, xoffset, yoffset, width, height);
    }
}

// Tests for cube map sampling completeness. [OpenGL ES 2.0.24] section 3.8.2 page 86.
bool TextureCubeMap::isSamplerComplete() const
{
    int size = mImageArray[0][0].getWidth();

    bool mipmapping = isMipmapFiltered();

    if ((gl::ExtractType(getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0)) == GL_FLOAT && !getContext()->supportsFloat32LinearFilter()) ||
        (gl::ExtractType(getInternalFormat(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0) == GL_HALF_FLOAT_OES) && !getContext()->supportsFloat16LinearFilter()))
    {
        if (mSamplerState.magFilter != GL_NEAREST ||
            (mSamplerState.minFilter != GL_NEAREST && mSamplerState.minFilter != GL_NEAREST_MIPMAP_NEAREST))
        {
            return false;
        }
    }

    if (!isPow2(size) && !getContext()->supportsNonPower2Texture())
    {
        if (mSamplerState.wrapS != GL_CLAMP_TO_EDGE || mSamplerState.wrapT != GL_CLAMP_TO_EDGE || mipmapping)
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
    if (mImageArray[0][0].getWidth() <= 0 || mImageArray[0][0].getHeight() != mImageArray[0][0].getWidth())
    {
        return false;
    }

    for (unsigned int face = 1; face < 6; face++)
    {
        if (mImageArray[face][0].getWidth() != mImageArray[0][0].getWidth() ||
            mImageArray[face][0].getWidth() != mImageArray[0][0].getHeight() ||
            mImageArray[face][0].getInternalFormat() != mImageArray[0][0].getInternalFormat())
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

    GLsizei size = mImageArray[0][0].getWidth();

    int q = log2(size);

    for (int face = 0; face < 6; face++)
    {
        for (int level = 1; level <= q; level++)
        {
            if (mImageArray[face][level].getInternalFormat() != mImageArray[0][0].getInternalFormat())
            {
                return false;
            }

            if (mImageArray[face][level].getWidth() != std::max(1, size >> level))
            {
                return false;
            }
        }
    }

    return true;
}

bool TextureCubeMap::isCompressed(GLenum target, GLint level) const
{
    return IsCompressed(getInternalFormat(target, level));
}

IDirect3DBaseTexture9 *TextureCubeMap::getBaseTexture() const
{
    return mTexStorage ? mTexStorage->getBaseTexture() : NULL;
}

// Constructs a native texture resource from the texture images, or returns an existing one
void TextureCubeMap::createTexture()
{
    GLsizei size = mImageArray[0][0].getWidth();

    if (!(size > 0))
        return; // do not attempt to create native textures for nonexistant data

    GLint levels = creationLevels(size);
    GLenum internalformat = mImageArray[0][0].getInternalFormat();

    delete mTexStorage;
    mTexStorage = new TextureStorageCubeMap(levels, internalformat, mUsage, false, size);

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int face = 0; face < 6; face++)
        {
            for (int level = 0; level < levels; level++)
            {
                IDirect3DSurface9 *surface = mTexStorage->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, false);
                mImageArray[face][level].setManagedSurface(surface);
            }
        }
    }

    mDirtyImages = true;
}

void TextureCubeMap::updateTexture()
{
    bool mipmapping = isMipmapFiltered() && isMipmapCubeComplete();

    for (int face = 0; face < 6; face++)
    {
        int levels = (mipmapping ? levelCount() : 1);

        for (int level = 0; level < levels; level++)
        {
            Image *image = &mImageArray[face][level];

            if (image->isDirty())
            {
                commitRect(face, level, 0, 0, image->getWidth(), image->getHeight());
            }
        }
    }
}

void TextureCubeMap::convertToRenderTarget()
{
    TextureStorageCubeMap *newTexStorage = NULL;

    if (mImageArray[0][0].getWidth() != 0)
    {
        GLsizei size = mImageArray[0][0].getWidth();
        GLint levels = creationLevels(size);
        GLenum internalformat = mImageArray[0][0].getInternalFormat();

        newTexStorage = new TextureStorageCubeMap(levels, internalformat, GL_FRAMEBUFFER_ATTACHMENT_ANGLE, true, size);

        if (mTexStorage != NULL)
        {
            int levels = levelCount();
            for (int f = 0; f < 6; f++)
            {
                for (int i = 0; i < levels; i++)
                {
                    IDirect3DSurface9 *source = mTexStorage->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i, false);
                    IDirect3DSurface9 *dest = newTexStorage->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i, true);

                    if (!copyToRenderTarget(dest, source, mTexStorage->isManaged()))
                    {
                       delete newTexStorage;
                       if (source) source->Release();
                       if (dest) dest->Release();
                       return error(GL_OUT_OF_MEMORY);
                    }

                    if (source) source->Release();
                    if (dest) dest->Release();
                }
            }
        }
    }

    delete mTexStorage;
    mTexStorage = newTexStorage;

    mDirtyImages = true;
}

void TextureCubeMap::setImage(int faceIndex, GLint level, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels)
{
    GLint internalformat = ConvertSizedInternalFormat(format, type);
    redefineImage(faceIndex, level, internalformat, width, height);

    Texture::setImage(unpackAlignment, pixels, &mImageArray[faceIndex][level]);
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
    bool redefined = mImageArray[face][level].redefine(internalformat, width, height, false);

    if (mTexStorage && redefined)
    {
        for (int i = 0; i < IMPLEMENTATION_MAX_TEXTURE_LEVELS; i++)
        {
            for (int f = 0; f < 6; f++)
            {
                mImageArray[f][i].markDirty();
            }
        }

        delete mTexStorage;
        mTexStorage = NULL;

        mDirtyImages = true;
    }
}

void TextureCubeMap::copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    IDirect3DSurface9 *renderTarget = source->getRenderTarget();

    if (!renderTarget)
    {
        ERR("Failed to retrieve the render target.");
        return error(GL_OUT_OF_MEMORY);
    }

    unsigned int faceindex = faceIndex(target);
    GLint internalformat = gl::ConvertSizedInternalFormat(format, GL_UNSIGNED_BYTE);
    redefineImage(faceindex, level, internalformat, width, height);

    if (!mImageArray[faceindex][level].isRenderableFormat())
    {
        mImageArray[faceindex][level].copy(0, 0, x, y, width, height, renderTarget);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        mImageArray[faceindex][level].markClean();

        ASSERT(width == height);

        if (width > 0 && level < levelCount())
        {
            RECT sourceRect;
            sourceRect.left = x;
            sourceRect.right = x + width;
            sourceRect.top = y;
            sourceRect.bottom = y + height;

            IDirect3DSurface9 *dest = mTexStorage->getCubeMapSurface(target, level, true);

            if (dest)
            {
                getBlitter()->copy(renderTarget, sourceRect, format, 0, 0, dest);
                dest->Release();
            }
        }
    }

    renderTarget->Release();
}

void TextureCubeMap::copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source)
{
    GLsizei size = mImageArray[faceIndex(target)][level].getWidth();

    if (xoffset + width > size || yoffset + height > size)
    {
        return error(GL_INVALID_VALUE);
    }

    IDirect3DSurface9 *renderTarget = source->getRenderTarget();

    if (!renderTarget)
    {
        ERR("Failed to retrieve the render target.");
        return error(GL_OUT_OF_MEMORY);
    }

    unsigned int faceindex = faceIndex(target);

    if (!mImageArray[faceindex][level].isRenderableFormat() || (!mTexStorage && !isSamplerComplete()))
    {
        mImageArray[faceindex][level].copy(0, 0, x, y, width, height, renderTarget);
        mDirtyImages = true;
    }
    else
    {
        if (!mTexStorage || !mTexStorage->isRenderTarget())
        {
            convertToRenderTarget();
        }
        
        updateTexture();

        if (level < levelCount())
        {
            RECT sourceRect;
            sourceRect.left = x;
            sourceRect.right = x + width;
            sourceRect.top = y;
            sourceRect.bottom = y + height;

            IDirect3DSurface9 *dest = mTexStorage->getCubeMapSurface(target, level, true);

            if (dest)
            {
                getBlitter()->copy(renderTarget, sourceRect, gl::ExtractFormat(mImageArray[0][0].getInternalFormat()), xoffset, yoffset, dest);
                dest->Release();
            }
        }
    }

    renderTarget->Release();
}

void TextureCubeMap::storage(GLsizei levels, GLenum internalformat, GLsizei size)
{
    delete mTexStorage;
    mTexStorage = new TextureStorageCubeMap(levels, internalformat, mUsage, false, size);
    mImmutable = true;

    for (int level = 0; level < levels; level++)
    {
        for (int face = 0; face < 6; face++)
        {
            mImageArray[face][level].redefine(internalformat, size, size, true);
            size = std::max(1, size >> 1);
        }
    }

    for (int level = levels; level < IMPLEMENTATION_MAX_TEXTURE_LEVELS; level++)
    {
        for (int face = 0; face < 6; face++)
        {
            mImageArray[face][level].redefine(GL_NONE, 0, 0, true);
        }
    }

    if (mTexStorage->isManaged())
    {
        int levels = levelCount();

        for (int face = 0; face < 6; face++)
        {
            for (int level = 0; level < levels; level++)
            {
                IDirect3DSurface9 *surface = mTexStorage->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, false);
                mImageArray[face][level].setManagedSurface(surface);
            }
        }
    }
}

void TextureCubeMap::generateMipmaps()
{
    if (!isCubeComplete())
    {
        return error(GL_INVALID_OPERATION);
    }

    if (!getContext()->supportsNonPower2Texture())
    {
        if (!isPow2(mImageArray[0][0].getWidth()))
        {
            return error(GL_INVALID_OPERATION);
        }
    }

    // Purge array levels 1 through q and reset them to represent the generated mipmap levels.
    unsigned int q = log2(mImageArray[0][0].getWidth());
    for (unsigned int f = 0; f < 6; f++)
    {
        for (unsigned int i = 1; i <= q; i++)
        {
            redefineImage(f, i, mImageArray[f][0].getInternalFormat(),
                                std::max(mImageArray[f][0].getWidth() >> i, 1),
                                std::max(mImageArray[f][0].getWidth() >> i, 1));
        }
    }

    if (mTexStorage && mTexStorage->isRenderTarget())
    {
        for (unsigned int f = 0; f < 6; f++)
        {
            for (unsigned int i = 1; i <= q; i++)
            {
                IDirect3DSurface9 *upper = mTexStorage->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i - 1, false);
                IDirect3DSurface9 *lower = mTexStorage->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i, true);

                if (upper != NULL && lower != NULL)
                {
                    getBlitter()->boxFilter(upper, lower);
                }

                if (upper != NULL) upper->Release();
                if (lower != NULL) lower->Release();

                mImageArray[f][i].markClean();
            }
        }
    }
    else
    {
        for (unsigned int f = 0; f < 6; f++)
        {
            for (unsigned int i = 1; i <= q; i++)
            {
                if (mImageArray[f][i].getSurface() == NULL)
                {
                    return error(GL_OUT_OF_MEMORY);
                }

                GenerateMip(mImageArray[f][i].getSurface(), mImageArray[f][i - 1].getSurface());

                mImageArray[f][i].markDirty();
            }
        }
    }
}

Renderbuffer *TextureCubeMap::getRenderbuffer(GLenum target)
{
    if (!IsCubemapTextureTarget(target))
    {
        return error(GL_INVALID_OPERATION, (Renderbuffer *)NULL);
    }

    unsigned int face = faceIndex(target);

    if (mFaceProxies[face] == NULL)
    {
        mFaceProxies[face] = new Renderbuffer(id(), new RenderbufferTextureCubeMap(this, target));
    }

    return mFaceProxies[face];
}

// Increments refcount on surface.
// caller must Release() the returned surface
IDirect3DSurface9 *TextureCubeMap::getRenderTarget(GLenum target)
{
    ASSERT(IsCubemapTextureTarget(target));

    // ensure the underlying texture is created
    if (getStorage(true) == NULL)
    {
        return NULL;
    }

    updateTexture();
    
    return mTexStorage->getCubeMapSurface(target, 0, false);
}

TextureStorage *TextureCubeMap::getStorage(bool renderTarget)
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

}
