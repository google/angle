//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferMtl.mm:
//    Implements the class methods for FramebufferMtl.
//

#include "libANGLE/renderer/metal/ContextMtl.h"

#include <TargetConditionals.h>

#include "common/angleutils.h"
#include "common/debug.h"
#include "libANGLE/renderer/metal/FrameBufferMtl.h"

namespace rx
{

// FramebufferMtl implementation
FramebufferMtl::FramebufferMtl(const gl::FramebufferState &state) : FramebufferImpl(state) {}

FramebufferMtl::~FramebufferMtl() {}

void FramebufferMtl::destroy(const gl::Context *context)
{
    UNIMPLEMENTED();
}

angle::Result FramebufferMtl::discard(const gl::Context *context,
                                      size_t count,
                                      const GLenum *attachments)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result FramebufferMtl::invalidate(const gl::Context *context,
                                         size_t count,
                                         const GLenum *attachments)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result FramebufferMtl::invalidateSub(const gl::Context *context,
                                            size_t count,
                                            const GLenum *attachments,
                                            const gl::Rectangle &area)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result FramebufferMtl::clear(const gl::Context *context, GLbitfield mask)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result FramebufferMtl::clearBufferfv(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            const GLfloat *values)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result FramebufferMtl::clearBufferuiv(const gl::Context *context,
                                             GLenum buffer,
                                             GLint drawbuffer,
                                             const GLuint *values)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result FramebufferMtl::clearBufferiv(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            const GLint *values)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result FramebufferMtl::clearBufferfi(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            GLfloat depth,
                                            GLint stencil)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

GLenum FramebufferMtl::getImplementationColorReadFormat(const gl::Context *context) const
{
    UNIMPLEMENTED();
    return GL_INVALID_ENUM;
}

GLenum FramebufferMtl::getImplementationColorReadType(const gl::Context *context) const
{
    UNIMPLEMENTED();
    return GL_INVALID_ENUM;
}

angle::Result FramebufferMtl::readPixels(const gl::Context *context,
                                         const gl::Rectangle &area,
                                         GLenum format,
                                         GLenum type,
                                         void *pixels)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result FramebufferMtl::blit(const gl::Context *context,
                                   const gl::Rectangle &sourceArea,
                                   const gl::Rectangle &destArea,
                                   GLbitfield mask,
                                   GLenum filter)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

bool FramebufferMtl::checkStatus(const gl::Context *context) const
{
    UNIMPLEMENTED();
    return false;
}

angle::Result FramebufferMtl::syncState(const gl::Context *context,
                                        const gl::Framebuffer::DirtyBits &dirtyBits)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result FramebufferMtl::getSamplePosition(const gl::Context *context,
                                                size_t index,
                                                GLfloat *xy) const
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
}
