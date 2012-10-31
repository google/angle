//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.cpp: Implements the gl::Image class, which acts as the interface to
// the actual underlying surfaces of a Texture.

#include "libGLESv2/renderer/Image.h"

#include "libEGL/Display.h"

#include "libGLESv2/main.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/Texture.h"

namespace gl
{
Image::Image()
{
    mWidth = 0; 
    mHeight = 0;
    mInternalFormat = GL_NONE;

    mSurface = NULL;

    mDirty = false;

    mD3DPool = D3DPOOL_SYSTEMMEM;
    mD3DFormat = D3DFMT_UNKNOWN;
    mActualFormat = GL_NONE;
}

Image::~Image()
{
    if (mSurface)
    {
        mSurface->Release();
    }
}

void Image::CopyLockableSurfaces(IDirect3DSurface9 *dest, IDirect3DSurface9 *source)
{
    D3DLOCKED_RECT sourceLock = {0};
    D3DLOCKED_RECT destLock = {0};
    
    source->LockRect(&sourceLock, NULL, 0);
    dest->LockRect(&destLock, NULL, 0);
    
    if (sourceLock.pBits && destLock.pBits)
    {
        D3DSURFACE_DESC desc;
        source->GetDesc(&desc);

        int rows = dx::IsCompressedFormat(desc.Format) ? desc.Height / 4 : desc.Height;
        int bytes = dx::ComputeRowSize(desc.Format, desc.Width);
        ASSERT(bytes <= sourceLock.Pitch && bytes <= destLock.Pitch);

        for(int i = 0; i < rows; i++)
        {
            memcpy((char*)destLock.pBits + destLock.Pitch * i, (char*)sourceLock.pBits + sourceLock.Pitch * i, bytes);
        }

        source->UnlockRect();
        dest->UnlockRect();
    }
    else UNREACHABLE();
}

bool Image::redefine(GLint internalformat, GLsizei width, GLsizei height, bool forceRelease)
{
    if (mWidth != width ||
        mHeight != height ||
        mInternalFormat != internalformat ||
        forceRelease)
    {
        mWidth = width;
        mHeight = height;
        mInternalFormat = internalformat;
        // compute the d3d format that will be used
        mD3DFormat = Texture::ConvertTextureInternalFormat(internalformat);
        mActualFormat = dx2es::GetEquivalentFormat(mD3DFormat);

        if (mSurface)
        {
            mSurface->Release();
            mSurface = NULL;
        }

        return true;
    }

    return false;
}

void Image::createSurface()
{
    if(mSurface)
    {
        return;
    }

    IDirect3DTexture9 *newTexture = NULL;
    IDirect3DSurface9 *newSurface = NULL;
    const D3DPOOL poolToUse = D3DPOOL_SYSTEMMEM;
    const D3DFORMAT d3dFormat = getD3DFormat();
    ASSERT(d3dFormat != D3DFMT_INTZ); // We should never get here for depth textures

    if (mWidth != 0 && mHeight != 0)
    {
        int levelToFetch = 0;
        GLsizei requestWidth = mWidth;
        GLsizei requestHeight = mHeight;
        MakeValidSize(true, IsCompressed(mInternalFormat), &requestWidth, &requestHeight, &levelToFetch);

        // D3D9_REPLACE
        IDirect3DDevice9 *device = getDisplay()->getRenderer()->getDevice();
        HRESULT result = device->CreateTexture(requestWidth, requestHeight, levelToFetch + 1, NULL, d3dFormat,
                                                    poolToUse, &newTexture, NULL);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            ERR("Creating image surface failed.");
            return error(GL_OUT_OF_MEMORY);
        }

        newTexture->GetSurfaceLevel(levelToFetch, &newSurface);
        newTexture->Release();
    }

    mSurface = newSurface;
    mDirty = false;
    mD3DPool = poolToUse;
}

HRESULT Image::lock(D3DLOCKED_RECT *lockedRect, const RECT *rect)
{
    createSurface();

    HRESULT result = D3DERR_INVALIDCALL;

    if (mSurface)
    {
        result = mSurface->LockRect(lockedRect, rect, 0);
        ASSERT(SUCCEEDED(result));

        mDirty = true;
    }

    return result;
}

void Image::unlock()
{
    if (mSurface)
    {
        HRESULT result = mSurface->UnlockRect();
        ASSERT(SUCCEEDED(result));
    }
}

bool Image::isRenderableFormat() const
{    
    return Texture::IsTextureFormatRenderable(getD3DFormat());
}

GLenum Image::getActualFormat() const
{
    return mActualFormat;
}

