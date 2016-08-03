//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Helper routines for the D3D11 texture format table.

#include "libANGLE/renderer/d3d/d3d11/texture_format_table.h"

#include "libANGLE/renderer/d3d/d3d11/load_functions_table.h"

namespace rx
{

namespace d3d11
{

ANGLEFormatSet::ANGLEFormatSet()
    : format(ANGLE_FORMAT_NONE),
      glInternalFormat(GL_NONE),
      fboImplementationInternalFormat(GL_NONE),
      texFormat(DXGI_FORMAT_UNKNOWN),
      srvFormat(DXGI_FORMAT_UNKNOWN),
      rtvFormat(DXGI_FORMAT_UNKNOWN),
      dsvFormat(DXGI_FORMAT_UNKNOWN),
      blitSRVFormat(DXGI_FORMAT_UNKNOWN),
      swizzleFormat(ANGLE_FORMAT_NONE),
      mipGenerationFunction(nullptr),
      colorReadFunction(nullptr)
{
}

ANGLEFormatSet::ANGLEFormatSet(ANGLEFormat format,
                               GLenum glInternalFormat,
                               GLenum fboImplementationInternalFormat,
                               DXGI_FORMAT texFormat,
                               DXGI_FORMAT srvFormat,
                               DXGI_FORMAT rtvFormat,
                               DXGI_FORMAT dsvFormat,
                               DXGI_FORMAT blitSRVFormat,
                               ANGLEFormat swizzleFormat,
                               MipGenerationFunction mipGenerationFunction,
                               ColorReadFunction colorReadFunction)
    : format(format),
      glInternalFormat(glInternalFormat),
      fboImplementationInternalFormat(fboImplementationInternalFormat),
      texFormat(texFormat),
      srvFormat(srvFormat),
      rtvFormat(rtvFormat),
      dsvFormat(dsvFormat),
      blitSRVFormat(blitSRVFormat),
      swizzleFormat(swizzleFormat),
      mipGenerationFunction(mipGenerationFunction),
      colorReadFunction(colorReadFunction)
{
}

// For sized GL internal formats, there are several possible corresponding D3D11 formats depending
// on device capabilities.
// This function allows querying for the DXGI texture formats to use for textures, SRVs, RTVs and
// DSVs given a GL internal format.
TextureFormat::TextureFormat(GLenum internalFormat,
                             const ANGLEFormat angleFormat,
                             InitializeTextureDataFunction internalFormatInitializer,
                             const Renderer11DeviceCaps &deviceCaps)
    : internalFormat(internalFormat),
      formatSet(GetANGLEFormatSet(angleFormat, deviceCaps)),
      swizzleFormatSet(GetANGLEFormatSet(formatSet.swizzleFormat, deviceCaps)),
      dataInitializerFunction(internalFormatInitializer),
      loadFunctions(GetLoadFunctionsMap(internalFormat, formatSet.texFormat))
{
    ASSERT(!loadFunctions.empty() || angleFormat == ANGLE_FORMAT_NONE);
}

}  // namespace d3d11

}  // namespace rx
