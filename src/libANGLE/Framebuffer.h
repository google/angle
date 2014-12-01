//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer.h: Defines the gl::Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#ifndef LIBANGLE_FRAMEBUFFER_H_
#define LIBANGLE_FRAMEBUFFER_H_

#include "libANGLE/Error.h"
#include "libANGLE/RefCountObject.h"
#include "libANGLE/Constants.h"

#include "common/angleutils.h"

#include <vector>

namespace rx
{
class RenderbufferImpl;
struct Workarounds;
class DefaultAttachmentImpl;
class FramebufferImpl;
}

namespace gl
{
class FramebufferAttachment;
class Texture;
class Renderbuffer;
struct ImageIndex;
struct Caps;
struct Extensions;
class TextureCapsMap;
struct Data;
class State;
struct Rectangle;

typedef std::vector<FramebufferAttachment *> ColorbufferInfo;

class Framebuffer
{
  public:
    Framebuffer(rx::FramebufferImpl *impl, GLuint id);
    virtual ~Framebuffer();

    const rx::FramebufferImpl *getImplementation() const { return mImpl; }
    rx::FramebufferImpl *getImplementation() { return mImpl; }

    GLuint id() const { return mId; }

    void setTextureAttachment(GLenum attachment, Texture *texture, const ImageIndex &imageIndex);
    void setRenderbufferAttachment(GLenum attachment, Renderbuffer *renderbuffer);
    void setNULLAttachment(GLenum attachment);

    void detachTexture(GLuint texture);
    void detachRenderbuffer(GLuint renderbuffer);

    FramebufferAttachment *getColorbuffer(unsigned int colorAttachment) const;
    FramebufferAttachment *getDepthbuffer() const;
    FramebufferAttachment *getStencilbuffer() const;
    FramebufferAttachment *getDepthStencilBuffer() const;
    FramebufferAttachment *getDepthOrStencilbuffer() const;
    FramebufferAttachment *getReadColorbuffer() const;
    GLenum getReadColorbufferType() const;
    FramebufferAttachment *getFirstColorbuffer() const;

    FramebufferAttachment *getAttachment(GLenum attachment) const;

    GLenum getDrawBufferState(unsigned int colorAttachment) const;
    void setDrawBuffers(size_t count, const GLenum *buffers);

    GLenum getReadBufferState() const;
    void setReadBuffer(GLenum buffer);

    bool isEnabledColorAttachment(unsigned int colorAttachment) const;
    bool hasEnabledColorAttachment() const;
    bool hasStencil() const;
    int getSamples(const gl::Data &data) const;
    bool usingExtendedDrawBuffers() const;

    GLenum checkStatus(const gl::Data &data) const;
    bool hasValidDepthStencil() const;

    Error invalidate(size_t count, const GLenum *attachments);
    Error invalidateSub(size_t count, const GLenum *attachments, const gl::Rectangle &area);

    Error clear(const State &state, GLbitfield mask);
    Error clearBufferfv(const State &state, GLenum buffer, GLint drawbuffer, const GLfloat *values);
    Error clearBufferuiv(const State &state, GLenum buffer, GLint drawbuffer, const GLuint *values);
    Error clearBufferiv(const State &state, GLenum buffer, GLint drawbuffer, const GLint *values);
    Error clearBufferfi(const State &state, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);

    GLenum getImplementationColorReadFormat() const;
    GLenum getImplementationColorReadType() const;
    Error readPixels(const gl::State &state, const gl::Rectangle &area, GLenum format, GLenum type, GLvoid *pixels) const;

    Error blit(const gl::State &state, const gl::Rectangle &sourceArea, const gl::Rectangle &destArea,
               GLbitfield mask, GLenum filter, const gl::Framebuffer *sourceFramebuffer);

    // Use this method to retrieve the color buffer map when doing rendering.
    // It will apply a workaround for poor shader performance on some systems
    // by compacting the list to skip NULL values.
    ColorbufferInfo getColorbuffersForRender(const rx::Workarounds &workarounds) const;

  protected:
    void setAttachment(GLenum attachment, FramebufferAttachment *attachmentObj);

    rx::FramebufferImpl *mImpl;
    GLuint mId;

    FramebufferAttachment *mColorbuffers[IMPLEMENTATION_MAX_DRAW_BUFFERS];
    GLenum mDrawBufferStates[IMPLEMENTATION_MAX_DRAW_BUFFERS];
    GLenum mReadBufferState;

    FramebufferAttachment *mDepthbuffer;
    FramebufferAttachment *mStencilbuffer;

  private:
    DISALLOW_COPY_AND_ASSIGN(Framebuffer);
};

class DefaultFramebuffer : public Framebuffer
{
  public:
    DefaultFramebuffer(rx::FramebufferImpl *impl, rx::DefaultAttachmentImpl *colorAttachment,
                       rx::DefaultAttachmentImpl *depthAttachment, rx::DefaultAttachmentImpl *stencilAttachment);

  private:
    DISALLOW_COPY_AND_ASSIGN(DefaultFramebuffer);
};

}

#endif   // LIBANGLE_FRAMEBUFFER_H_
