//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Helper routines for the D3D11 texture format table.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_TEXTURE_FORMAT_TABLE_UTILS_H_
#define LIBANGLE_RENDERER_D3D_D3D11_TEXTURE_FORMAT_TABLE_UTILS_H_

#include "libANGLE/renderer/d3d/d3d11/Renderer11.h"

namespace rx
{

namespace d3d11
{

using FormatSupportFunction = bool (*)(const Renderer11DeviceCaps &);

inline bool OnlyFL10Plus(const Renderer11DeviceCaps &deviceCaps)
{
    return (deviceCaps.featureLevel >= D3D_FEATURE_LEVEL_10_0);
}

inline bool OnlyFL9_3(const Renderer11DeviceCaps &deviceCaps)
{
    return (deviceCaps.featureLevel == D3D_FEATURE_LEVEL_9_3);
}

template <DXGI_FORMAT format, bool requireSupport>
bool SupportsFormat(const Renderer11DeviceCaps &deviceCaps)
{
    // Must support texture, SRV and RTV support
    UINT mustSupport = D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_TEXTURECUBE |
                       D3D11_FORMAT_SUPPORT_SHADER_SAMPLE | D3D11_FORMAT_SUPPORT_MIP |
                       D3D11_FORMAT_SUPPORT_RENDER_TARGET;

    if (d3d11_gl::GetMaximumClientVersion(deviceCaps.featureLevel) > 2)
    {
        mustSupport |= D3D11_FORMAT_SUPPORT_TEXTURE3D;
    }

    bool fullSupport = false;
    if (format == DXGI_FORMAT_B5G6R5_UNORM)
    {
        // All hardware that supports DXGI_FORMAT_B5G6R5_UNORM should support autogen mipmaps, but
        // check anyway.
        mustSupport |= D3D11_FORMAT_SUPPORT_MIP_AUTOGEN;
        fullSupport = ((deviceCaps.B5G6R5support & mustSupport) == mustSupport);
    }
    else if (format == DXGI_FORMAT_B4G4R4A4_UNORM)
    {
        fullSupport = ((deviceCaps.B4G4R4A4support & mustSupport) == mustSupport);
    }
    else if (format == DXGI_FORMAT_B5G5R5A1_UNORM)
    {
        fullSupport = ((deviceCaps.B5G5R5A1support & mustSupport) == mustSupport);
    }
    else
    {
        UNREACHABLE();
        return false;
    }

    // This 'SupportsFormat' function is used by individual entries in the D3D11 Format Map below,
    // which maps GL formats to DXGI formats.
    if (requireSupport)
    {
        // This means that ANGLE would like to use the entry in the map if the inputted DXGI format
        // *IS* supported.
        // e.g. the entry might map GL_RGB5_A1 to DXGI_FORMAT_B5G5R5A1, which should only be used if
        // DXGI_FORMAT_B5G5R5A1 is supported.
        // In this case, we should only return 'true' if the format *IS* supported.
        return fullSupport;
    }
    else
    {
        // This means that ANGLE would like to use the entry in the map if the inputted DXGI format
        // *ISN'T* supported.
        // This might be a fallback entry. e.g. for ANGLE to use DXGI_FORMAT_R8G8B8A8_UNORM if
        // DXGI_FORMAT_B5G5R5A1 isn't supported.
        // In this case, we should only return 'true' if the format *ISN'T* supported.
        return !fullSupport;
    }
}

}  // namespace d3d11

}  // namespace rx

#endif  // LIBANGLE_RENDERER_D3D_D3D11_TEXTURE_FORMAT_TABLE_UTILS_H_
