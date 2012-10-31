//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer9_utils.h: Conversion functions and other utility routines 
// specific to the D3D9 renderer

#ifndef LIBGLESV2_RENDERER_RENDERER9_UTILS_H
#define LIBGLESV2_RENDERER_RENDERER9_UTILS_H

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <d3d9.h>

#include "libGLESv2/utilities.h"

const D3DFORMAT D3DFMT_INTZ = ((D3DFORMAT)(MAKEFOURCC('I','N','T','Z')));
const D3DFORMAT D3DFMT_NULL = ((D3DFORMAT)(MAKEFOURCC('N','U','L','L')));

namespace es2dx
{

D3DCMPFUNC ConvertComparison(GLenum comparison);
D3DCOLOR ConvertColor(gl::Color color);
D3DBLEND ConvertBlendFunc(GLenum blend);
D3DBLENDOP ConvertBlendOp(GLenum blendOp);
D3DSTENCILOP ConvertStencilOp(GLenum stencilOp);
D3DTEXTUREADDRESS ConvertTextureWrap(GLenum wrap);
D3DCULL ConvertCullMode(GLenum cullFace, GLenum frontFace);
D3DCUBEMAP_FACES ConvertCubeFace(GLenum cubeFace);
DWORD ConvertColorMask(bool red, bool green, bool blue, bool alpha);
D3DTEXTUREFILTERTYPE ConvertMagFilter(GLenum magFilter, float maxAnisotropy);
void ConvertMinFilter(GLenum minFilter, D3DTEXTUREFILTERTYPE *d3dMinFilter, D3DTEXTUREFILTERTYPE *d3dMipFilter, float maxAnisotropy);
bool ConvertPrimitiveType(GLenum primitiveType, GLsizei elementCount,
                          D3DPRIMITIVETYPE *d3dPrimitiveType, int *d3dPrimitiveCount);
D3DFORMAT ConvertRenderbufferFormat(GLenum format);
D3DMULTISAMPLE_TYPE GetMultisampleTypeFromSamples(GLsizei samples);

}

namespace dx2es
{

GLuint GetAlphaSize(D3DFORMAT colorFormat);
GLuint GetStencilSize(D3DFORMAT stencilFormat);

GLsizei GetSamplesFromMultisampleType(D3DMULTISAMPLE_TYPE type);

bool IsFormatChannelEquivalent(D3DFORMAT d3dformat, GLenum format);
GLenum ConvertBackBufferFormat(D3DFORMAT format);
GLenum ConvertDepthStencilFormat(D3DFORMAT format);
GLenum GetEquivalentFormat(D3DFORMAT format);

}

namespace dx
{
bool IsCompressedFormat(D3DFORMAT format);
size_t ComputeRowSize(D3DFORMAT format, unsigned int width);
}

#endif // LIBGLESV2_RENDERER_RENDERER9_UTILS_H