D3DFORMAT Image::getD3DFormat() const
{
    // this should only happen if the image hasn't been redefined first
    // which would be a bug by the caller
    ASSERT(mD3DFormat != D3DFMT_UNKNOWN);

    return mD3DFormat;
}

IDirect3DSurface9 *Image::getSurface()
{
    createSurface();

    return mSurface;
}

void Image::setManagedSurface(IDirect3DSurface9 *surface)
{
    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);
    ASSERT(desc.Pool == D3DPOOL_MANAGED);

    if ((GLsizei)desc.Width == mWidth && (GLsizei)desc.Height == mHeight)
    {
        if (mSurface)
        {
            CopyLockableSurfaces(surface, mSurface);
            mSurface->Release();
        }

        mSurface = surface;
        mD3DPool = desc.Pool;
    }
}

void Image::updateSurface(IDirect3DSurface9 *destSurface, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    IDirect3DSurface9 *sourceSurface = getSurface();

    if (sourceSurface && sourceSurface != destSurface)
    {
        RECT rect;
        rect.left = xoffset;
        rect.top = yoffset;
        rect.right = xoffset + width;
        rect.bottom = yoffset + height;

        POINT point = {rect.left, rect.top};
        IDirect3DDevice9 *device = getDisplay()->getRenderer()->getDevice();  // D3D9_REPLACE

        if (mD3DPool == D3DPOOL_MANAGED)
        {
            D3DSURFACE_DESC desc;
            sourceSurface->GetDesc(&desc);

            IDirect3DSurface9 *surf = 0;
            HRESULT result = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &surf, NULL);

            if (SUCCEEDED(result))
            {
                CopyLockableSurfaces(surf, sourceSurface);
                result = device->UpdateSurface(surf, &rect, destSurface, &point);
                ASSERT(SUCCEEDED(result));
                surf->Release();
            }
        }
        else
        {
            // UpdateSurface: source must be SYSTEMMEM, dest must be DEFAULT pools 
            HRESULT result = device->UpdateSurface(sourceSurface, &rect, destSurface, &point);
            ASSERT(SUCCEEDED(result));
        }
    }
}

// Store the pixel rectangle designated by xoffset,yoffset,width,height with pixels stored as format/type at input
// into the target pixel rectangle.
void Image::loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                     GLint unpackAlignment, const void *input)
{
    RECT lockRect =
    {
        xoffset, yoffset,
        xoffset + width, yoffset + height
    };

    D3DLOCKED_RECT locked;
    HRESULT result = lock(&locked, &lockRect);
    if (FAILED(result))
    {
        return;
    }


    GLsizei inputPitch = ComputePitch(width, mInternalFormat, unpackAlignment);

    switch (mInternalFormat)
    {
      case GL_ALPHA8_EXT:
        if (supportsSSE2())
        {
            loadAlphaDataSSE2(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        }
        else
        {
            loadAlphaData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        }
        break;
      case GL_LUMINANCE8_EXT:
        loadLuminanceData(width, height, inputPitch, input, locked.Pitch, locked.pBits, getD3DFormat() == D3DFMT_L8);
        break;
      case GL_ALPHA32F_EXT:
        loadAlphaFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_LUMINANCE32F_EXT:
        loadLuminanceFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_ALPHA16F_EXT:
        loadAlphaHalfFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_LUMINANCE16F_EXT:
        loadLuminanceHalfFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_LUMINANCE8_ALPHA8_EXT:
        loadLuminanceAlphaData(width, height, inputPitch, input, locked.Pitch, locked.pBits, getD3DFormat() == D3DFMT_A8L8);
        break;
      case GL_LUMINANCE_ALPHA32F_EXT:
        loadLuminanceAlphaFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_LUMINANCE_ALPHA16F_EXT:
        loadLuminanceAlphaHalfFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_RGB8_OES:
        loadRGBUByteData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_RGB565:
        loadRGB565Data(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_RGBA8_OES:
        if (supportsSSE2())
        {
            loadRGBAUByteDataSSE2(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        }
        else
        {
            loadRGBAUByteData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        }
        break;
      case GL_RGBA4:
        loadRGBA4444Data(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_RGB5_A1:
        loadRGBA5551Data(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_BGRA8_EXT:
        loadBGRAData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      // float textures are converted to RGBA, not BGRA, as they're stored that way in D3D
      case GL_RGB32F_EXT:
        loadRGBFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_RGB16F_EXT:
        loadRGBHalfFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_RGBA32F_EXT:
        loadRGBAFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      case GL_RGBA16F_EXT:
        loadRGBAHalfFloatData(width, height, inputPitch, input, locked.Pitch, locked.pBits);
        break;
      default: UNREACHABLE(); 
    }

    unlock();
}

void Image::loadAlphaData(GLsizei width, GLsizei height,
                          int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;
    
    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = 0;
            dest[4 * x + 1] = 0;
            dest[4 * x + 2] = 0;
            dest[4 * x + 3] = source[x];
        }
    }
}

void Image::loadAlphaFloatData(GLsizei width, GLsizei height,
                               int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = 0;
            dest[4 * x + 1] = 0;
            dest[4 * x + 2] = 0;
            dest[4 * x + 3] = source[x];
        }
    }
}

void Image::loadAlphaHalfFloatData(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = 0;
            dest[4 * x + 1] = 0;
            dest[4 * x + 2] = 0;
            dest[4 * x + 3] = source[x];
        }
    }
}

