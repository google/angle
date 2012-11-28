//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// renderer11_utils.cpp: Conversion functions and other utility routines
// specific to the D3D11 renderer.

#include "libGLESv2/renderer/renderer11_utils.h"

#include "common/debug.h"

namespace gl_d3d11
{

D3D11_BLEND ConvertBlendFunc(GLenum glBlend)
{
    D3D11_BLEND d3dBlend = D3D11_BLEND_ZERO;

    switch (glBlend)
    {
      case GL_ZERO:                     d3dBlend = D3D11_BLEND_ZERO;                break;
      case GL_ONE:                      d3dBlend = D3D11_BLEND_ONE;                 break;
      case GL_SRC_COLOR:                d3dBlend = D3D11_BLEND_SRC_COLOR;           break;
      case GL_ONE_MINUS_SRC_COLOR:      d3dBlend = D3D11_BLEND_INV_SRC_COLOR;       break;
      case GL_DST_COLOR:                d3dBlend = D3D11_BLEND_DEST_COLOR;          break;
      case GL_ONE_MINUS_DST_COLOR:      d3dBlend = D3D11_BLEND_INV_DEST_COLOR;      break;
      case GL_SRC_ALPHA:                d3dBlend = D3D11_BLEND_SRC_ALPHA;           break;
      case GL_ONE_MINUS_SRC_ALPHA:      d3dBlend = D3D11_BLEND_INV_SRC_ALPHA;       break;
      case GL_DST_ALPHA:                d3dBlend = D3D11_BLEND_DEST_ALPHA;          break;
      case GL_ONE_MINUS_DST_ALPHA:      d3dBlend = D3D11_BLEND_INV_DEST_ALPHA;      break;
      case GL_CONSTANT_COLOR:           d3dBlend = D3D11_BLEND_BLEND_FACTOR;        break;
      case GL_ONE_MINUS_CONSTANT_COLOR: d3dBlend = D3D11_BLEND_INV_BLEND_FACTOR;    break;
      case GL_CONSTANT_ALPHA:           d3dBlend = D3D11_BLEND_BLEND_FACTOR;        break;
      case GL_ONE_MINUS_CONSTANT_ALPHA: d3dBlend = D3D11_BLEND_INV_BLEND_FACTOR;    break;
      case GL_SRC_ALPHA_SATURATE:       d3dBlend = D3D11_BLEND_SRC_ALPHA_SAT;       break;
      default: UNREACHABLE();
    }

    return d3dBlend;
}

D3D11_BLEND_OP ConvertBlendOp(GLenum glBlendOp)
{
    D3D11_BLEND_OP d3dBlendOp = D3D11_BLEND_OP_ADD;

    switch (glBlendOp)
    {
      case GL_FUNC_ADD:              d3dBlendOp = D3D11_BLEND_OP_ADD;           break;
      case GL_FUNC_SUBTRACT:         d3dBlendOp = D3D11_BLEND_OP_SUBTRACT;      break;
      case GL_FUNC_REVERSE_SUBTRACT: d3dBlendOp = D3D11_BLEND_OP_REV_SUBTRACT;  break;
      default: UNREACHABLE();
    }

    return d3dBlendOp;
}

UINT8 ConvertColorMask(bool red, bool green, bool blue, bool alpha)
{
    UINT8 mask = 0;
    if (red)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_RED;
    }
    if (green)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
    }
    if (blue)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
    }
    if (alpha)
    {
        mask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
    }
    return mask;
}

}

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

GLenum ConvertRenderbufferFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
        return GL_RGBA8_OES;
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return GL_DEPTH24_STENCIL8_OES;
      default:
        UNREACHABLE();
    }

    return GL_RGBA8_OES;
}

GLenum ConvertTextureInternalFormat(DXGI_FORMAT format)
{
    switch (format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
        return GL_RGBA8_OES;
      case DXGI_FORMAT_A8_UNORM:
        return GL_ALPHA8_EXT;
      case DXGI_FORMAT_BC1_UNORM:
        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      case DXGI_FORMAT_BC2_UNORM:
        return GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE;
      case DXGI_FORMAT_BC3_UNORM:
        return GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE;
      case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return GL_RGBA32F_EXT;
      case DXGI_FORMAT_R32G32B32_FLOAT:
        return GL_RGB32F_EXT;
      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return GL_RGBA16F_EXT;
      case DXGI_FORMAT_B8G8R8A8_UNORM:
        return GL_BGRA8_EXT;
      case DXGI_FORMAT_R8_UNORM:
        return GL_R8_EXT;
      case DXGI_FORMAT_R8G8_UNORM:
        return GL_RG8_EXT;
      case DXGI_FORMAT_R16_FLOAT:
        return GL_R16F_EXT;
      case DXGI_FORMAT_R16G16_FLOAT:
        return GL_RG16F_EXT;
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return GL_DEPTH24_STENCIL8_OES;
      default:
        UNREACHABLE();
    }

    return GL_RGBA8_OES;
}

}

namespace gl_d3d11
{

DXGI_FORMAT ConvertRenderbufferFormat(GLenum format)
{
    switch (format)
    {
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8_OES:
      case GL_RGB565:
      case GL_RGB8_OES:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
      case GL_DEPTH_COMPONENT16:
      case GL_STENCIL_INDEX8:
      case GL_DEPTH24_STENCIL8_OES:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
      default:
        UNREACHABLE();
    }

    return DXGI_FORMAT_R8G8B8A8_UNORM;
}

DXGI_FORMAT ConvertTextureFormat(GLenum internalformat)
{
    switch (internalformat)
    {
      case GL_RGB565:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGB8_OES:
      case GL_RGBA8_OES:
      case GL_LUMINANCE8_EXT:
      case GL_LUMINANCE8_ALPHA8_EXT:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
      case GL_ALPHA8_EXT:
        return DXGI_FORMAT_A8_UNORM;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return DXGI_FORMAT_BC1_UNORM;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
        return DXGI_FORMAT_BC2_UNORM;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
        return DXGI_FORMAT_BC3_UNORM;
      case GL_RGBA32F_EXT:
      case GL_ALPHA32F_EXT:
      case GL_LUMINANCE_ALPHA32F_EXT:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
      case GL_RGB32F_EXT:
      case GL_LUMINANCE32F_EXT:
        return DXGI_FORMAT_R32G32B32_FLOAT;
      case GL_RGBA16F_EXT:
      case GL_ALPHA16F_EXT:
      case GL_LUMINANCE_ALPHA16F_EXT:
      case GL_RGB16F_EXT:
      case GL_LUMINANCE16F_EXT:
        return DXGI_FORMAT_R16G16B16A16_FLOAT;
      case GL_BGRA8_EXT:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
      case GL_R8_EXT:
        return DXGI_FORMAT_R8_UNORM;
      case GL_RG8_EXT:
        return DXGI_FORMAT_R8G8_UNORM;
      case GL_R16F_EXT:
        return DXGI_FORMAT_R16_FLOAT;
      case GL_RG16F_EXT:
        return DXGI_FORMAT_R16G16_FLOAT;
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH_COMPONENT32_OES:
      case GL_DEPTH24_STENCIL8_OES:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
      default:
        UNREACHABLE();
    }

    return DXGI_FORMAT_R8G8B8A8_UNORM;
}
}
