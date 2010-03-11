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

GLenum Texture::getMinFilter()
{
    return mMinFilter;
}

GLenum Texture::getMagFilter()
{
    return mMagFilter;
}

GLenum Texture::getWrapS()
{
    return mWrapS;
}

GLenum Texture::getWrapT()
{
    return mWrapT;
}

// Copies an Image into an already locked Direct3D 9 surface, performing format conversions as necessary
void Texture::copyImage(D3DLOCKED_RECT &lock, D3DFORMAT format, Image &image)
{
    if (lock.pBits && image.pixels)
    {
        for (int y = 0; y < image.height; y++)
        {
            unsigned char *source = (unsigned char*)image.pixels + y * image.width * pixelSize(image);
            unsigned short *source16 = (unsigned short*)source;
            unsigned char *dest = (unsigned char*)lock.pBits + y * lock.Pitch;

            for (int x = 0; x < image.width; x++)
            {
                unsigned char r;
                unsigned char g;
                unsigned char b;
                unsigned char a;

                switch (image.format)
                {
                  case GL_ALPHA:
                    UNIMPLEMENTED();
                    break;
                  case GL_LUMINANCE:
                    UNIMPLEMENTED();
                    break;
                  case GL_LUMINANCE_ALPHA:
                    UNIMPLEMENTED();
                    break;
                  case GL_RGB:
                    switch (image.type)
                    {
                      case GL_UNSIGNED_BYTE:          UNIMPLEMENTED(); break;
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
                    switch (image.type)
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
                      case GL_UNSIGNED_SHORT_5_5_5_1:    UNIMPLEMENTED();            break;
                      default: UNREACHABLE();
                    }
                    break;
                  default: UNREACHABLE();
                }

                switch (format)
                {
                  case D3DFMT_A8R8G8B8:
                    dest[4 * x + 0] = b;
                    dest[4 * x + 1] = g;
                    dest[4 * x + 2] = r;
                    dest[4 * x + 3] = a;
                    break;
                  default: UNREACHABLE();
                }
            }
        }
    }
}

// Selects an internal Direct3D 9 format for storing an Image
D3DFORMAT Texture::selectFormat(const Image &image)
{
    switch (image.format)
    {
      case GL_ALPHA:
        UNIMPLEMENTED();
        break;
      case GL_LUMINANCE:
        UNIMPLEMENTED();
        break;
      case GL_LUMINANCE_ALPHA:
        UNIMPLEMENTED();
        break;
      case GL_RGB:
        switch (image.type)
        {
          case GL_UNSIGNED_BYTE:        UNIMPLEMENTED();
          case GL_UNSIGNED_SHORT_5_6_5: return D3DFMT_A8R8G8B8;
          default: UNREACHABLE();
        }
        break;
      case GL_RGBA:
        switch (image.type)
        {
          case GL_UNSIGNED_BYTE:          return D3DFMT_A8R8G8B8;
          case GL_UNSIGNED_SHORT_4_4_4_4: return D3DFMT_A8R8G8B8;
          case GL_UNSIGNED_SHORT_5_5_5_1: UNIMPLEMENTED();
          default: UNREACHABLE();
        }
        break;
      default: UNREACHABLE();
    }

    return D3DFMT_UNKNOWN;
}