void Image::loadLuminanceData(GLsizei width, GLsizei height,
                              int inputPitch, const void *input, size_t outputPitch, void *output, bool native) const
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;

        if (!native)   // BGRA8 destination format
        {
            for (int x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[x];
                dest[4 * x + 1] = source[x];
                dest[4 * x + 2] = source[x];
                dest[4 * x + 3] = 0xFF;
            }
        }
        else   // L8 destination format
        {
            memcpy(dest, source, width);
        }
    }
}

void Image::loadLuminanceFloatData(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x];
            dest[4 * x + 1] = source[x];
            dest[4 * x + 2] = source[x];
            dest[4 * x + 3] = 1.0f;
        }
    }
}

void Image::loadLuminanceHalfFloatData(GLsizei width, GLsizei height,
                                       int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x];
            dest[4 * x + 1] = source[x];
            dest[4 * x + 2] = source[x];
            dest[4 * x + 3] = 0x3C00; // SEEEEEMMMMMMMMMM, S = 0, E = 15, M = 0: 16bit flpt representation of 1
        }
    }
}

void Image::loadLuminanceAlphaData(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output, bool native) const
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        
        if (!native)   // BGRA8 destination format
        {
            for (int x = 0; x < width; x++)
            {
                dest[4 * x + 0] = source[2*x+0];
                dest[4 * x + 1] = source[2*x+0];
                dest[4 * x + 2] = source[2*x+0];
                dest[4 * x + 3] = source[2*x+1];
            }
        }
        else
        {
            memcpy(dest, source, width * 2);
        }
    }
}

void Image::loadLuminanceAlphaFloatData(GLsizei width, GLsizei height,
                                        int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[2*x+0];
            dest[4 * x + 1] = source[2*x+0];
            dest[4 * x + 2] = source[2*x+0];
            dest[4 * x + 3] = source[2*x+1];
        }
    }
}

void Image::loadLuminanceAlphaHalfFloatData(GLsizei width, GLsizei height,
                                            int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[2*x+0];
            dest[4 * x + 1] = source[2*x+0];
            dest[4 * x + 2] = source[2*x+0];
            dest[4 * x + 3] = source[2*x+1];
        }
    }
}

void Image::loadRGBUByteData(GLsizei width, GLsizei height,
                             int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x * 3 + 2];
            dest[4 * x + 1] = source[x * 3 + 1];
            dest[4 * x + 2] = source[x * 3 + 0];
            dest[4 * x + 3] = 0xFF;
        }
    }
}

void Image::loadRGB565Data(GLsizei width, GLsizei height,
                           int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0x001F) << 3) | ((rgba & 0x001F) >> 2);
            dest[4 * x + 1] = ((rgba & 0x07E0) >> 3) | ((rgba & 0x07E0) >> 9);
            dest[4 * x + 2] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
            dest[4 * x + 3] = 0xFF;
        }
    }
}

void Image::loadRGBFloatData(GLsizei width, GLsizei height,
                             int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x * 3 + 0];
            dest[4 * x + 1] = source[x * 3 + 1];
            dest[4 * x + 2] = source[x * 3 + 2];
            dest[4 * x + 3] = 1.0f;
        }
    }
}

void Image::loadRGBHalfFloatData(GLsizei width, GLsizei height,
                                 int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned short *source = NULL;
    unsigned short *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned short*>(static_cast<unsigned char*>(output) + y * outputPitch);
        for (int x = 0; x < width; x++)
        {
            dest[4 * x + 0] = source[x * 3 + 0];
            dest[4 * x + 1] = source[x * 3 + 1];
            dest[4 * x + 2] = source[x * 3 + 2];
            dest[4 * x + 3] = 0x3C00; // SEEEEEMMMMMMMMMM, S = 0, E = 15, M = 0: 16bit flpt representation of 1
        }
    }
}

