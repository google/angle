//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferGL.cpp: Implements the class methods for FramebufferGL.

#include "libANGLE/renderer/gl/FramebufferGL.h"

#include "common/debug.h"

namespace rx
{

FramebufferGL::FramebufferGL()
    : FramebufferImpl()
{}

FramebufferGL::~FramebufferGL()
{}

void FramebufferGL::setColorAttachment(size_t index, const gl::FramebufferAttachment *attachment)
{
    //UNIMPLEMENTED();
}

void FramebufferGL::setDepthttachment(const gl::FramebufferAttachment *attachment)
{
    //UNIMPLEMENTED();
}

void FramebufferGL::setStencilAttachment(const gl::FramebufferAttachment *attachment)
{
    //UNIMPLEMENTED();
}

void FramebufferGL::setDepthStencilAttachment(const gl::FramebufferAttachment *attachment)
{
    //UNIMPLEMENTED();
}

void FramebufferGL::setDrawBuffers(size_t count, const GLenum *buffers)
{
    //UNIMPLEMENTED();
}

void FramebufferGL::setReadBuffer(GLenum buffer)
{
    //UNIMPLEMENTED();
}

gl::Error FramebufferGL::invalidate(size_t count, const GLenum *attachments)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferGL::invalidateSub(size_t count, const GLenum *attachments, const gl::Rectangle &area)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferGL::clear(const gl::State &state, GLbitfield mask)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferGL::clearBufferfv(const gl::State &state, GLenum buffer, GLint drawbuffer, const GLfloat *values)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferGL::clearBufferuiv(const gl::State &state, GLenum buffer, GLint drawbuffer, const GLuint *values)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferGL::clearBufferiv(const gl::State &state, GLenum buffer, GLint drawbuffer, const GLint *values)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferGL::clearBufferfi(const gl::State &state, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

GLenum FramebufferGL::getImplementationColorReadFormat() const
{
    UNIMPLEMENTED();
    return GLenum();
}

GLenum FramebufferGL::getImplementationColorReadType() const
{
    UNIMPLEMENTED();
    return GLenum();
}

gl::Error FramebufferGL::readPixels(const gl::State &state, const gl::Rectangle &area, GLenum format, GLenum type, GLvoid *pixels) const
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FramebufferGL::blit(const gl::State &state, const gl::Rectangle &sourceArea, const gl::Rectangle &destArea,
                              GLbitfield mask, GLenum filter, const gl::Framebuffer *sourceFramebuffer)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

GLenum FramebufferGL::checkStatus() const
{
    UNIMPLEMENTED();
    return GLenum();
}

}
