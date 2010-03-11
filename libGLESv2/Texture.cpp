//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.cpp: Implements the gl::Texture class and its derived classes
// Texture2D and TextureCubeMap. Implements GL texture objects and related
// functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#include "Texture.h"

#include <algorithm>

#include "main.h"
#include "mathutil.h"
#include "debug.h"

namespace gl
{
Texture::Texture() : Colorbuffer(0)
{
    mMinFilter = GL_NEAREST_MIPMAP_LINEAR;
    mMagFilter = GL_LINEAR;
    mWrapS = GL_REPEAT;
    mWrapT = GL_REPEAT;

    mDirtyImageData = true;
    mDirtyMetaData = true;
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
        mMinFilter = filter;
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
        mMagFilter = filter;
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
        mWrapS = wrap;
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
        mWrapT = wrap;
        return true;
      default:
        return false;
    }
}

GLenum Texture::getMinFilter() const
{
    return mMinFilter;
}

GLenum Texture::getMagFilter() const
{
    return mMagFilter;
}

GLenum Texture::getWrapS() const
{
    return mWrapS;
}

GLenum Texture::getWrapT() const
{
    return mWrapT;
}

// Copies an Image into an already locked Direct3D 9 surface, performing format conversions as necessary
void Texture::copyImage(const D3DLOCKED_RECT &lock, D3DFORMAT format, const Image &image)
{
    ASSERT(format == D3DFMT_A8R8G8B8);

    std::size_t sourcePitch = imagePitch(image);

    if (lock.pBits && !image.pixels.empty())
    {
        if (lock.Pitch == sourcePitch)
        {
            memcpy(lock.pBits, &image.pixels[0], lock.Pitch * image.height);
        }
        else
        {
            for (int y = 0; y < image.height; y++)
            {
                memcpy(static_cast<unsigned char*>(lock.pBits) + y * lock.Pitch, &image.pixels[0] + y * sourcePitch, sourcePitch);
            }
        }
    }
}

// Selects an internal Direct3D 9 format for storing an Image
D3DFORMAT Texture::selectFormat(const Image &image)
{
    return D3DFMT_A8R8G8B8;
}

// Returns the size, in bytes, of a single texel in an Image
int Texture::pixelSize(GLenum format, GLenum type)
{
    switch (type)
    {
      case GL_UNSIGNED_BYTE:
        switch (format)
        {
          case GL_ALPHA:           return sizeof(unsigned char);
          case GL_LUMINANCE:       return sizeof(unsigned char);
          case GL_LUMINANCE_ALPHA: return sizeof(unsigned char) * 2;
          case GL_RGB:             return sizeof(unsigned char) * 3;
          case GL_RGBA:            return sizeof(unsigned char) * 4;
          default: UNREACHABLE();
        }
        break;
      case GL_UNSIGNED_SHORT_4_4_4_4:
      case GL_UNSIGNED_SHORT_5_5_5_1:
      case GL_UNSIGNED_SHORT_5_6_5:
        return sizeof(unsigned short);
      default: UNREACHABLE();
    }

    return 0;
}

int Texture::imagePitch(const Image &img) const
{
    return img.width * 4;
}