void Image::loadRGBAUByteData(GLsizei width, GLsizei height,
                              int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned int *source = NULL;
    unsigned int *dest = NULL;
    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned int*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<unsigned int*>(static_cast<unsigned char*>(output) + y * outputPitch);

        for (int x = 0; x < width; x++)
        {
            unsigned int rgba = source[x];
            dest[x] = (_rotl(rgba, 16) & 0x00ff00ff) | (rgba & 0xff00ff00);
        }
    }
}

void Image::loadRGBA4444Data(GLsizei width, GLsizei height,
                             int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0x00F0) << 0) | ((rgba & 0x00F0) >> 4);
            dest[4 * x + 1] = ((rgba & 0x0F00) >> 4) | ((rgba & 0x0F00) >> 8);
            dest[4 * x + 2] = ((rgba & 0xF000) >> 8) | ((rgba & 0xF000) >> 12);
            dest[4 * x + 3] = ((rgba & 0x000F) << 4) | ((rgba & 0x000F) >> 0);
        }
    }
}

void Image::loadRGBA5551Data(GLsizei width, GLsizei height,
                             int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned short *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const unsigned short*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        for (int x = 0; x < width; x++)
        {
            unsigned short rgba = source[x];
            dest[4 * x + 0] = ((rgba & 0x003E) << 2) | ((rgba & 0x003E) >> 3);
            dest[4 * x + 1] = ((rgba & 0x07C0) >> 3) | ((rgba & 0x07C0) >> 8);
            dest[4 * x + 2] = ((rgba & 0xF800) >> 8) | ((rgba & 0xF800) >> 13);
            dest[4 * x + 3] = (rgba & 0x0001) ? 0xFF : 0;
        }
    }
}

void Image::loadRGBAFloatData(GLsizei width, GLsizei height,
                              int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const float *source = NULL;
    float *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = reinterpret_cast<const float*>(static_cast<const unsigned char*>(input) + y * inputPitch);
        dest = reinterpret_cast<float*>(static_cast<unsigned char*>(output) + y * outputPitch);
        memcpy(dest, source, width * 16);
    }
}

void Image::loadRGBAHalfFloatData(GLsizei width, GLsizei height,
                                  int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        memcpy(dest, source, width * 8);
    }
}

void Image::loadBGRAData(GLsizei width, GLsizei height,
                         int inputPitch, const void *input, size_t outputPitch, void *output) const
{
    const unsigned char *source = NULL;
    unsigned char *dest = NULL;

    for (int y = 0; y < height; y++)
    {
        source = static_cast<const unsigned char*>(input) + y * inputPitch;
        dest = static_cast<unsigned char*>(output) + y * outputPitch;
        memcpy(dest, source, width*4);
    }
}

void Image::loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                               const void *input) {
    ASSERT(xoffset % 4 == 0);
    ASSERT(yoffset % 4 == 0);

    RECT lockRect = {
        xoffset, yoffset,
        xoffset + width, yoffset + height
    };

    D3DLOCKED_RECT locked;
    HRESULT result = lock(&locked, &lockRect);
    if (FAILED(result))
    {
        return;
    }

    GLsizei inputSize = ComputeCompressedSize(width, height, mInternalFormat);
    GLsizei inputPitch = ComputeCompressedPitch(width, mInternalFormat);
    int rows = inputSize / inputPitch;
    for (int i = 0; i < rows; ++i)
    {
        memcpy((void*)((BYTE*)locked.pBits + i * locked.Pitch), (void*)((BYTE*)input + i * inputPitch), inputPitch);
    }

    unlock();
}

