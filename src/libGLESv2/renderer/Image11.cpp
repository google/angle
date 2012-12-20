//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image11.h: Implements the rx::Image11 class, which acts as the interface to
// the actual underlying resources of a Texture

#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/renderer/Image11.h"

#include "libGLESv2/main.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/renderer/renderer11_utils.h"

namespace rx
{

Image11::Image11()
{
    mStagingTexture = NULL;
    mRenderer = NULL;
    mDXGIFormat = DXGI_FORMAT_UNKNOWN;
}

Image11::~Image11()
{
    if (mStagingTexture)
    {
        mStagingTexture->Release();
    }
}

Image11 *Image11::makeImage11(Image *img)
{
    ASSERT(dynamic_cast<rx::Image11*>(img) != NULL);
    return static_cast<rx::Image11*>(img);
}

bool Image11::isDirty() const
{
    return (mStagingTexture && mDirty);
}

bool Image11::updateSurface(TextureStorageInterface2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    // TODO - relies on TextureStorage11 implementation
    UNIMPLEMENTED();
    return false;
}

bool Image11::updateSurface(TextureStorageInterfaceCube *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height)
{
    // TODO - relies on TextureStorage11 implementation
    UNIMPLEMENTED();
    return false;
}

bool Image11::redefine(Renderer *renderer, GLint internalformat, GLsizei width, GLsizei height, bool forceRelease)
{
    if (mWidth != width ||
        mHeight != height ||
        mInternalFormat != internalformat ||
        forceRelease)
    {
        mRenderer = Renderer11::makeRenderer11(renderer);

        mWidth = width;
        mHeight = height;
        mInternalFormat = internalformat;
        // compute the d3d format that will be used
        mDXGIFormat = gl_d3d11::ConvertTextureFormat(internalformat);
        mActualFormat = d3d11_gl::ConvertTextureInternalFormat(mDXGIFormat);

        if (mStagingTexture)
        {
            mStagingTexture->Release();
            mStagingTexture = NULL;
        }
        
        return true;
    }

    return false;
}

bool Image11::isRenderableFormat() const
{
    // TODO - relies on TextureStorage11 implementation
    UNIMPLEMENTED();
    return false;
}

DXGI_FORMAT Image11::getDXGIFormat() const
{
    // this should only happen if the image hasn't been redefined first
    // which would be a bug by the caller
    ASSERT(mDXGIFormat != DXGI_FORMAT_UNKNOWN);

    return mDXGIFormat;
}

// Store the pixel rectangle designated by xoffset,yoffset,width,height with pixels stored as format/type at input
// into the target pixel rectangle.
void Image11::loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                       GLint unpackAlignment, const void *input)
{
    D3D11_MAPPED_SUBRESOURCE mappedImage;
    HRESULT result = map(&mappedImage);
    if (FAILED(result))
    {
        ERR("Could not map image for loading.");
        return;
    }
    
    GLsizei inputPitch = gl::ComputePitch(width, mInternalFormat, unpackAlignment);
    size_t pixelSize = d3d11::ComputePixelSizeBits(mDXGIFormat) / 8;
    void* offsetMappedData = (void*)((BYTE *)mappedImage.pData + (yoffset * mappedImage.RowPitch + xoffset * pixelSize));

    switch (mInternalFormat)
    {
      case GL_ALPHA8_EXT:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_LUMINANCE8_EXT:
        loadLuminanceDataToNativeOrBGRA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData, false);
        break;
      case GL_ALPHA32F_EXT:
        loadAlphaFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE32F_EXT:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_ALPHA16F_EXT:
        loadAlphaHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE16F_EXT:
        loadLuminanceHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE8_ALPHA8_EXT:
        loadLuminanceAlphaDataToNativeOrBGRA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData, false);
        break;
      case GL_LUMINANCE_ALPHA32F_EXT:
        loadLuminanceAlphaFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_LUMINANCE_ALPHA16F_EXT:
        loadLuminanceAlphaHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGB8_OES:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_RGB565:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_RGBA8_OES:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_RGBA4:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_RGB5_A1:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_BGRA8_EXT:
        loadBGRADataToBGRA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGB32F_EXT:
        UNIMPLEMENTED(); // TODO - new function for D3D11-supported formats
        break;
      case GL_RGB16F_EXT:
        loadRGBHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGBA32F_EXT:
        loadRGBAFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      case GL_RGBA16F_EXT:
        loadRGBAHalfFloatDataToRGBA(width, height, inputPitch, input, mappedImage.RowPitch, offsetMappedData);
        break;
      default: UNREACHABLE(); 
    }

    unmap();
}

void Image11::loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                const void *input)
{
    ASSERT(xoffset % 4 == 0);
    ASSERT(yoffset % 4 == 0);

    D3D11_MAPPED_SUBRESOURCE mappedImage;
    HRESULT result = map(&mappedImage);
    if (FAILED(result))
    {
        ERR("Could not map image for loading.");
        return;
    }

    // Size computation assumes a 4x4 block compressed texture format
    size_t blockSize = d3d11::ComputeBlockSizeBits(mDXGIFormat) / 8;
    void* offsetMappedData = (void*)((BYTE *)mappedImage.pData + ((yoffset / 4) * mappedImage.RowPitch + (xoffset / 4) * blockSize));

    GLsizei inputSize = gl::ComputeCompressedSize(width, height, mInternalFormat);
    GLsizei inputPitch = gl::ComputeCompressedPitch(width, mInternalFormat);
    int rows = inputSize / inputPitch;
    for (int i = 0; i < rows; ++i)
    {
        memcpy((void*)((BYTE*)offsetMappedData + i * mappedImage.RowPitch), (void*)((BYTE*)input + i * inputPitch), inputPitch);
    }

    unmap();
}

void Image11::copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source)
{
    // TODO
    UNIMPLEMENTED();
}

void Image11::createStagingTexture()
{
    if (mStagingTexture)
    {
        return;
    }

    ID3D11Texture2D *newTexture = NULL;
    const DXGI_FORMAT dxgiFormat = getDXGIFormat();
    ASSERT(dxgiFormat != DXGI_FORMAT_D24_UNORM_S8_UINT); // We should never get here for depth textures

    if (mWidth != 0 && mHeight != 0)
    {
        ID3D11Device *device = mRenderer->getDevice();

        D3D11_TEXTURE2D_DESC desc;
        desc.Width = mWidth;
        desc.Height = mHeight;
        desc.MipLevels = desc.ArraySize = 1;
        desc.Format = dxgiFormat;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT result = device->CreateTexture2D(&desc, NULL, &newTexture);

        if (FAILED(result))
        {
            ASSERT(result == E_OUTOFMEMORY);
            ERR("Creating image failed.");
            return error(GL_OUT_OF_MEMORY);
        }
    }

    mStagingTexture = newTexture;
    mDirty = false;
}

HRESULT Image11::map(D3D11_MAPPED_SUBRESOURCE *map)
{
    createStagingTexture();

    HRESULT result = D3DERR_INVALIDCALL;

    if (mStagingTexture)
    {
        ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();
        result = deviceContext->Map(mStagingTexture, 0, D3D11_MAP_WRITE, 0, map);
        ASSERT(SUCCEEDED(result));

        mDirty = true;
    }

    return result;
}

void Image11::unmap()
{
    if (mStagingTexture)
    {
        ID3D11DeviceContext *deviceContext = mRenderer->getDeviceContext();
        deviceContext->Unmap(mStagingTexture, 0);
    }
}

}
