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

namespace d3d11_gl
{

GLenum ConvertBackBufferFormat(DXGI_FORMAT format);
GLenum ConvertDepthStencilFormat(DXGI_FORMAT format);

}
