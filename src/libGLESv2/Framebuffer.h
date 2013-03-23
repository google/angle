//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer.h: Defines the gl::Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#ifndef LIBGLESV2_FRAMEBUFFER_H_
#define LIBGLESV2_FRAMEBUFFER_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "constants.h"

namespace rx
{
class Renderer;
}

namespace gl
{
class Renderbuffer;
class Colorbuffer;
class Depthbuffer;
class Stencilbuffer;
class DepthStencilbuffer;

class Framebuffer
{
  public:
    explicit Framebuffer(rx::Renderer *renderer);

    virtual ~Framebuffer();

    void setColorbuffer(GLenum type, GLuint colorbuffer);
    void setDepthbuffer(GLenum type, GLuint depthbuffer);
    void setStencilbuffer(GLenum type, GLuint stencilbuffer);

    void detachTexture(GLuint texture);
    void detachRenderbuffer(GLuint renderbuffer);

    unsigned int getRenderTargetSerial() const;
    unsigned int getDepthbufferSerial() const;
    unsigned int getStencilbufferSerial() const;

    Renderbuffer *getColorbuffer() const;
    Renderbuffer *getDepthbuffer() const;
    Renderbuffer *getStencilbuffer() const;
    Renderbuffer *getDepthOrStencilbuffer() const;

    GLenum getColorbufferType() const;
    GLenum getDepthbufferType() const;
    GLenum getStencilbufferType() const;

    GLuint getColorbufferHandle() const;
    GLuint getDepthbufferHandle() const;
    GLuint getStencilbufferHandle() const;

    bool hasStencil() const;
    int getSamples() const;

    virtual GLenum completeness() const;

  protected:
    GLenum mColorbufferType;
    BindingPointer<Renderbuffer> mColorbufferPointer;

    GLenum mDepthbufferType;
    BindingPointer<Renderbuffer> mDepthbufferPointer;

    GLenum mStencilbufferType;
    BindingPointer<Renderbuffer> mStencilbufferPointer;

    rx::Renderer *mRenderer;

  private:
    DISALLOW_COPY_AND_ASSIGN(Framebuffer);

    Renderbuffer *lookupRenderbuffer(GLenum type, GLuint handle) const;
};

class DefaultFramebuffer : public Framebuffer
{
  public:
    DefaultFramebuffer(rx::Renderer *Renderer, Colorbuffer *colorbuffer, DepthStencilbuffer *depthStencil);

    virtual GLenum completeness() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(DefaultFramebuffer);
};

}

#endif   // LIBGLESV2_FRAMEBUFFER_H_
