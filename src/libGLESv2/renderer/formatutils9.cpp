#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils9.cpp: Queries for GL image formats and their translations to D3D9
// formats.

#include "libGLESv2/renderer/formatutils9.h"
#include "libGLESv2/renderer/Renderer9.h"
#include "libGLESv2/renderer/generatemip.h"
#include "libGLESv2/renderer/loadimage.h"

namespace rx
{

// Each GL internal format corresponds to one D3D format and data loading function.
// Due to not all formats being available all the time, some of the function/format types are wrapped
// in templates that perform format support queries on a Renderer9 object which is supplied
// when requesting the function or format.

typedef bool ((Renderer9::*Renderer9FormatCheckFunction)(void) const);
typedef LoadImageFunction (*RendererCheckLoadFunction)(const Renderer9 *renderer);

template <Renderer9FormatCheckFunction pred, LoadImageFunction prefered, LoadImageFunction fallback>
LoadImageFunction RendererCheckLoad(const Renderer9 *renderer)
{
    return ((renderer->*pred)()) ? prefered : fallback;
}

template <LoadImageFunction loadFunc>
LoadImageFunction SimpleLoad(const Renderer9 *renderer)
{
    return loadFunc;
}

LoadImageFunction UnreachableLoad(const Renderer9 *renderer)
{
    UNREACHABLE();
    return NULL;
}

typedef bool (*FallbackPredicateFunction)(void);

template <FallbackPredicateFunction pred, LoadImageFunction prefered, LoadImageFunction fallback>
LoadImageFunction FallbackLoadFunction(const Renderer9 *renderer)
{
    return pred() ? prefered : fallback;
}

typedef D3DFORMAT (*FormatQueryFunction)(const rx::Renderer9 *renderer);

template <Renderer9FormatCheckFunction pred, D3DFORMAT prefered, D3DFORMAT fallback>
D3DFORMAT CheckFormatSupport(const rx::Renderer9 *renderer)
{
    return (renderer->*pred)() ? prefered : fallback;
}

template <D3DFORMAT format>
D3DFORMAT D3D9Format(const rx::Renderer9 *renderer)
{
    return format;
}

struct D3D9FormatInfo
{
    FormatQueryFunction mTexFormat;
    FormatQueryFunction mRenderFormat;
    RendererCheckLoadFunction mLoadFunction;

    D3D9FormatInfo()
        : mTexFormat(NULL), mRenderFormat(NULL), mLoadFunction(NULL)
    { }

