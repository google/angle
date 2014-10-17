//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferD3d.h: Defines the RenderbufferD3D class which implements RenderbufferImpl.

#ifndef LIBGLESV2_RENDERER_RENDERBUFFERD3D_H_
#define LIBGLESV2_RENDERER_RENDERBUFFERD3D_H_

#include "angle_gl.h"

#include "common/angleutils.h"
#include "libGLESv2/renderer/RenderbufferImpl.h"

namespace rx
{
class Renderer;
class RenderTarget;
class SwapChain;

class RenderbufferD3D : public RenderbufferImpl
{
  public:
    RenderbufferD3D(Renderer *renderer);
    virtual ~RenderbufferD3D();

    static RenderbufferD3D *makeRenderbufferD3D(RenderbufferImpl *texture);

    virtual void setStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples);
    virtual void setStorage(SwapChain *swapChain, bool depth);

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual rx::RenderTarget *getRenderTarget();
    virtual unsigned int getSerial() const;
    static unsigned int issueSerials(unsigned int count);

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferD3D);

    Renderer *mRenderer;
    RenderTarget *mRenderTarget;
    const unsigned int mSerial;

    static unsigned int mCurrentSerial;
};
}

#endif // LIBGLESV2_RENDERER_RENDERBUFFERD3D_H_