// Store the pixel rectangle designated by xoffset,yoffset,width,height with pixels stored as format/type at input
// into the BGRA8 pixel rectangle at output with outputPitch bytes in between each line.
void Texture::loadImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type,
                            const void *input, size_t outputPitch, void *output) const
{
    size_t inputPitch = width * pixelSize(format, type);

    for (int y = 0; y < height; y++)
    {
        const unsigned char *source = static_cast<const unsigned char*>(input) + y * inputPitch;
        const unsigned short *source16 = reinterpret_cast<const unsigned short*>(source);
        unsigned char *dest = static_cast<unsigned char*>(output) + (y + yoffset) * outputPitch + xoffset * 4;

        for (int x = 0; x < width; x++)
        {
            unsigned char r;
            unsigned char g;
            unsigned char b;
            unsigned char a;

            switch (format)
            {
              case GL_ALPHA:
                a = source[x];
                r = 0;
                g = 0;
                b = 0;
                break;

              case GL_LUMINANCE:
                r = source[x];
                g = source[x];
                b = source[x];
                a = 0xFF;
                break;

              case GL_LUMINANCE_ALPHA:
                r = source[2*x+0];
                g = source[2*x+0];
                b = source[2*x+0];
                a = source[2*x+1];
                break;

              case GL_RGB:
                switch (type)
                {
                  case GL_UNSIGNED_BYTE:
                    r = source[x * 3 + 0];
                    b = source[x * 3 + 1];
                    g = source[x * 3 + 2];
                    a = 0xFF;
                    break;

                  case GL_UNSIGNED_SHORT_5_6_5:
                    {
                        unsigned short rgba = source16[x];

                        a = 0xFF;
                        b = ((rgba & 0x001F) << 3) | ((rgba & 0x001F) >> 2);
                        g = ((rgba & 0x07E0) >> 3) | ((rgba & 0x07E0) >> 9);
                        r = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
                    }
                    break;

                  default: UNREACHABLE();
                }
                break;

              case GL_RGBA:
                switch (type)
                {
                  case GL_UNSIGNED_BYTE:
                    r = source[x * 4 + 0];
                    g = source[x * 4 + 1];
                    b = source[x * 4 + 2];
                    a = source[x * 4 + 3];
                    break;

                  case GL_UNSIGNED_SHORT_4_4_4_4:
                    {
                        unsigned short rgba = source16[x];

                        a = ((rgba & 0x000F) << 4) | ((rgba & 0x000F) >> 0);
                        b = ((rgba & 0x00F0) << 0) | ((rgba & 0x00F0) >> 4);
                        g = ((rgba & 0x0F00) >> 4) | ((rgba & 0x0F00) >> 8);
                        r = ((rgba & 0xF000) >> 8) | ((rgba & 0xF000) >> 12);
                    }
                    break;

                  case GL_UNSIGNED_SHORT_5_5_5_1:
                    {
                        unsigned short rgba = source16[x];

                        a = (rgba & 0x0001) ? 0xFF : 0;
                        b = ((rgba & 0x003E) << 2) | ((rgba & 0x903E) >> 3);
                        g = ((rgba & 0x07C0) >> 3) | ((rgba & 0x07C0) >> 8);
                        r = ((rgba & 0xF800) >> 8) | ((rgba & 0x07C0) >> 13);
                    }
                    break;

                  default: UNREACHABLE();
                }
                break;
              default: UNREACHABLE();
            }

            dest[4 * x + 0] = b;
            dest[4 * x + 1] = g;
            dest[4 * x + 2] = r;
            dest[4 * x + 3] = a;
        }
    }
}

void Texture::setImage(GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels, Image *img)
{
    img->width = width;
    img->height = height;
    img->format = format;

    size_t imageSize = imagePitch(*img) * img->height;

    std::vector<unsigned char> storage(imageSize);

    if (pixels)
    {
        loadImageData(0, 0, width, height, format, type, pixels, imagePitch(*img), &storage[0]);
    }

    img->pixels.swap(storage);

    mDirtyImageData = true;
    mDirtyMetaData = true;
}

void Texture::subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels, Image *img)
{
    if (width + xoffset > img->width || height + yoffset > img->height) return error(GL_INVALID_VALUE);

    loadImageData(xoffset, yoffset, width, height, format, type, pixels, imagePitch(*img), &img->pixels[0]);

    mDirtyImageData = true;
}

IDirect3DBaseTexture9 *Texture::getTexture()
{
    if (!isComplete())
    {
        return NULL;
    }

    if (mDirtyMetaData)
    {
        ASSERT(mDirtyImageData);

        mBaseTexture = createTexture();
    }

    if (mDirtyImageData)
    {
        updateTexture();
    }

    mDirtyMetaData = false;
    mDirtyImageData = false;

    return mBaseTexture;
}

Texture2D::Texture2D()
{
    mTexture = NULL;
}