    D3D9FormatInfo(FormatQueryFunction textureFormat, FormatQueryFunction renderFormat, RendererCheckLoadFunction loadFunc)
        : mTexFormat(textureFormat), mRenderFormat(renderFormat), mLoadFunction(loadFunc)
    { }
};

const D3DFORMAT D3DFMT_INTZ = ((D3DFORMAT)(MAKEFOURCC('I','N','T','Z')));
const D3DFORMAT D3DFMT_NULL = ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')));

typedef std::pair<GLint, D3D9FormatInfo> D3D9FormatPair;
typedef std::map<GLint, D3D9FormatInfo> D3D9FormatMap;

static D3D9FormatMap buildD3D9FormatMap()
{
    D3D9FormatMap map;

    //                       | Internal format                                   | Texture format                  | Render format                    | Load function                                   |
    map.insert(D3D9FormatPair(GL_NONE,                             D3D9FormatInfo(D3D9Format<D3DFMT_NULL>,          D3D9Format<D3DFMT_NULL>,           UnreachableLoad                                  )));

    map.insert(D3D9FormatPair(GL_DEPTH_COMPONENT16,                D3D9FormatInfo(D3D9Format<D3DFMT_INTZ>,          D3D9Format<D3DFMT_D24S8>,          UnreachableLoad                                  )));
    map.insert(D3D9FormatPair(GL_DEPTH_COMPONENT32_OES,            D3D9FormatInfo(D3D9Format<D3DFMT_INTZ>,          D3D9Format<D3DFMT_D32>,            UnreachableLoad                                  )));
    map.insert(D3D9FormatPair(GL_DEPTH24_STENCIL8_OES,             D3D9FormatInfo(D3D9Format<D3DFMT_INTZ>,          D3D9Format<D3DFMT_D24S8>,          UnreachableLoad                                  )));
    map.insert(D3D9FormatPair(GL_STENCIL_INDEX8,                   D3D9FormatInfo(D3D9Format<D3DFMT_UNKNOWN>,       D3D9Format<D3DFMT_D24S8>,          UnreachableLoad                                  ))); // TODO: What's the texture format?

    map.insert(D3D9FormatPair(GL_RGBA32F_EXT,                      D3D9FormatInfo(D3D9Format<D3DFMT_A32B32G32R32F>, D3D9Format<D3DFMT_A32B32G32R32F>,  SimpleLoad<loadRGBAFloatDataToRGBA>              )));
    map.insert(D3D9FormatPair(GL_RGB32F_EXT,                       D3D9FormatInfo(D3D9Format<D3DFMT_A32B32G32R32F>, D3D9Format<D3DFMT_A32B32G32R32F>,  SimpleLoad<loadRGBFloatDataToRGBA>               )));
    map.insert(D3D9FormatPair(GL_ALPHA32F_EXT,                     D3D9FormatInfo(D3D9Format<D3DFMT_A32B32G32R32F>, D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadAlphaFloatDataToRGBA>             )));
    map.insert(D3D9FormatPair(GL_LUMINANCE32F_EXT,                 D3D9FormatInfo(D3D9Format<D3DFMT_A32B32G32R32F>, D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadLuminanceFloatDataToRGBA>         )));
    map.insert(D3D9FormatPair(GL_LUMINANCE_ALPHA32F_EXT,           D3D9FormatInfo(D3D9Format<D3DFMT_A32B32G32R32F>, D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadLuminanceAlphaFloatDataToRGBA>    )));

    map.insert(D3D9FormatPair(GL_RGBA16F_EXT,                      D3D9FormatInfo(D3D9Format<D3DFMT_A16B16G16R16F>, D3D9Format<D3DFMT_A16B16G16R16F>,  SimpleLoad<loadRGBAHalfFloatDataToRGBA>          )));
    map.insert(D3D9FormatPair(GL_RGB16F_EXT,                       D3D9FormatInfo(D3D9Format<D3DFMT_A16B16G16R16F>, D3D9Format<D3DFMT_A16B16G16R16F>,  SimpleLoad<loadRGBHalfFloatDataToRGBA>           )));
    map.insert(D3D9FormatPair(GL_ALPHA16F_EXT,                     D3D9FormatInfo(D3D9Format<D3DFMT_A16B16G16R16F>, D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadAlphaHalfFloatDataToRGBA>         )));
    map.insert(D3D9FormatPair(GL_LUMINANCE16F_EXT,                 D3D9FormatInfo(D3D9Format<D3DFMT_A16B16G16R16F>, D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadLuminanceHalfFloatDataToRGBA>     )));
    map.insert(D3D9FormatPair(GL_LUMINANCE_ALPHA16F_EXT,           D3D9FormatInfo(D3D9Format<D3DFMT_A16B16G16R16F>, D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadLuminanceAlphaHalfFloatDataToRGBA>)));

    map.insert(D3D9FormatPair(GL_ALPHA8_EXT,                       D3D9FormatInfo(D3D9Format<D3DFMT_A8R8G8B8>,      D3D9Format<D3DFMT_A8R8G8B8>,       FallbackLoadFunction<gl::supportsSSE2, loadAlphaDataToBGRASSE2, loadAlphaDataToBGRA>)));

    map.insert(D3D9FormatPair(GL_RGB8_OES,                         D3D9FormatInfo(D3D9Format<D3DFMT_X8R8G8B8>,      D3D9Format<D3DFMT_X8R8G8B8>,       SimpleLoad<loadRGBUByteDataToBGRX>                )));
    map.insert(D3D9FormatPair(GL_RGB565,                           D3D9FormatInfo(D3D9Format<D3DFMT_X8R8G8B8>,      D3D9Format<D3DFMT_R5G6B5>,         SimpleLoad<loadRGB565DataToBGRA>                  )));
    map.insert(D3D9FormatPair(GL_RGBA8_OES,                        D3D9FormatInfo(D3D9Format<D3DFMT_A8R8G8B8>,      D3D9Format<D3DFMT_A8R8G8B8>,       FallbackLoadFunction<gl::supportsSSE2, loadRGBAUByteDataToBGRASSE2, loadRGBAUByteDataToBGRA>)));
    map.insert(D3D9FormatPair(GL_RGBA4,                            D3D9FormatInfo(D3D9Format<D3DFMT_A8R8G8B8>,      D3D9Format<D3DFMT_A8R8G8B8>,       SimpleLoad<loadRGBA4444DataToBGRA>                )));
    map.insert(D3D9FormatPair(GL_RGB5_A1,                          D3D9FormatInfo(D3D9Format<D3DFMT_A8R8G8B8>,      D3D9Format<D3DFMT_A8R8G8B8>,       SimpleLoad<loadRGBA5551DataToBGRA>                )));

    map.insert(D3D9FormatPair(GL_BGRA8_EXT,                        D3D9FormatInfo(D3D9Format<D3DFMT_A8R8G8B8>,      D3D9Format<D3DFMT_A8R8G8B8>,       SimpleLoad<loadBGRADataToBGRA>                    )));
    map.insert(D3D9FormatPair(GL_BGRA4_ANGLEX,                     D3D9FormatInfo(D3D9Format<D3DFMT_A8R8G8B8>,      D3D9Format<D3DFMT_A8R8G8B8>,       SimpleLoad<loadRGBA4444DataToRGBA>                )));
    map.insert(D3D9FormatPair(GL_BGR5_A1_ANGLEX,                   D3D9FormatInfo(D3D9Format<D3DFMT_A8R8G8B8>,      D3D9Format<D3DFMT_A8R8G8B8>,       SimpleLoad<loadRGBA5551DataToRGBA>                )));

    map.insert(D3D9FormatPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,     D3D9FormatInfo(D3D9Format<D3DFMT_DXT1>,          D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadCompressedBlockDataToNative<4, 4,  8> >)));
    map.insert(D3D9FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,    D3D9FormatInfo(D3D9Format<D3DFMT_DXT1>,          D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadCompressedBlockDataToNative<4, 4,  8> >)));
    map.insert(D3D9FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,  D3D9FormatInfo(D3D9Format<D3DFMT_DXT3>,          D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadCompressedBlockDataToNative<4, 4, 16> >)));
    map.insert(D3D9FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,  D3D9FormatInfo(D3D9Format<D3DFMT_DXT5>,          D3D9Format<D3DFMT_UNKNOWN>,        SimpleLoad<loadCompressedBlockDataToNative<4, 4, 16> >)));

    // These formats require checking if the renderer supports D3DFMT_L8 or D3DFMT_A8L8 and
    // then changing the format and loading function appropriately.
    map.insert(D3D9FormatPair(GL_LUMINANCE8_EXT,                   D3D9FormatInfo(CheckFormatSupport<&Renderer9::getLuminanceTextureSupport, D3DFMT_L8, D3DFMT_A8R8G8B8>,        D3D9Format<D3DFMT_UNKNOWN>,  RendererCheckLoad<&Renderer9::getLuminanceTextureSupport, loadLuminanceDataToNative, loadLuminanceDataToBGRA>)));
    map.insert(D3D9FormatPair(GL_LUMINANCE8_ALPHA8_EXT,            D3D9FormatInfo(CheckFormatSupport<&Renderer9::getLuminanceAlphaTextureSupport, D3DFMT_A8L8, D3DFMT_A8R8G8B8>, D3D9Format<D3DFMT_UNKNOWN>,  RendererCheckLoad<&Renderer9::getLuminanceTextureSupport, loadLuminanceAlphaDataToNative, loadLuminanceAlphaDataToBGRA>)));

    return map;
}