// This implements glCopyTex[Sub]Image2D for non-renderable internal texture formats and incomplete textures
void Image::copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, IDirect3DSurface9 *renderTarget)
{
    IDirect3DDevice9 *device = getDisplay()->getRenderer()->getDevice(); // D3D9_REPLACE
    IDirect3DSurface9 *renderTargetData = NULL;
    D3DSURFACE_DESC description;
    renderTarget->GetDesc(&description);
    
    HRESULT result = device->CreateOffscreenPlainSurface(description.Width, description.Height, description.Format, D3DPOOL_SYSTEMMEM, &renderTargetData, NULL);

    if (FAILED(result))
    {
        ERR("Could not create matching destination surface.");
        return error(GL_OUT_OF_MEMORY);
    }

    result = device->GetRenderTargetData(renderTarget, renderTargetData);

    if (FAILED(result))
    {
        ERR("GetRenderTargetData unexpectedly failed.");
        renderTargetData->Release();
        return error(GL_OUT_OF_MEMORY);
    }

    RECT sourceRect = {x, y, x + width, y + height};
    RECT destRect = {xoffset, yoffset, xoffset + width, yoffset + height};

    D3DLOCKED_RECT sourceLock = {0};
    result = renderTargetData->LockRect(&sourceLock, &sourceRect, 0);

    if (FAILED(result))
    {
        ERR("Failed to lock the source surface (rectangle might be invalid).");
        renderTargetData->Release();
        return error(GL_OUT_OF_MEMORY);
    }

    D3DLOCKED_RECT destLock = {0};
    result = lock(&destLock, &destRect);
    
    if (FAILED(result))
    {
        ERR("Failed to lock the destination surface (rectangle might be invalid).");
        renderTargetData->UnlockRect();
        renderTargetData->Release();
        return error(GL_OUT_OF_MEMORY);
    }

    if (destLock.pBits && sourceLock.pBits)
    {
        unsigned char *source = (unsigned char*)sourceLock.pBits;
        unsigned char *dest = (unsigned char*)destLock.pBits;

        switch (description.Format)
        {
          case D3DFMT_X8R8G8B8:
          case D3DFMT_A8R8G8B8:
            switch(getD3DFormat())
            {
              case D3DFMT_X8R8G8B8:
              case D3DFMT_A8R8G8B8:
                for(int y = 0; y < height; y++)
                {
                    memcpy(dest, source, 4 * width);

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              case D3DFMT_L8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        dest[x] = source[x * 4 + 2];
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              case D3DFMT_A8L8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        dest[x * 2 + 0] = source[x * 4 + 2];
                        dest[x * 2 + 1] = source[x * 4 + 3];
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              default:
                UNREACHABLE();
            }
            break;
          case D3DFMT_R5G6B5:
            switch(getD3DFormat())
            {
              case D3DFMT_X8R8G8B8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        unsigned short rgb = ((unsigned short*)source)[x];
                        unsigned char red = (rgb & 0xF800) >> 8;
                        unsigned char green = (rgb & 0x07E0) >> 3;
                        unsigned char blue = (rgb & 0x001F) << 3;
                        dest[x + 0] = blue | (blue >> 5);
                        dest[x + 1] = green | (green >> 6);
                        dest[x + 2] = red | (red >> 5);
                        dest[x + 3] = 0xFF;
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              case D3DFMT_L8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        unsigned char red = source[x * 2 + 1] & 0xF8;
                        dest[x] = red | (red >> 5);
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              default:
                UNREACHABLE();
            }
            break;
          case D3DFMT_A1R5G5B5:
            switch(getD3DFormat())
            {
              case D3DFMT_X8R8G8B8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        unsigned short argb = ((unsigned short*)source)[x];
                        unsigned char red = (argb & 0x7C00) >> 7;
                        unsigned char green = (argb & 0x03E0) >> 2;
                        unsigned char blue = (argb & 0x001F) << 3;
                        dest[x + 0] = blue | (blue >> 5);
                        dest[x + 1] = green | (green >> 5);
                        dest[x + 2] = red | (red >> 5);
                        dest[x + 3] = 0xFF;
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              case D3DFMT_A8R8G8B8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        unsigned short argb = ((unsigned short*)source)[x];
                        unsigned char red = (argb & 0x7C00) >> 7;
                        unsigned char green = (argb & 0x03E0) >> 2;
                        unsigned char blue = (argb & 0x001F) << 3;
                        unsigned char alpha = (signed short)argb >> 15;
                        dest[x + 0] = blue | (blue >> 5);
                        dest[x + 1] = green | (green >> 5);
                        dest[x + 2] = red | (red >> 5);
                        dest[x + 3] = alpha;
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              case D3DFMT_L8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        unsigned char red = source[x * 2 + 1] & 0x7C;
                        dest[x] = (red << 1) | (red >> 4);
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              case D3DFMT_A8L8:
                for(int y = 0; y < height; y++)
                {
                    for(int x = 0; x < width; x++)
                    {
                        unsigned char red = source[x * 2 + 1] & 0x7C;
                        dest[x * 2 + 0] = (red << 1) | (red >> 4);
                        dest[x * 2 + 1] = (signed char)source[x * 2 + 1] >> 7;
                    }

                    source += sourceLock.Pitch;
                    dest += destLock.Pitch;
                }
                break;
              default:
                UNREACHABLE();
            }
            break;
          default:
            UNREACHABLE();
        }
    }

    unlock();
    renderTargetData->UnlockRect();

    renderTargetData->Release();

    mDirty = true;
}

}