//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget.h: Defines an abstract wrapper class to manage IDirect3DSurface9
// and ID3D11View objects belonging to renderbuffers.

#ifndef LIBGLESV2_RENDERER_RENDERTARGET_H_
#define LIBGLESV2_RENDERER_RENDERTARGET_H_

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/angleutils.h"

struct IDirect3DSurface9;   // D3D9_REPLACE

namespace rx
{
class RenderTarget
{
  public:
    RenderTarget() {};
    virtual ~RenderTarget() {};

    GLsizei getWidth() { return mWidth; }
    GLsizei getHeight() { return mHeight; }
    GLenum getInternalFormat() { return mInternalFormat; }
    GLenum getActualFormat() { return mActualFormat; }
    GLsizei getSamples() { return mSamples; }
    
    struct Desc {
        GLsizei width;
        GLsizei height;
        GLenum  format;
    };

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLenum mInternalFormat;
    GLenum mActualFormat;
    GLsizei mSamples;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderTarget);
};

}

#endif // LIBGLESV2_RENDERTARGET_H_