Texture2D::~Texture2D()
{
    if (mTexture)
    {
        mTexture->Release();
        mTexture = NULL;
    }
}

GLenum Texture2D::getTarget() const
{
    return GL_TEXTURE_2D;
}

void Texture2D::setImage(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    Texture::setImage(width, height, format, type, pixels, &mImageArray[level]);

    if (level == 0)
    {
        mWidth = width;
        mHeight = height;
    }
}

void Texture2D::subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    Texture::subImage(xoffset, yoffset, width, height, format, type, pixels, &mImageArray[level]);
}

// Tests for GL texture object completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isComplete() const
{
    ASSERT(mWidth == mImageArray[0].width && mHeight == mImageArray[0].height);

    if (mWidth <= 0 || mHeight <= 0)
    {
        return false;
    }

    bool mipmapping;

    switch (mMagFilter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
        mipmapping = false;
        break;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        mipmapping = true;
        break;
     default: UNREACHABLE();
    }

    if (mipmapping)
    {
        int q = log2(std::max(mWidth, mHeight));

        for (int level = 1; level <= q; level++)
        {
            if (mImageArray[level].format != mImageArray[0].format)
            {
                return false;
            }

            if (mImageArray[level].width != (mImageArray[level - 1].width + 1) / 2)
            {
                return false;
            }

            if (mImageArray[level].height != (mImageArray[level - 1].height + 1) / 2)
            {
                return false;
            }
        }
    }

    return true;
}

// Constructs a Direct3D 9 texture resource from the texture images, or returns an existing one
IDirect3DBaseTexture9 *Texture2D::createTexture()
{
    IDirect3DTexture9 *texture;

    IDirect3DDevice9 *device = getDevice();
    D3DFORMAT format = selectFormat(mImageArray[0]);

    HRESULT result = device->CreateTexture(mWidth, mHeight, 0, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &texture, NULL);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY, (IDirect3DBaseTexture9*)NULL);
    }

    if (mTexture) mTexture->Release();
    mTexture = texture;
    return texture;
}

void Texture2D::updateTexture()
{
    IDirect3DDevice9 *device = getDevice();
    D3DFORMAT format = selectFormat(mImageArray[0]);

    IDirect3DTexture9 *lockableTexture;
    HRESULT result = device->CreateTexture(mWidth, mHeight, 0, D3DUSAGE_DYNAMIC, format, D3DPOOL_SYSTEMMEM, &lockableTexture, NULL);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY);
    }

    int levelCount = mTexture->GetLevelCount();

    for (int level = 0; level < levelCount; level++)
    {
        D3DLOCKED_RECT lock = {0};
        lockableTexture->LockRect(level, &lock, NULL, 0);

        copyImage(lock, format, mImageArray[level]);

        lockableTexture->UnlockRect(level);
    }

    device->UpdateTexture(lockableTexture, mTexture);
    lockableTexture->Release();
}

// Returns the top-level texture surface as a render target
IDirect3DSurface9 *Texture2D::getRenderTarget()
{
    if (mDirtyMetaData && mRenderTarget)
    {
        mRenderTarget->Release();
        mRenderTarget = NULL;
    }

    if (!mRenderTarget && getTexture()) // FIXME: getTexture fails for incomplete textures. Check spec.
    {
        mTexture->GetSurfaceLevel(0, &mRenderTarget);
    }

    return mRenderTarget;
}

TextureCubeMap::TextureCubeMap()
{
    mTexture = NULL;
}

TextureCubeMap::~TextureCubeMap()
{
    if (mTexture)
    {
        mTexture->Release();
        mTexture = NULL;
    }
}

GLenum TextureCubeMap::getTarget() const
{
    return GL_TEXTURE_CUBE_MAP;
}

void TextureCubeMap::setImagePosX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    setImage(0, level, internalFormat, width, height, format, type, pixels);
}

void TextureCubeMap::setImageNegX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    setImage(1, level, internalFormat, width, height, format, type, pixels);
}

