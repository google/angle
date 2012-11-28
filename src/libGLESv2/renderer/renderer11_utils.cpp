//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer11_utils.cpp: Conversion functions and other utility routines
// specific to the D3D11 renderer.

#include "libGLESv2/renderer/renderer11_utils.h"

#include "common/debug.h"

namespace d3d11_gl
{

GLenum ConvertBackBufferFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM: return GL_RGBA8_OES;
      default:
        UNREACHABLE();
    }

    return GL_RGBA8_OES;
}

GLenum ConvertDepthStencilFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_D24_UNORM_S8_UINT: return GL_DEPTH24_STENCIL8_OES;
      default:
        UNREACHABLE();
    }

    return GL_DEPTH24_STENCIL8_OES;
}

}
