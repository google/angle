//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderbuffer.h: Defines the wrapper class gl::Renderbuffer, as well as the
// class hierarchy used to store its contents: RenderbufferStorage, Colorbuffer,
// DepthStencilbuffer, Depthbuffer and Stencilbuffer. Implements GL renderbuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBGLESV2_RENDERBUFFER_H_
#define LIBGLESV2_RENDERBUFFER_H_

#include <GLES3/gl3.h>
#include <GLES2/gl2.h>

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "libGLESv2/FramebufferAttachment.h"

namespace rx
{
class Renderer;
class SwapChain;
class RenderTarget;
class TextureStorage;
}

namespace gl
{

// A class derived from RenderbufferStorage is created whenever glRenderbufferStorage
// is called. The specific concrete type depends on whether the internal format is
// colour depth, stencil or packed depth/stencil.
class RenderbufferStorage : public FramebufferAttachmentInterface
{
  public:
    RenderbufferStorage();

    virtual ~RenderbufferStorage() = 0;

    virtual rx::RenderTarget *getRenderTarget();
    virtual rx::RenderTarget *getDepthStencil();
    virtual rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual bool isTexture() const;
    virtual unsigned int getTextureSerial() const;

    static unsigned int issueSerials(GLuint count);

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLenum mInternalFormat;
    GLenum mActualFormat;
    GLsizei mSamples;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferStorage);

    const unsigned int mSerial;

    static unsigned int mCurrentSerial;
};

class Colorbuffer : public RenderbufferStorage
{
  public:
    Colorbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain);
    Colorbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLenum format, GLsizei samples);

    virtual ~Colorbuffer();

    virtual rx::RenderTarget *getRenderTarget();

  private:
    DISALLOW_COPY_AND_ASSIGN(Colorbuffer);

    rx::RenderTarget *mRenderTarget;
};

class DepthStencilbuffer : public RenderbufferStorage
{
  public:
    DepthStencilbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain);
    DepthStencilbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    ~DepthStencilbuffer();

    virtual rx::RenderTarget *getDepthStencil();

  protected:
    rx::RenderTarget  *mDepthStencil;

  private:
    DISALLOW_COPY_AND_ASSIGN(DepthStencilbuffer);
};

class Depthbuffer : public DepthStencilbuffer
{
  public:
    Depthbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Depthbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(Depthbuffer);
};

class Stencilbuffer : public DepthStencilbuffer
{
  public:
    Stencilbuffer(rx::Renderer *renderer, GLsizei width, GLsizei height, GLsizei samples);

    virtual ~Stencilbuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(Stencilbuffer);
};
}

#endif   // LIBGLESV2_RENDERBUFFER_H_
