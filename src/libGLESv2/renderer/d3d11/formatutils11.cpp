#include "precompiled.h"
//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils11.cpp: Queries for GL image formats and their translations to D3D11
// formats.

#include "libGLESv2/renderer/d3d11/formatutils11.h"
#include "libGLESv2/renderer/generatemip.h"
#include "libGLESv2/renderer/loadimage.h"
#include "libGLESv2/renderer/copyimage.h"

namespace rx
{

struct D3D11ES3FormatInfo
{
    DXGI_FORMAT mTexFormat;
    DXGI_FORMAT mSRVFormat;
    DXGI_FORMAT mRTVFormat;
    DXGI_FORMAT mDSVFormat;

    D3D11ES3FormatInfo()
        : mTexFormat(DXGI_FORMAT_UNKNOWN), mDSVFormat(DXGI_FORMAT_UNKNOWN), mRTVFormat(DXGI_FORMAT_UNKNOWN), mSRVFormat(DXGI_FORMAT_UNKNOWN)
    { }

    D3D11ES3FormatInfo(DXGI_FORMAT texFormat, DXGI_FORMAT srvFormat, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat)
        : mTexFormat(texFormat), mDSVFormat(dsvFormat), mRTVFormat(rtvFormat), mSRVFormat(srvFormat)
    { }
};

// For sized GL internal formats, there is only one corresponding D3D11 format. This map type allows
// querying for the DXGI texture formats to use for textures, SRVs, RTVs and DSVs given a GL internal
// format.
typedef std::pair<GLenum, D3D11ES3FormatInfo> D3D11ES3FormatPair;
typedef std::map<GLenum, D3D11ES3FormatInfo> D3D11ES3FormatMap;

static D3D11ES3FormatMap BuildD3D11ES3FormatMap()
{
    D3D11ES3FormatMap map;

    //                           | GL internal format  |                  | D3D11 texture format            | D3D11 SRV format               | D3D11 RTV format               | D3D11 DSV format   |
    map.insert(D3D11ES3FormatPair(GL_NONE,              D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R8,                D3D11ES3FormatInfo(DXGI_FORMAT_R8_UNORM,             DXGI_FORMAT_R8_UNORM,            DXGI_FORMAT_R8_UNORM,            DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R8_SNORM,          D3D11ES3FormatInfo(DXGI_FORMAT_R8_SNORM,             DXGI_FORMAT_R8_SNORM,            DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG8,               D3D11ES3FormatInfo(DXGI_FORMAT_R8G8_UNORM,           DXGI_FORMAT_R8G8_UNORM,          DXGI_FORMAT_R8G8_UNORM,          DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG8_SNORM,         D3D11ES3FormatInfo(DXGI_FORMAT_R8G8_SNORM,           DXGI_FORMAT_R8G8_SNORM,          DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB8,              D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB8_SNORM,        D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_SNORM,       DXGI_FORMAT_R8G8B8A8_SNORM,      DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB565,            D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA4,             D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB5_A1,           D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA8,             D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA8_SNORM,       D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_SNORM,       DXGI_FORMAT_R8G8B8A8_SNORM,      DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB10_A2,          D3D11ES3FormatInfo(DXGI_FORMAT_R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,   DXGI_FORMAT_R10G10B10A2_UNORM,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB10_A2UI,        D3D11ES3FormatInfo(DXGI_FORMAT_R10G10B10A2_UINT,     DXGI_FORMAT_R10G10B10A2_UINT,    DXGI_FORMAT_R10G10B10A2_UINT,    DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_SRGB8,             D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_SRGB8_ALPHA8,      D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,  DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R16F,              D3D11ES3FormatInfo(DXGI_FORMAT_R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,           DXGI_FORMAT_R16_FLOAT,           DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG16F,             D3D11ES3FormatInfo(DXGI_FORMAT_R16G16_FLOAT,         DXGI_FORMAT_R16G16_FLOAT,        DXGI_FORMAT_R16G16_FLOAT,        DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB16F,            D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT,   DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA16F,           D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT,   DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R32F,              D3D11ES3FormatInfo(DXGI_FORMAT_R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,           DXGI_FORMAT_R32_FLOAT,           DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG32F,             D3D11ES3FormatInfo(DXGI_FORMAT_R32G32_FLOAT,         DXGI_FORMAT_R32G32_FLOAT,        DXGI_FORMAT_R32G32_FLOAT,        DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB32F,            D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT,   DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA32F,           D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT,   DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R11F_G11F_B10F,    D3D11ES3FormatInfo(DXGI_FORMAT_R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,     DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB9_E5,           D3D11ES3FormatInfo(DXGI_FORMAT_R9G9B9E5_SHAREDEXP,   DXGI_FORMAT_R9G9B9E5_SHAREDEXP,  DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R8I,               D3D11ES3FormatInfo(DXGI_FORMAT_R8_SINT,              DXGI_FORMAT_R8_SINT,             DXGI_FORMAT_R8_SINT,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R8UI,              D3D11ES3FormatInfo(DXGI_FORMAT_R8_UINT,              DXGI_FORMAT_R8_UINT,             DXGI_FORMAT_R8_UINT,             DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R16I,              D3D11ES3FormatInfo(DXGI_FORMAT_R16_SINT,             DXGI_FORMAT_R16_SINT,            DXGI_FORMAT_R16_SINT,            DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R16UI,             D3D11ES3FormatInfo(DXGI_FORMAT_R16_UINT,             DXGI_FORMAT_R16_UINT,            DXGI_FORMAT_R16_UINT,            DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R32I,              D3D11ES3FormatInfo(DXGI_FORMAT_R32_SINT,             DXGI_FORMAT_R32_SINT,            DXGI_FORMAT_R32_SINT,            DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_R32UI,             D3D11ES3FormatInfo(DXGI_FORMAT_R32_UINT,             DXGI_FORMAT_R32_UINT,            DXGI_FORMAT_R32_UINT,            DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG8I,              D3D11ES3FormatInfo(DXGI_FORMAT_R8G8_SINT,            DXGI_FORMAT_R8G8_SINT,           DXGI_FORMAT_R8G8_SINT,           DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG8UI,             D3D11ES3FormatInfo(DXGI_FORMAT_R8G8_UINT,            DXGI_FORMAT_R8G8_UINT,           DXGI_FORMAT_R8G8_UINT,           DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG16I,             D3D11ES3FormatInfo(DXGI_FORMAT_R16G16_SINT,          DXGI_FORMAT_R16G16_SINT,         DXGI_FORMAT_R16G16_SINT,         DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG16UI,            D3D11ES3FormatInfo(DXGI_FORMAT_R16G16_UINT,          DXGI_FORMAT_R16G16_UINT,         DXGI_FORMAT_R16G16_UINT,         DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG32I,             D3D11ES3FormatInfo(DXGI_FORMAT_R32G32_SINT,          DXGI_FORMAT_R32G32_SINT,         DXGI_FORMAT_R32G32_SINT,         DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RG32UI,            D3D11ES3FormatInfo(DXGI_FORMAT_R32G32_UINT,          DXGI_FORMAT_R32G32_UINT,         DXGI_FORMAT_R32G32_UINT,         DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB8I,             D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_SINT,        DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB8UI,            D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UINT,        DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB16I,            D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_SINT,    DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB16UI,           D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_UINT,    DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB32I,            D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_SINT,    DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB32UI,           D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_UINT,    DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA8I,            D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_SINT,        DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_R8G8B8A8_SINT,       DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA8UI,           D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UINT,        DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_R8G8B8A8_UINT,       DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA16I,           D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_SINT,    DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_R16G16B16A16_SINT,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA16UI,          D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_UINT,    DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_R16G16B16A16_UINT,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA32I,           D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_SINT,    DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_R32G32B32A32_SINT,   DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA32UI,          D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_UINT,    DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_R32G32B32A32_UINT,   DXGI_FORMAT_UNKNOWN)));

    // Unsized formats, TODO: Are types of float and half float allowed for the unsized types? Would it change the DXGI format?
    map.insert(D3D11ES3FormatPair(GL_ALPHA,             D3D11ES3FormatInfo(DXGI_FORMAT_A8_UNORM,             DXGI_FORMAT_A8_UNORM,            DXGI_FORMAT_A8_UNORM,            DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE,         D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE_ALPHA,   D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGB,               D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_RGBA,              D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_BGRA_EXT,          D3D11ES3FormatInfo(DXGI_FORMAT_B8G8R8A8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM,      DXGI_FORMAT_B8G8R8A8_UNORM,      DXGI_FORMAT_UNKNOWN)));

    // From GL_EXT_texture_storage
    //                           | GL internal format       |                  | D3D11 texture format          | D3D11 SRV format                    | D3D11 RTV format              | D3D11 DSV format               |
    map.insert(D3D11ES3FormatPair(GL_ALPHA8_EXT,             D3D11ES3FormatInfo(DXGI_FORMAT_A8_UNORM,           DXGI_FORMAT_A8_UNORM,                 DXGI_FORMAT_A8_UNORM,           DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE8_EXT,         D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_ALPHA32F_EXT,           D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE32F_EXT,       D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32_FLOAT,    DXGI_FORMAT_R32G32B32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT,    DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_ALPHA16F_EXT,           D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE16F_EXT,       D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,                  DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE8_ALPHA8_EXT,  D3D11ES3FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE_ALPHA32F_EXT, D3D11ES3FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_LUMINANCE_ALPHA16F_EXT, D3D11ES3FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_BGRA8_EXT,              D3D11ES3FormatInfo(DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_BGRA4_ANGLEX,           D3D11ES3FormatInfo(DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN             )));
    map.insert(D3D11ES3FormatPair(GL_BGR5_A1_ANGLEX,         D3D11ES3FormatInfo(DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN             )));

    // Depth stencil formats
    map.insert(D3D11ES3FormatPair(GL_DEPTH_COMPONENT16,     D3D11ES3FormatInfo(DXGI_FORMAT_R16_TYPELESS,        DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D16_UNORM           )));
    map.insert(D3D11ES3FormatPair(GL_DEPTH_COMPONENT24,     D3D11ES3FormatInfo(DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT   )));
    map.insert(D3D11ES3FormatPair(GL_DEPTH_COMPONENT32F,    D3D11ES3FormatInfo(DXGI_FORMAT_R32_TYPELESS,        DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D32_FLOAT           )));
    map.insert(D3D11ES3FormatPair(GL_DEPTH24_STENCIL8,      D3D11ES3FormatInfo(DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT   )));
    map.insert(D3D11ES3FormatPair(GL_DEPTH32F_STENCIL8,     D3D11ES3FormatInfo(DXGI_FORMAT_R32G8X24_TYPELESS,   DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D32_FLOAT_S8X24_UINT)));
    map.insert(D3D11ES3FormatPair(GL_STENCIL_INDEX8,        D3D11ES3FormatInfo(DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_X24_TYPELESS_G8_UINT,     DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT   )));

    // From GL_ANGLE_depth_texture
    map.insert(D3D11ES3FormatPair(GL_DEPTH_COMPONENT32_OES, D3D11ES3FormatInfo(DXGI_FORMAT_R24G8_TYPELESS,      DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_UNKNOWN,             DXGI_FORMAT_D24_UNORM_S8_UINT  )));

    // Compressed formats, From ES 3.0.1 spec, table 3.16
    //                           | GL internal format                          |                  | D3D11 texture format | D3D11 SRV format     | D3D11 RTV format   | D3D11 DSV format  |
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_R11_EAC,                        D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_SIGNED_R11_EAC,                 D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RG11_EAC,                       D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_SIGNED_RG11_EAC,                D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RGB8_ETC2,                      D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_SRGB8_ETC2,                     D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RGBA8_ETC2_EAC,                 D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          D3D11ES3FormatInfo(DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN,   DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));

    // From GL_EXT_texture_compression_dxt1
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              D3D11ES3FormatInfo(DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             D3D11ES3FormatInfo(DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));

    // From GL_ANGLE_texture_compression_dxt3
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           D3D11ES3FormatInfo(DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));

    // From GL_ANGLE_texture_compression_dxt5
    map.insert(D3D11ES3FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           D3D11ES3FormatInfo(DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_UNKNOWN)));

    return map;
}

static bool GetD3D11ES3FormatInfo(GLenum internalFormat, GLuint clientVersion, D3D11ES3FormatInfo *outFormatInfo)
{
    static const D3D11ES3FormatMap formatMap = BuildD3D11ES3FormatMap();
    D3D11ES3FormatMap::const_iterator iter = formatMap.find(internalFormat);
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

// ES3 image loading functions vary based on the internal format and data type given,
// this map type determines the loading function from the internal format and type supplied
// to glTex*Image*D and the destination DXGI_FORMAT. Source formats and types are taken from
// Tables 3.2 and 3.3 of the ES 3 spec.
typedef std::pair<GLenum, GLenum> InternalFormatTypePair;
typedef std::pair<InternalFormatTypePair, LoadImageFunction> D3D11LoadFunctionPair;
typedef std::map<InternalFormatTypePair, LoadImageFunction> D3D11LoadFunctionMap;

static void UnimplementedLoadFunction(int width, int height, int depth,
                                      const void *input, unsigned int inputRowPitch, unsigned int inputDepthPitch,
                                      void *output, unsigned int outputRowPitch, unsigned int outputDepthPitch)
{
    UNIMPLEMENTED();
}

static void UnreachableLoadFunction(int width, int height, int depth,
                                    const void *input, unsigned int inputRowPitch, unsigned int inputDepthPitch,
                                    void *output, unsigned int outputRowPitch, unsigned int outputDepthPitch)
{
    UNREACHABLE();
}

// A helper function to insert data into the D3D11LoadFunctionMap with fewer characters.
static inline void insertLoadFunction(D3D11LoadFunctionMap *map, GLenum internalFormat, GLenum type,
                                      LoadImageFunction loadFunc)
{
    map->insert(D3D11LoadFunctionPair(InternalFormatTypePair(internalFormat, type), loadFunc));
}

D3D11LoadFunctionMap buildD3D11LoadFunctionMap()
{
    D3D11LoadFunctionMap map;

    //                      | Internal format      | Type                             | Load function                       |
    insertLoadFunction(&map, GL_RGBA8,              GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>             );
    insertLoadFunction(&map, GL_RGB5_A1,            GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>             );
    insertLoadFunction(&map, GL_RGBA4,              GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>             );
    insertLoadFunction(&map, GL_SRGB8_ALPHA8,       GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>             );
    insertLoadFunction(&map, GL_RGBA8_SNORM,        GL_BYTE,                           loadToNative<GLbyte, 4>              );
    insertLoadFunction(&map, GL_RGBA4,              GL_UNSIGNED_SHORT_4_4_4_4,         loadRGBA4444DataToRGBA               );
    insertLoadFunction(&map, GL_RGB10_A2,           GL_UNSIGNED_INT_2_10_10_10_REV,    loadToNative<GLuint, 1>              );
    insertLoadFunction(&map, GL_RGB5_A1,            GL_UNSIGNED_SHORT_5_5_5_1,         loadRGBA5551DataToRGBA               );
    insertLoadFunction(&map, GL_RGB5_A1,            GL_UNSIGNED_INT_2_10_10_10_REV,    loadRGBA2101010ToRGBA                );
    insertLoadFunction(&map, GL_RGBA16F,            GL_HALF_FLOAT,                     loadToNative<GLhalf, 4>              );
    insertLoadFunction(&map, GL_RGBA32F,            GL_FLOAT,                          loadToNative<GLfloat, 4>             );
    insertLoadFunction(&map, GL_RGBA16F,            GL_FLOAT,                          loadFloatDataToHalfFloat<4>          );
    insertLoadFunction(&map, GL_RGBA8UI,            GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>             );
    insertLoadFunction(&map, GL_RGBA8I,             GL_BYTE,                           loadToNative<GLbyte, 4>              );
    insertLoadFunction(&map, GL_RGBA16UI,           GL_UNSIGNED_SHORT,                 loadToNative<GLushort, 4>            );
    insertLoadFunction(&map, GL_RGBA16I,            GL_SHORT,                          loadToNative<GLshort, 4>             );
    insertLoadFunction(&map, GL_RGBA32UI,           GL_UNSIGNED_INT,                   loadToNative<GLuint, 4>              );
    insertLoadFunction(&map, GL_RGBA32I,            GL_INT,                            loadToNative<GLint, 4>               );
    insertLoadFunction(&map, GL_RGB10_A2UI,         GL_UNSIGNED_INT_2_10_10_10_REV,    loadToNative<GLuint, 1>              );
    insertLoadFunction(&map, GL_RGB8,               GL_UNSIGNED_BYTE,                  loadRGBUByteDataToRGBA               );
    insertLoadFunction(&map, GL_RGB565,             GL_UNSIGNED_BYTE,                  loadToNative3To4<GLubyte, 0xFF>      );
    insertLoadFunction(&map, GL_SRGB8,              GL_UNSIGNED_BYTE,                  loadToNative3To4<GLubyte, 0xFF>      );
    insertLoadFunction(&map, GL_RGB8_SNORM,         GL_BYTE,                           loadRGBSByteDataToRGBA               );
    insertLoadFunction(&map, GL_RGB565,             GL_UNSIGNED_SHORT_5_6_5,           loadRGB565DataToRGBA                 );
    insertLoadFunction(&map, GL_R11F_G11F_B10F,     GL_UNSIGNED_INT_10F_11F_11F_REV,   loadToNative<GLuint, 1>              );
    insertLoadFunction(&map, GL_RGB9_E5,            GL_UNSIGNED_INT_5_9_9_9_REV,       loadToNative<GLuint, 1>              );
    insertLoadFunction(&map, GL_RGB16F,             GL_HALF_FLOAT,                     loadToNative3To4<GLhalf, gl::Float16One>);
    insertLoadFunction(&map, GL_R11F_G11F_B10F,     GL_HALF_FLOAT,                     loadRGBHalfFloatDataTo111110Float    );
    insertLoadFunction(&map, GL_RGB9_E5,            GL_HALF_FLOAT,                     loadRGBHalfFloatDataTo999E5          );
    insertLoadFunction(&map, GL_RGB32F,             GL_FLOAT,                          loadToNative3To4<GLfloat, gl::Float32One>);
    insertLoadFunction(&map, GL_RGB16F,             GL_FLOAT,                          loadFloatRGBDataToHalfFloatRGBA      );
    insertLoadFunction(&map, GL_R11F_G11F_B10F,     GL_FLOAT,                          loadRGBFloatDataTo111110Float        );
    insertLoadFunction(&map, GL_RGB9_E5,            GL_FLOAT,                          loadRGBFloatDataTo999E5              );
    insertLoadFunction(&map, GL_RGB8UI,             GL_UNSIGNED_BYTE,                  loadToNative3To4<GLubyte, 0x01>      );
    insertLoadFunction(&map, GL_RGB8I,              GL_BYTE,                           loadToNative3To4<GLbyte, 0x01>       );
    insertLoadFunction(&map, GL_RGB16UI,            GL_UNSIGNED_SHORT,                 loadToNative3To4<GLushort, 0x0001>   );
    insertLoadFunction(&map, GL_RGB16I,             GL_SHORT,                          loadToNative3To4<GLshort, 0x0001>    );
    insertLoadFunction(&map, GL_RGB32UI,            GL_UNSIGNED_INT,                   loadToNative3To4<GLuint, 0x00000001> );
    insertLoadFunction(&map, GL_RGB32I,             GL_INT,                            loadToNative3To4<GLint, 0x00000001>  );
    insertLoadFunction(&map, GL_RG8,                GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 2>             );
    insertLoadFunction(&map, GL_RG8_SNORM,          GL_BYTE,                           loadToNative<GLbyte, 2>              );
    insertLoadFunction(&map, GL_RG16F,              GL_HALF_FLOAT,                     loadToNative<GLhalf, 2>              );
    insertLoadFunction(&map, GL_RG32F,              GL_FLOAT,                          loadToNative<GLfloat, 2>             );
    insertLoadFunction(&map, GL_RG16F,              GL_FLOAT,                          loadFloatDataToHalfFloat<2>          );
    insertLoadFunction(&map, GL_RG8UI,              GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 2>             );
    insertLoadFunction(&map, GL_RG8I,               GL_BYTE,                           loadToNative<GLbyte, 2>              );
    insertLoadFunction(&map, GL_RG16UI,             GL_UNSIGNED_SHORT,                 loadToNative<GLushort, 2>            );
    insertLoadFunction(&map, GL_RG16I,              GL_SHORT,                          loadToNative<GLshort, 2>             );
    insertLoadFunction(&map, GL_RG32UI,             GL_UNSIGNED_INT,                   loadToNative<GLuint, 2>              );
    insertLoadFunction(&map, GL_RG32I,              GL_INT,                            loadToNative<GLint, 2>               );
    insertLoadFunction(&map, GL_R8,                 GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 1>             );
    insertLoadFunction(&map, GL_R8_SNORM,           GL_BYTE,                           loadToNative<GLbyte, 1>              );
    insertLoadFunction(&map, GL_R16F,               GL_HALF_FLOAT,                     loadToNative<GLhalf, 1>              );
    insertLoadFunction(&map, GL_R32F,               GL_FLOAT,                          loadToNative<GLfloat, 1>             );
    insertLoadFunction(&map, GL_R16F,               GL_FLOAT,                          loadFloatDataToHalfFloat<1>          );
    insertLoadFunction(&map, GL_R8UI,               GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 1>             );
    insertLoadFunction(&map, GL_R8I,                GL_BYTE,                           loadToNative<GLbyte, 1>              );
    insertLoadFunction(&map, GL_R16UI,              GL_UNSIGNED_SHORT,                 loadToNative<GLushort, 1>            );
    insertLoadFunction(&map, GL_R16I,               GL_SHORT,                          loadToNative<GLshort, 1>             );
    insertLoadFunction(&map, GL_R32UI,              GL_UNSIGNED_INT,                   loadToNative<GLuint, 1>              );
    insertLoadFunction(&map, GL_R32I,               GL_INT,                            loadToNative<GLint, 1>               );
    insertLoadFunction(&map, GL_DEPTH_COMPONENT16,  GL_UNSIGNED_SHORT,                 loadToNative<GLushort, 1>            );
    insertLoadFunction(&map, GL_DEPTH_COMPONENT24,  GL_UNSIGNED_INT,                   loadG8R24DataToR24G8                 );
    insertLoadFunction(&map, GL_DEPTH_COMPONENT16,  GL_UNSIGNED_INT,                   loadUintDataToUshort                 );
    insertLoadFunction(&map, GL_DEPTH_COMPONENT32F, GL_FLOAT,                          loadToNative<GLfloat, 1>             );
    insertLoadFunction(&map, GL_DEPTH24_STENCIL8,   GL_UNSIGNED_INT_24_8,              loadG8R24DataToR24G8                 );
    insertLoadFunction(&map, GL_DEPTH32F_STENCIL8,  GL_FLOAT_32_UNSIGNED_INT_24_8_REV, loadToNative<GLuint, 2>              );

    // Unsized formats
    // Load functions are unreachable because they are converted to sized internal formats based on
    // the format and type before loading takes place.
    insertLoadFunction(&map, GL_RGBA,               GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    insertLoadFunction(&map, GL_RGBA,               GL_UNSIGNED_SHORT_4_4_4_4,         UnreachableLoadFunction              );
    insertLoadFunction(&map, GL_RGBA,               GL_UNSIGNED_SHORT_5_5_5_1,         UnreachableLoadFunction              );
    insertLoadFunction(&map, GL_RGB,                GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    insertLoadFunction(&map, GL_RGB,                GL_UNSIGNED_SHORT_5_6_5,           UnreachableLoadFunction              );
    insertLoadFunction(&map, GL_LUMINANCE_ALPHA,    GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    insertLoadFunction(&map, GL_LUMINANCE,          GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );
    insertLoadFunction(&map, GL_ALPHA,              GL_UNSIGNED_BYTE,                  UnreachableLoadFunction              );

    // From GL_OES_texture_float
    insertLoadFunction(&map, GL_LUMINANCE_ALPHA,    GL_FLOAT,                          loadLuminanceAlphaFloatDataToRGBA    );
    insertLoadFunction(&map, GL_LUMINANCE,          GL_FLOAT,                          loadLuminanceFloatDataToRGB          );
    insertLoadFunction(&map, GL_ALPHA,              GL_FLOAT,                          loadAlphaFloatDataToRGBA             );

    // From GL_OES_texture_half_float
    insertLoadFunction(&map, GL_LUMINANCE_ALPHA,    GL_HALF_FLOAT,                     loadLuminanceAlphaHalfFloatDataToRGBA);
    insertLoadFunction(&map, GL_LUMINANCE,          GL_HALF_FLOAT,                     loadLuminanceHalfFloatDataToRGBA     );
    insertLoadFunction(&map, GL_ALPHA,              GL_HALF_FLOAT,                     loadAlphaHalfFloatDataToRGBA         );

    // From GL_EXT_texture_storage
    insertLoadFunction(&map, GL_ALPHA8_EXT,             GL_UNSIGNED_BYTE,              loadToNative<GLubyte, 1>             );
    insertLoadFunction(&map, GL_LUMINANCE8_EXT,         GL_UNSIGNED_BYTE,              loadLuminanceDataToBGRA              );
    insertLoadFunction(&map, GL_LUMINANCE8_ALPHA8_EXT,  GL_UNSIGNED_BYTE,              loadLuminanceAlphaDataToBGRA         );
    insertLoadFunction(&map, GL_ALPHA32F_EXT,           GL_FLOAT,                      loadAlphaFloatDataToRGBA             );
    insertLoadFunction(&map, GL_LUMINANCE32F_EXT,       GL_FLOAT,                      loadLuminanceFloatDataToRGB          );
    insertLoadFunction(&map, GL_LUMINANCE_ALPHA32F_EXT, GL_FLOAT,                      loadLuminanceAlphaFloatDataToRGBA    );
    insertLoadFunction(&map, GL_ALPHA16F_EXT,           GL_HALF_FLOAT,                 loadAlphaHalfFloatDataToRGBA         );
    insertLoadFunction(&map, GL_LUMINANCE16F_EXT,       GL_HALF_FLOAT,                 loadLuminanceHalfFloatDataToRGBA     );
    insertLoadFunction(&map, GL_LUMINANCE_ALPHA16F_EXT, GL_HALF_FLOAT,                 loadLuminanceAlphaHalfFloatDataToRGBA);

    insertLoadFunction(&map, GL_BGRA8_EXT,              GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>         );
    insertLoadFunction(&map, GL_BGRA4_ANGLEX,           GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT, loadRGBA4444DataToRGBA           );
    insertLoadFunction(&map, GL_BGRA4_ANGLEX,           GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>         );
    insertLoadFunction(&map, GL_BGR5_A1_ANGLEX,         GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT, loadRGBA5551DataToRGBA           );
    insertLoadFunction(&map, GL_BGR5_A1_ANGLEX,         GL_UNSIGNED_BYTE,                  loadToNative<GLubyte, 4>         );

    // Compressed formats
    // From ES 3.0.1 spec, table 3.16
    //                      | Internal format                             | Type            | Load function                     |
    insertLoadFunction(&map, GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_R11_EAC,                        GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_SIGNED_R11_EAC,                 GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_RG11_EAC,                       GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_SIGNED_RG11_EAC,                GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_RGB8_ETC2,                      GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_SRGB8_ETC2,                     GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2,  GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2, GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_RGBA8_ETC2_EAC,                 GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );
    insertLoadFunction(&map, GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC,          GL_UNSIGNED_BYTE, UnimplementedLoadFunction          );

    // From GL_EXT_texture_compression_dxt1
    insertLoadFunction(&map, GL_COMPRESSED_RGB_S3TC_DXT1_EXT,              GL_UNSIGNED_BYTE, loadCompressedBlockDataToNative<4, 4,  8>);
    insertLoadFunction(&map, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,             GL_UNSIGNED_BYTE, loadCompressedBlockDataToNative<4, 4,  8>);

    // From GL_ANGLE_texture_compression_dxt3
    insertLoadFunction(&map, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE,           GL_UNSIGNED_BYTE, loadCompressedBlockDataToNative<4, 4, 16>);

    // From GL_ANGLE_texture_compression_dxt5
    insertLoadFunction(&map, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE,           GL_UNSIGNED_BYTE, loadCompressedBlockDataToNative<4, 4, 16>);

    return map;
}

struct D3D11ES2FormatInfo
{
    DXGI_FORMAT mTexFormat;
    DXGI_FORMAT mSRVFormat;
    DXGI_FORMAT mRTVFormat;
    DXGI_FORMAT mDSVFormat;

    LoadImageFunction mLoadImageFunction;

    D3D11ES2FormatInfo()
        : mTexFormat(DXGI_FORMAT_UNKNOWN), mDSVFormat(DXGI_FORMAT_UNKNOWN), mRTVFormat(DXGI_FORMAT_UNKNOWN),
        mSRVFormat(DXGI_FORMAT_UNKNOWN), mLoadImageFunction(NULL)
    { }

    D3D11ES2FormatInfo(DXGI_FORMAT texFormat, DXGI_FORMAT srvFormat, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat,
        LoadImageFunction loadFunc)
        : mTexFormat(texFormat), mDSVFormat(dsvFormat), mRTVFormat(rtvFormat), mSRVFormat(srvFormat),
        mLoadImageFunction(loadFunc)
    { }
};

// ES2 internal formats can map to DXGI formats and loading functions
typedef std::pair<GLenum, D3D11ES2FormatInfo> D3D11ES2FormatPair;
typedef std::map<GLenum, D3D11ES2FormatInfo> D3D11ES2FormatMap;

static D3D11ES2FormatMap BuildD3D11ES2FormatMap()
{
    D3D11ES2FormatMap map;

    //                           | Internal format                   |                  | Texture format                | SRV format                       | RTV format                    | DSV format                   | Load function                           |
    map.insert(D3D11ES2FormatPair(GL_NONE,                            D3D11ES2FormatInfo(DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,               DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,           UnreachableLoadFunction                  )));
    map.insert(D3D11ES2FormatPair(GL_DEPTH_COMPONENT16,               D3D11ES2FormatInfo(DXGI_FORMAT_R16_TYPELESS,       DXGI_FORMAT_R16_UNORM,             DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D16_UNORM,         UnreachableLoadFunction                  )));
    map.insert(D3D11ES2FormatPair(GL_DEPTH_COMPONENT32_OES,           D3D11ES2FormatInfo(DXGI_FORMAT_R32_TYPELESS,       DXGI_FORMAT_R32_FLOAT,             DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D32_FLOAT,         UnreachableLoadFunction                  )));
    map.insert(D3D11ES2FormatPair(GL_DEPTH24_STENCIL8_OES,            D3D11ES2FormatInfo(DXGI_FORMAT_R24G8_TYPELESS,     DXGI_FORMAT_R24_UNORM_X8_TYPELESS, DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT, UnreachableLoadFunction                  )));
    map.insert(D3D11ES2FormatPair(GL_STENCIL_INDEX8,                  D3D11ES2FormatInfo(DXGI_FORMAT_R24G8_TYPELESS,     DXGI_FORMAT_X24_TYPELESS_G8_UINT,  DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_D24_UNORM_S8_UINT, UnreachableLoadFunction                  )));

    map.insert(D3D11ES2FormatPair(GL_RGBA32F_EXT,                     D3D11ES2FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,    DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN,           loadRGBAFloatDataToRGBA                  )));
    map.insert(D3D11ES2FormatPair(GL_RGB32F_EXT,                      D3D11ES2FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,    DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN,           loadRGBFloatDataToRGBA                   )));
    map.insert(D3D11ES2FormatPair(GL_ALPHA32F_EXT,                    D3D11ES2FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,    DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN,           loadAlphaFloatDataToRGBA                 )));
    map.insert(D3D11ES2FormatPair(GL_LUMINANCE32F_EXT,                D3D11ES2FormatInfo(DXGI_FORMAT_R32G32B32_FLOAT,    DXGI_FORMAT_R32G32B32_FLOAT,       DXGI_FORMAT_R32G32B32_FLOAT,    DXGI_FORMAT_UNKNOWN,           loadLuminanceFloatDataToRGB              )));
    map.insert(D3D11ES2FormatPair(GL_LUMINANCE_ALPHA32F_EXT,          D3D11ES2FormatInfo(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT,    DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN,           loadLuminanceAlphaFloatDataToRGBA        )));

    map.insert(D3D11ES2FormatPair(GL_RGBA16F_EXT,                     D3D11ES2FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,    DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN,           loadRGBAHalfFloatDataToRGBA              )));
    map.insert(D3D11ES2FormatPair(GL_RGB16F_EXT,                      D3D11ES2FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,    DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN,           loadRGBHalfFloatDataToRGBA               )));
    map.insert(D3D11ES2FormatPair(GL_ALPHA16F_EXT,                    D3D11ES2FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,    DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN,           loadAlphaHalfFloatDataToRGBA             )));
    map.insert(D3D11ES2FormatPair(GL_LUMINANCE16F_EXT,                D3D11ES2FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,    DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN,           loadLuminanceHalfFloatDataToRGBA         )));
    map.insert(D3D11ES2FormatPair(GL_LUMINANCE_ALPHA16F_EXT,          D3D11ES2FormatInfo(DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,    DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_UNKNOWN,           loadLuminanceAlphaHalfFloatDataToRGBA    )));

    map.insert(D3D11ES2FormatPair(GL_ALPHA8_EXT,                      D3D11ES2FormatInfo(DXGI_FORMAT_A8_UNORM,           DXGI_FORMAT_A8_UNORM,              DXGI_FORMAT_A8_UNORM,           DXGI_FORMAT_UNKNOWN,           loadAlphaDataToNative                    )));
    map.insert(D3D11ES2FormatPair(GL_LUMINANCE8_EXT,                  D3D11ES2FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadLuminanceDataToBGRA                  )));
    map.insert(D3D11ES2FormatPair(GL_LUMINANCE8_ALPHA8_EXT,           D3D11ES2FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadLuminanceAlphaDataToBGRA             )));

    map.insert(D3D11ES2FormatPair(GL_RGB8_OES,                        D3D11ES2FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadRGBUByteDataToRGBA                   )));
    map.insert(D3D11ES2FormatPair(GL_RGB565,                          D3D11ES2FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadRGB565DataToRGBA                     )));
    map.insert(D3D11ES2FormatPair(GL_RGBA8_OES,                       D3D11ES2FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadRGBAUByteDataToNative                )));
    map.insert(D3D11ES2FormatPair(GL_RGBA4,                           D3D11ES2FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadRGBA4444DataToRGBA                   )));
    map.insert(D3D11ES2FormatPair(GL_RGB5_A1,                         D3D11ES2FormatInfo(DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_R8G8B8A8_UNORM,        DXGI_FORMAT_R8G8B8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadRGBA5551DataToRGBA                   )));
    map.insert(D3D11ES2FormatPair(GL_BGRA8_EXT,                       D3D11ES2FormatInfo(DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,        DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadBGRADataToBGRA                       )));
    map.insert(D3D11ES2FormatPair(GL_BGRA4_ANGLEX,                    D3D11ES2FormatInfo(DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,        DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadRGBA4444DataToRGBA                   )));
    map.insert(D3D11ES2FormatPair(GL_BGR5_A1_ANGLEX,                  D3D11ES2FormatInfo(DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_B8G8R8A8_UNORM,        DXGI_FORMAT_B8G8R8A8_UNORM,     DXGI_FORMAT_UNKNOWN,           loadRGBA5551DataToRGBA                   )));

    map.insert(D3D11ES2FormatPair(GL_COMPRESSED_RGB_S3TC_DXT1_EXT,    D3D11ES2FormatInfo(DXGI_FORMAT_BC1_UNORM,          DXGI_FORMAT_BC1_UNORM,             DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,           loadCompressedBlockDataToNative<4, 4,  8>)));
    map.insert(D3D11ES2FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   D3D11ES2FormatInfo(DXGI_FORMAT_BC1_UNORM,          DXGI_FORMAT_BC1_UNORM,             DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,           loadCompressedBlockDataToNative<4, 4,  8>)));
    map.insert(D3D11ES2FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, D3D11ES2FormatInfo(DXGI_FORMAT_BC2_UNORM,          DXGI_FORMAT_BC2_UNORM,             DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,           loadCompressedBlockDataToNative<4, 4, 16>)));
    map.insert(D3D11ES2FormatPair(GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, D3D11ES2FormatInfo(DXGI_FORMAT_BC3_UNORM,          DXGI_FORMAT_BC3_UNORM,             DXGI_FORMAT_UNKNOWN,            DXGI_FORMAT_UNKNOWN,           loadCompressedBlockDataToNative<4, 4, 16>)));

    return map;
}

static bool GetD3D11ES2FormatInfo(GLenum internalFormat, GLuint clientVersion, D3D11ES2FormatInfo *outFormatInfo)
{
    static const D3D11ES2FormatMap formatMap = BuildD3D11ES2FormatMap();
    D3D11ES2FormatMap::const_iterator iter = formatMap.find(internalFormat);
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

// A map to determine the pixel size and mipmap generation function of a given DXGI format
struct DXGIFormatInfo
{
    GLuint mPixelBits;
    GLuint mBlockWidth;
    GLuint mBlockHeight;
    GLenum mInternalFormat;
    GLuint mClientVersion;

    MipGenerationFunction mMipGenerationFunction;
    ColorReadFunction mColorReadFunction;

    DXGIFormatInfo()
        : mPixelBits(0), mBlockWidth(0), mBlockHeight(0), mInternalFormat(GL_NONE), mMipGenerationFunction(NULL),
          mColorReadFunction(NULL), mClientVersion(0)
    { }

    DXGIFormatInfo(GLuint pixelBits, GLuint blockWidth, GLuint blockHeight, GLenum internalFormat,
                   MipGenerationFunction mipFunc, ColorReadFunction readFunc, GLuint clientVersion)
        : mPixelBits(pixelBits), mBlockWidth(blockWidth), mBlockHeight(blockHeight), mInternalFormat(internalFormat),
          mMipGenerationFunction(mipFunc), mColorReadFunction(readFunc), mClientVersion(clientVersion)
    { }
};

typedef std::pair<DXGI_FORMAT, DXGIFormatInfo> DXGIFormatInfoPair;
typedef std::map<DXGI_FORMAT, DXGIFormatInfo> DXGIFormatInfoMap;

static DXGIFormatInfoMap BuildCommonDXGIFormatInfoMap(GLuint clientVersion)
{
    DXGIFormatInfoMap map;

    //                           | DXGI format                                   | S  |W |H | Internal format                   | Mip generation function  | Color read function              | Version |
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_UNKNOWN,             DXGIFormatInfo(  0, 0, 0, GL_NONE,                            NULL,                       NULL,                              clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_A8_UNORM,            DXGIFormatInfo(  8, 1, 1, GL_ALPHA8_EXT,                      GenerateMip<A8>,            ReadColor<A8, GLfloat>,            clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8_UNORM,            DXGIFormatInfo(  8, 1, 1, GL_R8,                              GenerateMip<R8>,            ReadColor<R8, GLfloat>,            clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8_UNORM,          DXGIFormatInfo( 16, 1, 1, GL_RG8,                             GenerateMip<R8G8>,          ReadColor<R8G8, GLfloat>,          clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8B8A8_UNORM,      DXGIFormatInfo( 32, 1, 1, GL_RGBA8,                           GenerateMip<R8G8B8A8>,      ReadColor<R8G8B8A8, GLfloat>,      clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGIFormatInfo( 32, 1, 1, GL_SRGB8_ALPHA8,                    GenerateMip<R8G8B8A8>,      ReadColor<R8G8B8A8, GLfloat>,      clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_B8G8R8A8_UNORM,      DXGIFormatInfo( 32, 1, 1, GL_BGRA8_EXT,                       GenerateMip<B8G8R8A8>,      ReadColor<B8G8R8A8, GLfloat>,      clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8_SNORM,            DXGIFormatInfo(  8, 1, 1, GL_R8_SNORM,                        GenerateMip<R8S>,           ReadColor<R8S, GLfloat>,           clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8_SNORM,          DXGIFormatInfo( 16, 1, 1, GL_RG8_SNORM,                       GenerateMip<R8G8S>,         ReadColor<R8G8S, GLfloat>,         clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8B8A8_SNORM,      DXGIFormatInfo( 32, 1, 1, GL_RGBA8_SNORM,                     GenerateMip<R8G8B8A8S>,     ReadColor<R8G8B8A8S, GLfloat>,     clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8_UINT,             DXGIFormatInfo(  8, 1, 1, GL_R8UI,                            GenerateMip<R8>,            ReadColor<R8, GLuint>,             clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16_UINT,            DXGIFormatInfo( 16, 1, 1, GL_R16UI,                           GenerateMip<R16>,           ReadColor<R16, GLuint>,            clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32_UINT,            DXGIFormatInfo( 32, 1, 1, GL_R32UI,                           GenerateMip<R32>,           ReadColor<R32, GLuint>,            clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8_UINT,           DXGIFormatInfo( 16, 1, 1, GL_RG8UI,                           GenerateMip<R8G8>,          ReadColor<R8G8, GLuint>,           clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16G16_UINT,         DXGIFormatInfo( 32, 1, 1, GL_RG16UI,                          GenerateMip<R16G16>,        ReadColor<R16G16, GLuint>,         clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32_UINT,         DXGIFormatInfo( 64, 1, 1, GL_RG32UI,                          GenerateMip<R32G32>,        ReadColor<R32G32, GLuint>,         clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32B32_UINT,      DXGIFormatInfo( 96, 1, 1, GL_RGB32UI,                         GenerateMip<R32G32B32>,     ReadColor<R32G32B32, GLuint>,      clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8B8A8_UINT,       DXGIFormatInfo( 32, 1, 1, GL_RGBA8UI,                         GenerateMip<R8G8B8A8>,      ReadColor<R8G8B8A8, GLuint>,       clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16G16B16A16_UINT,   DXGIFormatInfo( 64, 1, 1, GL_RGBA16UI,                        GenerateMip<R16G16B16A16>,  ReadColor<R16G16B16A16, GLuint>,   clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32B32A32_UINT,   DXGIFormatInfo(128, 1, 1, GL_RGBA32UI,                        GenerateMip<R32G32B32A32>,  ReadColor<R32G32B32A32, GLuint>,   clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8_SINT,             DXGIFormatInfo(  8, 1, 1, GL_R8I,                             GenerateMip<R8S>,           ReadColor<R8S, GLint>,             clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16_SINT,            DXGIFormatInfo( 16, 1, 1, GL_R16I,                            GenerateMip<R16S>,          ReadColor<R16S, GLint>,            clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32_SINT,            DXGIFormatInfo( 32, 1, 1, GL_R32I,                            GenerateMip<R32S>,          ReadColor<R32S, GLint>,            clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8_SINT,           DXGIFormatInfo( 16, 1, 1, GL_RG8I,                            GenerateMip<R8G8S>,         ReadColor<R8G8S, GLint>,           clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16G16_SINT,         DXGIFormatInfo( 32, 1, 1, GL_RG16I,                           GenerateMip<R16G16S>,       ReadColor<R16G16S, GLint>,         clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32_SINT,         DXGIFormatInfo( 64, 1, 1, GL_RG32I,                           GenerateMip<R32G32S>,       ReadColor<R32G32S, GLint>,         clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32B32_SINT,      DXGIFormatInfo( 96, 1, 1, GL_RGB32I,                          GenerateMip<R32G32B32S>,    ReadColor<R32G32B32S, GLint>,      clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R8G8B8A8_SINT,       DXGIFormatInfo( 32, 1, 1, GL_RGBA8I,                          GenerateMip<R8G8B8A8S>,     ReadColor<R8G8B8A8S, GLint>,       clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16G16B16A16_SINT,   DXGIFormatInfo( 64, 1, 1, GL_RGBA16I,                         GenerateMip<R16G16B16A16S>, ReadColor<R16G16B16A16S, GLint>,   clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32B32A32_SINT,   DXGIFormatInfo(128, 1, 1, GL_RGBA32I,                         GenerateMip<R32G32B32A32S>, ReadColor<R32G32B32A32S, GLint>,   clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R10G10B10A2_UNORM,   DXGIFormatInfo( 32, 1, 1, GL_RGB10_A2,                        GenerateMip<R10G10B10A2>,   ReadColor<R10G10B10A2, GLfloat>,   clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R10G10B10A2_UINT,    DXGIFormatInfo( 32, 1, 1, GL_RGB10_A2UI,                      GenerateMip<R10G10B10A2>,   ReadColor<R10G10B10A2, GLuint>,    clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16_FLOAT,           DXGIFormatInfo( 16, 1, 1, GL_R16F,                            GenerateMip<R16F>,          ReadColor<R16F, GLfloat>,          clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16G16_FLOAT,        DXGIFormatInfo( 32, 1, 1, GL_RG16F,                           GenerateMip<R16G16F>,       ReadColor<R16G16F, GLfloat>,       clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16G16B16A16_FLOAT,  DXGIFormatInfo( 64, 1, 1, GL_RGBA16F,                         GenerateMip<R16G16B16A16F>, ReadColor<R16G16B16A16F, GLfloat>, clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32_FLOAT,           DXGIFormatInfo( 32, 1, 1, GL_R32F,                            GenerateMip<R32F>,          ReadColor<R32F, GLfloat>,          clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32_FLOAT,        DXGIFormatInfo( 64, 1, 1, GL_RG32F,                           GenerateMip<R32G32F>,       ReadColor<R32G32F, GLfloat>,       clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32B32_FLOAT,     DXGIFormatInfo( 96, 1, 1, GL_RGB32F,                          GenerateMip<R32G32B32F>,    ReadColor<R32G32B32F, GLfloat>,    clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G32B32A32_FLOAT,  DXGIFormatInfo(128, 1, 1, GL_RGBA32F,                         GenerateMip<R32G32B32A32F>, ReadColor<R32G32B32A32F, GLfloat>, clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R9G9B9E5_SHAREDEXP,  DXGIFormatInfo( 32, 1, 1, GL_RGB9_E5,                         GenerateMip<R9G9B9E5>,      ReadColor<R9G9B9E5, GLfloat>,      clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R11G11B10_FLOAT,     DXGIFormatInfo( 32, 1, 1, GL_R11F_G11F_B10F,                  GenerateMip<R11G11B10F>,    ReadColor<R11G11B10F, GLfloat>,    clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16_TYPELESS,             DXGIFormatInfo( 16, 1, 1, GL_DEPTH_COMPONENT16,          NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R16_UNORM,                DXGIFormatInfo( 16, 1, 1, GL_DEPTH_COMPONENT16,          NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_D16_UNORM,                DXGIFormatInfo( 16, 1, 1, GL_DEPTH_COMPONENT16,          NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R24G8_TYPELESS,           DXGIFormatInfo( 32, 1, 1, GL_DEPTH24_STENCIL8_OES,       NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGIFormatInfo( 32, 1, 1, GL_DEPTH24_STENCIL8_OES,       NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_D24_UNORM_S8_UINT,        DXGIFormatInfo( 32, 1, 1, GL_DEPTH24_STENCIL8_OES,       NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32G8X24_TYPELESS,        DXGIFormatInfo( 64, 1, 1, GL_DEPTH32F_STENCIL8,          NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGIFormatInfo( 64, 1, 1, GL_DEPTH32F_STENCIL8,          NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     DXGIFormatInfo( 64, 1, 1, GL_DEPTH32F_STENCIL8,          NULL,                       NULL,                              clientVersion)));

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_BC1_UNORM,           DXGIFormatInfo( 64, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT,   NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_BC2_UNORM,           DXGIFormatInfo(128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE, NULL,                       NULL,                              clientVersion)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_BC3_UNORM,           DXGIFormatInfo(128, 4, 4, GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE, NULL,                       NULL,                              clientVersion)));

    return map;
}

static DXGIFormatInfoMap BuildES2DXGIFormatInfoMap()
{
    DXGIFormatInfoMap map = BuildCommonDXGIFormatInfoMap(2);

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32_TYPELESS,             DXGIFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT32_OES,      NULL,                       NULL,                              2)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_D32_FLOAT,                DXGIFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT32_OES,      NULL,                       NULL,                              2)));

    return map;
}

static DXGIFormatInfoMap BuildES3DXGIFormatInfoMap()
{
    DXGIFormatInfoMap map = BuildCommonDXGIFormatInfoMap(3);

    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_R32_TYPELESS,             DXGIFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT32F,         NULL,                       NULL,                              3)));
    map.insert(DXGIFormatInfoPair(DXGI_FORMAT_D32_FLOAT,                DXGIFormatInfo( 32, 1, 1, GL_DEPTH_COMPONENT32F,         NULL,                       NULL,                              3)));

    return map;
}

static const DXGIFormatInfoMap &GetDXGIFormatInfoMap(GLuint clientVersion)
{
    ASSERT(clientVersion == 2 || clientVersion == 3);

    if (clientVersion == 2)
    {
        static const DXGIFormatInfoMap infoMapES2 = BuildES2DXGIFormatInfoMap();
        return infoMapES2;
    }
    else
    {
        static const DXGIFormatInfoMap infoMapES3 = BuildES3DXGIFormatInfoMap();
        return infoMapES3;
    }
}

static bool GetDXGIFormatInfo(DXGI_FORMAT format, GLuint clientVersion, DXGIFormatInfo *outFormatInfo)
{
    const DXGIFormatInfoMap &infoMap = GetDXGIFormatInfoMap(clientVersion);
    DXGIFormatInfoMap::const_iterator iter = infoMap.find(format);
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

static d3d11::DXGIFormatSet BuildAllDXGIFormatSet()
{
    d3d11::DXGIFormatSet set;

    for (GLuint clientVersion = 2; clientVersion <= 3; clientVersion++)
    {
        const DXGIFormatInfoMap &infoMap = GetDXGIFormatInfoMap(clientVersion);
        for (DXGIFormatInfoMap::const_iterator i = infoMap.begin(); i != infoMap.end(); ++i)
        {
            set.insert(i->first);
        }
    }

    return set;
}

struct D3D11FastCopyFormat
{
    DXGI_FORMAT mSourceFormat;
    GLenum mDestFormat;
    GLenum mDestType;

    D3D11FastCopyFormat(DXGI_FORMAT sourceFormat, GLenum destFormat, GLenum destType)
        : mSourceFormat(sourceFormat), mDestFormat(destFormat), mDestType(destType)
    { }

    bool operator<(const D3D11FastCopyFormat& other) const
    {
        return memcmp(this, &other, sizeof(D3D11FastCopyFormat)) < 0;
    }
};

typedef std::map<D3D11FastCopyFormat, ColorCopyFunction> D3D11FastCopyMap;
typedef std::pair<D3D11FastCopyFormat, ColorCopyFunction> D3D11FastCopyPair;

static D3D11FastCopyMap BuildFastCopyMap()
{
    D3D11FastCopyMap map;

    map.insert(D3D11FastCopyPair(D3D11FastCopyFormat(DXGI_FORMAT_B8G8R8A8_UNORM, GL_RGBA, GL_UNSIGNED_BYTE), CopyBGRAUByteToRGBAUByte));

    return map;
}

struct DXGIDepthStencilInfo
{
    unsigned int mDepthBits;
    unsigned int mDepthOffset;
    unsigned int mStencilBits;
    unsigned int mStencilOffset;

    DXGIDepthStencilInfo()
        : mDepthBits(0), mDepthOffset(0), mStencilBits(0), mStencilOffset(0)
    { }

    DXGIDepthStencilInfo(unsigned int depthBits, unsigned int depthOffset, unsigned int stencilBits, unsigned int stencilOffset)
        : mDepthBits(depthBits), mDepthOffset(depthOffset), mStencilBits(stencilBits), mStencilOffset(stencilOffset)
    { }
};

typedef std::map<DXGI_FORMAT, DXGIDepthStencilInfo> DepthStencilInfoMap;
typedef std::pair<DXGI_FORMAT, DXGIDepthStencilInfo> DepthStencilInfoPair;

static DepthStencilInfoMap BuildDepthStencilInfoMap()
{
    DepthStencilInfoMap map;

    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R16_TYPELESS,             DXGIDepthStencilInfo(16, 0, 0,  0)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R16_UNORM,                DXGIDepthStencilInfo(16, 0, 0,  0)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_D16_UNORM,                DXGIDepthStencilInfo(16, 0, 0,  0)));

    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R24G8_TYPELESS,           DXGIDepthStencilInfo(24, 0, 8, 24)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGIDepthStencilInfo(24, 0, 8, 24)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_D24_UNORM_S8_UINT,        DXGIDepthStencilInfo(24, 0, 8, 24)));

    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R32_TYPELESS,             DXGIDepthStencilInfo(32, 0, 0,  0)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R32_FLOAT,                DXGIDepthStencilInfo(32, 0, 0,  0)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_D32_FLOAT,                DXGIDepthStencilInfo(32, 0, 0,  0)));

    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R32G8X24_TYPELESS,        DXGIDepthStencilInfo(32, 0, 8, 32)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGIDepthStencilInfo(32, 0, 8, 32)));
    map.insert(DepthStencilInfoPair(DXGI_FORMAT_D32_FLOAT_S8X24_UINT,     DXGIDepthStencilInfo(32, 0, 8, 32)));

    return map;
}

static const DepthStencilInfoMap &GetDepthStencilInfoMap()
{
    static const DepthStencilInfoMap infoMap = BuildDepthStencilInfoMap();
    return infoMap;
}

bool GetDepthStencilInfo(DXGI_FORMAT format, DXGIDepthStencilInfo *outDepthStencilInfo)
{
    const DepthStencilInfoMap& infoMap = GetDepthStencilInfoMap();
    DepthStencilInfoMap::const_iterator iter = infoMap.find(format);
    if (iter != infoMap.end())
    {
        if (outDepthStencilInfo)
        {
            *outDepthStencilInfo = iter->second;
        }
        return true;
    }
    else
    {
        return false;
    }
}

namespace d3d11
{

MipGenerationFunction GetMipGenerationFunction(DXGI_FORMAT format, GLuint clientVersion)
{
    DXGIFormatInfo formatInfo;
    if (GetDXGIFormatInfo(format, clientVersion, &formatInfo))
    {
        return formatInfo.mMipGenerationFunction;
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

LoadImageFunction GetImageLoadFunction(GLenum internalFormat, GLenum type, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        D3D11ES2FormatInfo d3d11FormatInfo;
        if (GetD3D11ES2FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mLoadImageFunction;
        }
        else
        {
            UNREACHABLE();
            return NULL;
        }
    }
    else if (clientVersion == 3)
    {
        static const D3D11LoadFunctionMap loadImageMap = buildD3D11LoadFunctionMap();
        D3D11LoadFunctionMap::const_iterator iter = loadImageMap.find(InternalFormatTypePair(internalFormat, type));
        if (iter != loadImageMap.end())
        {
            return iter->second;
        }
        else
        {
            UNREACHABLE();
            return NULL;
        }
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

GLuint GetFormatPixelBytes(DXGI_FORMAT format, GLuint clientVersion)
{
    DXGIFormatInfo dxgiFormatInfo;
    if (GetDXGIFormatInfo(format, clientVersion, &dxgiFormatInfo))
    {
        return dxgiFormatInfo.mPixelBits / 8;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockWidth(DXGI_FORMAT format, GLuint clientVersion)
{
    DXGIFormatInfo dxgiFormatInfo;
    if (GetDXGIFormatInfo(format, clientVersion, &dxgiFormatInfo))
    {
        return dxgiFormatInfo.mBlockWidth;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetBlockHeight(DXGI_FORMAT format, GLuint clientVersion)
{
    DXGIFormatInfo dxgiFormatInfo;
    if (GetDXGIFormatInfo(format, clientVersion, &dxgiFormatInfo))
    {
        return dxgiFormatInfo.mBlockHeight;
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

GLuint GetDepthBits(DXGI_FORMAT format)
{
    DXGIDepthStencilInfo dxgiDSInfo;
    if (GetDepthStencilInfo(format, &dxgiDSInfo))
    {
        return dxgiDSInfo.mDepthBits;
    }
    else
    {
        // Since the depth stencil info map does not contain all used DXGI formats,
        // we should not assert that the format exists
        return 0;
    }
}

GLuint GetDepthOffset(DXGI_FORMAT format)
{
    DXGIDepthStencilInfo dxgiDSInfo;
    if (GetDepthStencilInfo(format, &dxgiDSInfo))
    {
        return dxgiDSInfo.mDepthOffset;
    }
    else
    {
        // Since the depth stencil info map does not contain all used DXGI formats,
        // we should not assert that the format exists
        return 0;
    }
}

GLuint GetStencilBits(DXGI_FORMAT format)
{
    DXGIDepthStencilInfo dxgiDSInfo;
    if (GetDepthStencilInfo(format, &dxgiDSInfo))
    {
        return dxgiDSInfo.mStencilBits;
    }
    else
    {
        // Since the depth stencil info map does not contain all used DXGI formats,
        // we should not assert that the format exists
        return 0;
    }
}

GLuint GetStencilOffset(DXGI_FORMAT format)
{
    DXGIDepthStencilInfo dxgiDSInfo;
    if (GetDepthStencilInfo(format, &dxgiDSInfo))
    {
        return dxgiDSInfo.mStencilOffset;
    }
    else
    {
        // Since the depth stencil info map does not contain all used DXGI formats,
        // we should not assert that the format exists
        return 0;
    }
}

void MakeValidSize(bool isImage, DXGI_FORMAT format, GLuint clientVersion, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset)
{
    DXGIFormatInfo dxgiFormatInfo;
    if (GetDXGIFormatInfo(format, clientVersion, &dxgiFormatInfo))
    {
        int upsampleCount = 0;

        GLsizei blockWidth = dxgiFormatInfo.mBlockWidth;
        GLsizei blockHeight = dxgiFormatInfo.mBlockHeight;

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
    else
    {
        UNREACHABLE();
    }
}

const DXGIFormatSet &GetAllUsedDXGIFormats()
{
    static DXGIFormatSet formatSet = BuildAllDXGIFormatSet();
    return formatSet;
}

ColorReadFunction GetColorReadFunction(DXGI_FORMAT format, GLuint clientVersion)
{
    DXGIFormatInfo dxgiFormatInfo;
    if (GetDXGIFormatInfo(format, clientVersion, &dxgiFormatInfo))
    {
        return dxgiFormatInfo.mColorReadFunction;
    }
    else
    {
        UNREACHABLE();
        return NULL;
    }
}

ColorCopyFunction GetFastCopyFunction(DXGI_FORMAT sourceFormat, GLenum destFormat, GLenum destType, GLuint clientVersion)
{
    static const D3D11FastCopyMap fastCopyMap = BuildFastCopyMap();
    D3D11FastCopyMap::const_iterator iter = fastCopyMap.find(D3D11FastCopyFormat(sourceFormat, destFormat, destType));
    return (iter != fastCopyMap.end()) ? iter->second : NULL;
}

}

namespace gl_d3d11
{

DXGI_FORMAT GetTexFormat(GLenum internalFormat, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        D3D11ES2FormatInfo d3d11FormatInfo;
        if (GetD3D11ES2FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mTexFormat;
        }
        else
        {
            UNREACHABLE();
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else if (clientVersion == 3)
    {
        D3D11ES3FormatInfo d3d11FormatInfo;
        if (GetD3D11ES3FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mTexFormat;
        }
        else
        {
            UNREACHABLE();
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else
    {
        UNREACHABLE();
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT GetSRVFormat(GLenum internalFormat, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        D3D11ES2FormatInfo d3d11FormatInfo;
        if (GetD3D11ES2FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mSRVFormat;
        }
        else
        {
            UNREACHABLE();
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else if (clientVersion == 3)
    {
        D3D11ES3FormatInfo d3d11FormatInfo;
        if (GetD3D11ES3FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mSRVFormat;
        }
        else
        {
            UNREACHABLE();
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else
    {
        UNREACHABLE();
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT GetRTVFormat(GLenum internalFormat, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        D3D11ES2FormatInfo d3d11FormatInfo;
        if (GetD3D11ES2FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mRTVFormat;
        }
        else
        {
            UNREACHABLE();
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else if (clientVersion == 3)
    {
        D3D11ES3FormatInfo d3d11FormatInfo;
        if (GetD3D11ES3FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mRTVFormat;
        }
        else
        {
            UNREACHABLE();
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else
    {
        UNREACHABLE();
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT GetDSVFormat(GLenum internalFormat, GLuint clientVersion)
{
    if (clientVersion == 2)
    {
        D3D11ES2FormatInfo d3d11FormatInfo;
        if (GetD3D11ES2FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mDSVFormat;
        }
        else
        {
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else if (clientVersion == 3)
    {
        D3D11ES3FormatInfo d3d11FormatInfo;
        if (GetD3D11ES3FormatInfo(internalFormat, clientVersion, &d3d11FormatInfo))
        {
            return d3d11FormatInfo.mDSVFormat;
        }
        else
        {
            return DXGI_FORMAT_UNKNOWN;
        }
    }
    else
    {
        UNREACHABLE();
        return DXGI_FORMAT_UNKNOWN;
    }
}

// Given a GL internal format, this function returns the DSV format if it is depth- or stencil-renderable,
// the RTV format if it is color-renderable, and the (nonrenderable) texture format otherwise.
DXGI_FORMAT GetRenderableFormat(GLenum internalFormat, GLuint clientVersion)
{
    DXGI_FORMAT targetFormat = GetDSVFormat(internalFormat, clientVersion);
    if (targetFormat == DXGI_FORMAT_UNKNOWN)
        targetFormat = GetRTVFormat(internalFormat, clientVersion);
    if (targetFormat == DXGI_FORMAT_UNKNOWN)
        targetFormat = GetTexFormat(internalFormat, clientVersion);

    return targetFormat;
}

}

namespace d3d11_gl
{

GLenum GetInternalFormat(DXGI_FORMAT format, GLuint clientVersion)
{
    DXGIFormatInfo formatInfo;
    if (GetDXGIFormatInfo(format, clientVersion, &formatInfo))
    {
        return formatInfo.mInternalFormat;
    }
    else
    {
        UNREACHABLE();
        return GL_NONE;
    }
}

}

}
