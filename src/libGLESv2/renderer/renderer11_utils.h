//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer11_utils.h: Conversion functions and other utility routines
// specific to the D3D11 renderer.

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <d3d11.h>

#include "libGLESv2/angletypes.h"

namespace gl_d3d11
{

D3D11_BLEND ConvertBlendFunc(GLenum glBlend);
D3D11_BLEND_OP ConvertBlendOp(GLenum glBlendOp);
UINT8 ConvertColorMask(bool maskRed, bool maskGreen, bool maskBlue, bool maskAlpha);

D3D11_CULL_MODE ConvertCullMode(bool cullEnabled, GLenum cullMode);

}

namespace d3d11_gl
{

GLenum ConvertBackBufferFormat(DXGI_FORMAT format);
GLenum ConvertDepthStencilFormat(DXGI_FORMAT format);
GLenum ConvertRenderbufferFormat(DXGI_FORMAT format);
GLenum ConvertTextureInternalFormat(DXGI_FORMAT format);
}

namespace gl_d3d11
{
DXGI_FORMAT ConvertRenderbufferFormat(GLenum format);
DXGI_FORMAT ConvertTextureInternalFormat(GLenum internalformat);
}