static bool getD3D9FormatInfo(GLint internalFormat, D3D9FormatInfo *outFormatInfo)
{
    static const D3D9FormatMap formatMap = buildD3D9FormatMap();
    D3D9FormatMap::const_iterator iter = formatMap.find(internalFormat);
    if (iter != formatMap.end())
    {
        if (outFormatInfo)
        {
            *outFormatInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}

// A map for determining the mip map generation function given a texture's internal D3D format
typedef std::pair<D3DFORMAT, MipGenerationFunction> FormatMipPair;
typedef std::map<D3DFORMAT, MipGenerationFunction> FormatMipMap;

static FormatMipMap buildFormatMipMap()
{
    FormatMipMap map;

    //                      | D3DFORMAT           | Mip generation function  |
    map.insert(FormatMipPair(D3DFMT_L8,            GenerateMip<L8>           ));
    map.insert(FormatMipPair(D3DFMT_A8L8,          GenerateMip<A8L8>         ));
    map.insert(FormatMipPair(D3DFMT_A8R8G8B8,      GenerateMip<A8R8G8B8>     ));
    map.insert(FormatMipPair(D3DFMT_X8R8G8B8,      GenerateMip<A8R8G8B8>     ));
    map.insert(FormatMipPair(D3DFMT_A16B16G16R16F, GenerateMip<A16B16G16R16F>));
    map.insert(FormatMipPair(D3DFMT_A32B32G32R32F, GenerateMip<A32B32G32R32F>));

    return map;
}

// A map to determine the pixel size of a given D3D format
struct D3DFormatInfo
{
    GLuint mPixelBits;
    GLuint mBlockWidth;
    GLuint mBlockHeight;
    GLint mInternalFormat;

    D3DFormatInfo()
        : mPixelBits(0), mBlockWidth(0), mBlockHeight(0), mInternalFormat(GL_NONE)
    { }

    D3DFormatInfo(GLuint pixelBits, GLuint blockWidth, GLuint blockHeight, GLint internalFormat)
        : mPixelBits(pixelBits), mBlockWidth(blockWidth), mBlockHeight(blockHeight), mInternalFormat(internalFormat)
    { }
};

typedef std::pair<D3DFORMAT, D3DFormatInfo> D3D9FormatInfoPair;
typedef std::map<D3DFORMAT, D3DFormatInfo> D3D9FormatInfoMap;

static D3D9FormatInfoMap buildD3D9FormatInfoMap()
{
    D3D9FormatInfoMap map;

    //                           | D3DFORMAT           |             | S  |W |H | Internal format                  |
    map.insert(D3D9FormatInfoPair(D3DFMT_NULL,          D3DFormatInfo(  0, 0, 0, GL_NONE                           )));
    map.insert(D3D9FormatInfoPair(D3DFMT_UNKNOWN,       D3DFormatInfo(  0, 0, 0, GL_NONE                           )));

    map.insert(D3D9FormatInfoPair(D3DFMT_L8,            D3DFormatInfo(  8, 1, 1, GL_LUMINANCE8_EXT                 )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A8,            D3DFormatInfo(  8, 1, 1, GL_ALPHA8_EXT                     )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A8L8,          D3DFormatInfo( 16, 1, 1, GL_LUMINANCE8_ALPHA8_EXT          )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A4R4G4B4,      D3DFormatInfo( 16, 1, 1, GL_RGBA4                          )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A1R5G5B5,      D3DFormatInfo( 16, 1, 1, GL_RGB5_A1                        )));
    map.insert(D3D9FormatInfoPair(D3DFMT_R5G6B5,        D3DFormatInfo( 16, 1, 1, GL_RGB565                         )));
    map.insert(D3D9FormatInfoPair(D3DFMT_X8R8G8B8,      D3DFormatInfo( 32, 1, 1, GL_RGB8_OES                       )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A8R8G8B8,      D3DFormatInfo( 32, 1, 1, GL_RGBA8_OES                      )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A16B16G16R16F, D3DFormatInfo( 64, 1, 1, GL_RGBA16F_EXT                    )));
    map.insert(D3D9FormatInfoPair(D3DFMT_A32B32G32R32F, D3DFormatInfo(128, 1, 1, GL_RGBA32F_EXT                    )));

    map.insert(D3D9FormatInfoPair(D3DFMT_D16,           D3DFormatInfo( 16, 1, 1, GL_DEPTH_COMPONENT16              )));
    map.insert(D3D9FormatInfoPair(D3DFMT_D24S8,         D3DFormatInfo( 32, 1, 1, GL_DEPTH24_STENCIL8_OES           )));
    map.insert(D3D9FormatInfoPair(D3DFMT_D24X8,         D3DFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT16              )));
    map.insert(D3D9FormatInfoPair(D3DFMT_D32,           D3DFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT32_OES          )));

    map.insert(D3D9FormatInfoPair(D3DFMT_INTZ,          D3DFormatInfo( 32, 1, 1, GL_DEPTH24_STENCIL8_OES           )));

    map.insert(D3D9FormatInfoPair(D3DFMT_DXT1,          D3DFormatInfo( 64, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT  )));
    map.insert(D3D9FormatInfoPair(D3DFMT_DXT3,          D3DFormatInfo(128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE)));
    map.insert(D3D9FormatInfoPair(D3DFMT_DXT5,          D3DFormatInfo(128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE)));

    return map;
}