void TextureCubeMap::setImagePosY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    setImage(2, level, internalFormat, width, height, format, type, pixels);
}

void TextureCubeMap::setImageNegY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    setImage(3, level, internalFormat, width, height, format, type, pixels);
}

void TextureCubeMap::setImagePosZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    setImage(4, level, internalFormat, width, height, format, type, pixels);
}

void TextureCubeMap::setImageNegZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    setImage(5, level, internalFormat, width, height, format, type, pixels);
}

void TextureCubeMap::subImage(GLenum face, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    Texture::subImage(xoffset, yoffset, width, height, format, type, pixels, &mImageArray[faceIndex(face)][level]);
}

// Tests for GL texture object completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isComplete() const
{
    if (mWidth <= 0 || mHeight <= 0 || mWidth != mHeight)
    {
        return false;
    }

    bool mipmapping;

    switch (mMagFilter)
    {
      case GL_NEAREST:
      case GL_LINEAR:
        mipmapping = false;
        break;
      case GL_NEAREST_MIPMAP_NEAREST:
      case GL_LINEAR_MIPMAP_NEAREST:
      case GL_NEAREST_MIPMAP_LINEAR:
      case GL_LINEAR_MIPMAP_LINEAR:
        mipmapping = true;
        break;
      default: UNREACHABLE();
    }

    for (int face = 0; face < 6; face++)
    {
        if (mImageArray[face][0].width != mWidth || mImageArray[face][0].height != mHeight)
        {
            return false;
        }
    }

    if (mipmapping)
    {
        int q = log2(mWidth);

        for (int face = 0; face < 6; face++)
        {
            for (int level = 1; level <= q; level++)
            {
                if (mImageArray[face][level].format != mImageArray[0][0].format)
                {
                    return false;
                }

                if (mImageArray[face][level].width != (mImageArray[0][level - 1].width + 1) / 2)
                {
                    return false;
                }

                if (mImageArray[face][level].height != (mImageArray[0][level - 1].height + 1) / 2)
                {
                    return false;
                }
            }
        }
    }

    return true;
}

// Constructs a Direct3D 9 texture resource from the texture images, or returns an existing one
IDirect3DBaseTexture9 *TextureCubeMap::createTexture()
{
    IDirect3DDevice9 *device = getDevice();
    D3DFORMAT format = selectFormat(mImageArray[0][0]);

    IDirect3DCubeTexture9 *texture;

    HRESULT result = device->CreateCubeTexture(mWidth, 0, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &texture, NULL);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY, (IDirect3DBaseTexture9*)NULL);
    }

    if (mTexture) mTexture->Release();

    mTexture = texture;
    return mTexture;
}

void TextureCubeMap::updateTexture()
{
    IDirect3DDevice9 *device = getDevice();
    D3DFORMAT format = selectFormat(mImageArray[0][0]);

    IDirect3DCubeTexture9 *lockableTexture;
    HRESULT result = device->CreateCubeTexture(mWidth, 0, D3DUSAGE_DYNAMIC, format, D3DPOOL_SYSTEMMEM, &lockableTexture, NULL);

    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
    {
        return error(GL_OUT_OF_MEMORY);
    }

    ASSERT(SUCCEEDED(result));

    for (int face = 0; face < 6; face++)
    {
        for (int level = 0; level < MAX_TEXTURE_LEVELS; level++)
        {
            D3DLOCKED_RECT lock = {0};
            lockableTexture->LockRect((D3DCUBEMAP_FACES)face, level, &lock, NULL, 0);

            copyImage(lock, format, mImageArray[face][level]);

            lockableTexture->UnlockRect((D3DCUBEMAP_FACES)face, level);
        }
    }

    device->UpdateTexture(lockableTexture, mTexture);
    lockableTexture->Release();
}

void TextureCubeMap::setImage(int face, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    Texture::setImage(width, height, format, type, pixels, &mImageArray[face][level]);

    if (face == 0 && level == 0)
    {
        mWidth = width;
        mHeight = height;
    }
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

}