// Returns the size, in bytes, of a single texel in an Image
int Texture::pixelSize(const Image &image)
{
    switch (image.type)
    {
      case GL_UNSIGNED_BYTE:
        switch (image.format)
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

Texture2D::Texture2D()
{
    for (int level = 0; level < MAX_TEXTURE_LEVELS; level++)
    {
        mImageArray[level].pixels = NULL;
    }

    mTexture = NULL;
}

Texture2D::~Texture2D()
{
    for (int level = 0; level < MAX_TEXTURE_LEVELS; level++)
    {
        delete[] mImageArray[level].pixels;
    }

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
    if (level < 0 || level >= MAX_TEXTURE_LEVELS)
    {
        return;
    }

    mImageArray[level].internalFormat = internalFormat;
    mImageArray[level].width = width;
    mImageArray[level].height = height;
    mImageArray[level].format = format;
    mImageArray[level].type = type;

    delete[] mImageArray[level].pixels;
    mImageArray[level].pixels = NULL;

    int imageSize = pixelSize(mImageArray[level]) * width * height;
    mImageArray[level].pixels = new unsigned char[imageSize];

    if (pixels)
    {
        memcpy(mImageArray[level].pixels, pixels, imageSize);
    }

    if (level == 0)
    {
        mWidth = width;
        mHeight = height;
    }
}

// Tests for GL texture object completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool Texture2D::isComplete()
{
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

            if (mImageArray[level].internalFormat != mImageArray[0].internalFormat)
            {
                return false;
            }

            if (mImageArray[level].type != mImageArray[0].type)
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
IDirect3DBaseTexture9 *Texture2D::getTexture()
{
    if (!isComplete())
    {
        return NULL;
    }

    if (!mTexture)   // FIXME: Recreate when changed (same for getRenderTarget)
    {
        IDirect3DDevice9 *device = getDevice();
        D3DFORMAT format = selectFormat(mImageArray[0]);

        HRESULT result = device->CreateTexture(mWidth, mHeight, 0, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &mTexture, NULL);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            error(GL_OUT_OF_MEMORY, 0);
        }

        ASSERT(SUCCEEDED(result));

        IDirect3DTexture9 *lockableTexture;
        result = device->CreateTexture(mWidth, mHeight, 0, D3DUSAGE_DYNAMIC, format, D3DPOOL_SYSTEMMEM, &lockableTexture, NULL);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            error(GL_OUT_OF_MEMORY,(IDirect3DBaseTexture9*)NULL);
        }

        ASSERT(SUCCEEDED(result));

        if (mTexture)   // FIXME: Handle failure
        {
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
    }

    return mTexture;
}

// Returns the top-level texture surface as a render target
IDirect3DSurface9 *Texture2D::getRenderTarget()
{
    if (!mRenderTarget && getTexture())
    {
        mTexture->GetSurfaceLevel(0, &mRenderTarget);    
    }

    return mRenderTarget;
}

TextureCubeMap::TextureCubeMap()
{
    for (int face = 0; face < 6; face++)
    {
        for (int level = 0; level < MAX_TEXTURE_LEVELS; level++)
        {
            mImageArray[face][level].pixels = NULL;
        }
    }

    mTexture = NULL;
}

TextureCubeMap::~TextureCubeMap()
{
    for (int face = 0; face < 6; face++)
    {
        for (int level = 0; level < MAX_TEXTURE_LEVELS; level++)
        {
            delete[] mImageArray[face][level].pixels;
        }
    }

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

// Tests for GL texture object completeness. [OpenGL ES 2.0.24] section 3.7.10 page 81.
bool TextureCubeMap::isComplete()
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

                if (mImageArray[face][level].internalFormat != mImageArray[0][0].internalFormat)
                {
                    return false;
                }

                if (mImageArray[face][level].type != mImageArray[0][0].type)
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
IDirect3DBaseTexture9 *TextureCubeMap::getTexture()
{
    if (!isComplete())
    {
        return NULL;
    }

    if (!mTexture)   // FIXME: Recreate when changed (same for getRenderTarget)
    {
        IDirect3DDevice9 *device = getDevice();
        D3DFORMAT format = selectFormat(mImageArray[0][0]);

        HRESULT result = device->CreateCubeTexture(mWidth, 0, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &mTexture, NULL);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            error(GL_OUT_OF_MEMORY, (IDirect3DBaseTexture9*)NULL);
        }

        ASSERT(SUCCEEDED(result));

        IDirect3DCubeTexture9 *lockableTexture;
        result = device->CreateCubeTexture(mWidth, 0, D3DUSAGE_DYNAMIC, format, D3DPOOL_SYSTEMMEM, &lockableTexture, NULL);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            return error(GL_OUT_OF_MEMORY, (IDirect3DBaseTexture9*)NULL);
        }

        ASSERT(SUCCEEDED(result));

        if (mTexture)
        {
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
    }

    return mTexture;
}

void TextureCubeMap::setImage(int face, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels)
{
    if (level < 0 || level >= MAX_TEXTURE_LEVELS)
    {
        return;
    }

    mImageArray[face][level].internalFormat = internalFormat;
    mImageArray[face][level].width = width;
    mImageArray[face][level].height = height;
    mImageArray[face][level].format = format;
    mImageArray[face][level].type = type;

    delete[] mImageArray[face][level].pixels;
    mImageArray[face][level].pixels = NULL;

    int imageSize = pixelSize(mImageArray[face][level]) * width * height;
    mImageArray[face][level].pixels = new unsigned char[imageSize];

    if (pixels)
    {
        memcpy(mImageArray[face][level].pixels, pixels, imageSize);
    }

    if (face == 0 && level == 0)
    {
        mWidth = width;
        mHeight = height;
    }
}
}
