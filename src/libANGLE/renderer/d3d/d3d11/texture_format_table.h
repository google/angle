//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// texture_format_table:
//   Queries for full textureFormat information based on internalFormat
//

#ifndef LIBANGLE_RENDERER_D3D_D3D11_TEXTUREFORMATTABLE_H_
#define LIBANGLE_RENDERER_D3D_D3D11_TEXTUREFORMATTABLE_H_

#include <map>

#include "common/angleutils.h"
#include "common/platform.h"
#include "libANGLE/renderer/Format.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/d3d/formatutilsD3D.h"

namespace rx
{

struct Renderer11DeviceCaps;

namespace d3d11
{

struct LoadImageFunctionInfo
{
    LoadImageFunctionInfo() : loadFunction(nullptr), requiresConversion(false) {}
    LoadImageFunctionInfo(LoadImageFunction loadFunction, bool requiresConversion)
        : loadFunction(loadFunction), requiresConversion(requiresConversion)
    {
    }

    LoadImageFunction loadFunction;
    bool requiresConversion;
};

struct ANGLEFormatSet final : angle::NonCopyable
{
    ANGLEFormatSet();
    ANGLEFormatSet(angle::Format::ID formatID,
                   DXGI_FORMAT texFormat,
                   DXGI_FORMAT srvFormat,
                   DXGI_FORMAT rtvFormat,
                   DXGI_FORMAT dsvFormat,
                   DXGI_FORMAT blitSRVFormat,
                   angle::Format::ID swizzleFormat);

    const angle::Format &format;

    DXGI_FORMAT texFormat;
    DXGI_FORMAT srvFormat;
    DXGI_FORMAT rtvFormat;
    DXGI_FORMAT dsvFormat;

    DXGI_FORMAT blitSRVFormat;

    angle::Format::ID swizzleFormat;
};

struct TextureFormat : public angle::NonCopyable
{
    TextureFormat(GLenum internalFormat,
                  const angle::Format::ID angleFormatID,
                  InitializeTextureDataFunction internalFormatInitializer,
                  const Renderer11DeviceCaps &deviceCaps);

    GLenum internalFormat;
    const ANGLEFormatSet &formatSet;
    const ANGLEFormatSet &swizzleFormatSet;

    InitializeTextureDataFunction dataInitializerFunction;
    typedef std::map<GLenum, LoadImageFunctionInfo> LoadFunctionMap;

    LoadFunctionMap loadFunctions;
};

const ANGLEFormatSet &GetANGLEFormatSet(angle::Format::ID angleFormat,
                                        const Renderer11DeviceCaps &deviceCaps);

const TextureFormat &GetTextureFormatInfo(GLenum internalformat,
                                          const Renderer11DeviceCaps &renderer11DeviceCaps);

}  // namespace d3d11

}  // namespace rx

#endif  // LIBANGLE_RENDERER_D3D_D3D11_TEXTUREFORMATTABLE_H_