static bool getD3D9FormatInfo(D3DFORMAT format, D3DFormatInfo *outFormatInfo)
{
    static const D3D9FormatInfoMap infoMap = buildD3D9FormatInfoMap();
    D3D9FormatInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        if (outFormatInfo)
        {
            *outFormatInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}

namespace d3d9
{

MipGenerationFunction GetMipGenerationFunction(D3DFORMAT format)
{
    static const FormatMipMap mipFunctionMap = buildFormatMipMap();
    FormatMipMap::const_iterator iter = mipFunctionMap.find(format);
    if (iter != mipFunctionMap.end())
    {
        return iter->second;
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

LoadImageFunction GetImageLoadFunction(GLint internalFormat, const Renderer9 *renderer)
{
    if (!renderer)
    {
        return NULL;
    }

    ASSERT(renderer->getCurrentClientVersion() == 2);

    D3D9FormatInfo d3d9FormatInfo;
    if (getD3D9FormatInfo(internalFormat, &d3d9FormatInfo))
    {
        return d3d9FormatInfo.mLoadFunction(renderer);
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

GLuint GetFormatPixelBytes(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (getD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mPixelBits / 8;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockWidth(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (getD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mBlockWidth;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockHeight(D3DFORMAT format)
{
    D3DFormatInfo d3dFormatInfo;
    if (getD3D9FormatInfo(format, &d3dFormatInfo))
    {
        return d3dFormatInfo.mBlockHeight;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockSize(D3DFORMAT format, GLuint width, GLuint height)
{
    D3DFormatInfo d3dFormatInfo;
    if (getD3D9FormatInfo(format, &d3dFormatInfo))
    {
        GLuint numBlocksWide = (width + d3dFormatInfo.mBlockWidth - 1) / d3dFormatInfo.mBlockWidth;
        GLuint numBlocksHight = (height + d3dFormatInfo.mBlockHeight - 1) / d3dFormatInfo.mBlockHeight;

        return (d3dFormatInfo.mPixelBits * numBlocksWide * numBlocksHight) / 8;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

void MakeValidSize(bool isImage, D3DFORMAT format, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset)
{
    D3DFormatInfo d3dFormatInfo;
    if (getD3D9FormatInfo(format, &d3dFormatInfo))
    {
        int upsampleCount = 0;

        GLsizei blockWidth = d3dFormatInfo.mBlockWidth;
        GLsizei blockHeight = d3dFormatInfo.mBlockHeight;

        // Don't expand the size of full textures that are at least (blockWidth x blockHeight) already.
        if (isImage || *requestWidth < blockWidth || *requestHeight < blockHeight)
        {
            while (*requestWidth % blockWidth != 0 || *requestHeight % blockHeight != 0)
            {
                *requestWidth <<= 1;
                *requestHeight <<= 1;
                upsampleCount++;
            }
        }
        *levelOffset = upsampleCount;
    }
}

}

namespace gl_d3d9
{

D3DFORMAT GetTexureFormat(GLint internalFormat, const Renderer9 *renderer)
{
    if (!renderer)
    {
        UNREACHABLE();
        return D3DFMT_UNKNOWN;
    }

    ASSERT(renderer->getCurrentClientVersion() == 2);

    D3D9FormatInfo d3d9FormatInfo;
    if (getD3D9FormatInfo(internalFormat, &d3d9FormatInfo))
    {
        return d3d9FormatInfo.mTexFormat(renderer);
    }
    else
    {
        UNREACHABLE();
        return D3DFMT_UNKNOWN;
    }
}

D3DFORMAT GetRenderFormat(GLint internalFormat, const Renderer9 *renderer)
{
    if (!renderer)
    {
        UNREACHABLE();
        return D3DFMT_UNKNOWN;
    }

    ASSERT(renderer->getCurrentClientVersion() == 2);

    D3D9FormatInfo d3d9FormatInfo;
    if (getD3D9FormatInfo(internalFormat, &d3d9FormatInfo))
    {
        return d3d9FormatInfo.mRenderFormat(renderer);
    }
    else
    {
        UNREACHABLE();
        return D3DFMT_UNKNOWN;
    }
}

D3DMULTISAMPLE_TYPE GetMultisampleType(GLsizei samples)
{
    return (samples > 1) ? static_cast<D3DMULTISAMPLE_TYPE>(samples) : D3DMULTISAMPLE_NONE;
}

}

namespace d3d9_gl
{

GLint GetInternalFormat(D3DFORMAT format)
{
    static const D3D9FormatInfoMap infoMap = buildD3D9FormatInfoMap();
    D3D9FormatInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        return iter->second.mInternalFormat;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

GLsizei GetSamplesCount(D3DMULTISAMPLE_TYPE type)
{
    return (type != D3DMULTISAMPLE_NONMASKABLE) ? type : 0;
}

bool IsFormatChannelEquivalent(D3DFORMAT d3dformat, GLenum format, GLuint clientVersion)
{
    GLint internalFormat = d3d9_gl::GetInternalFormat(d3dformat);
    GLenum convertedFormat = gl::GetFormat(internalFormat, clientVersion);
    return convertedFormat == format;
}

}

}
