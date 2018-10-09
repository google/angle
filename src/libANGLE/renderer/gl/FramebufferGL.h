//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferGL.h: Defines the class interface for FramebufferGL.

#ifndef LIBANGLE_RENDERER_GL_FRAMEBUFFERGL_H_
#define LIBANGLE_RENDERER_GL_FRAMEBUFFERGL_H_

#include "libANGLE/renderer/FramebufferImpl.h"

namespace rx
{

class BlitGL;
class ClearMultiviewGL;
class FunctionsGL;
class StateManagerGL;
struct WorkaroundsGL;

class FramebufferGL : public FramebufferImpl
{
  public:
    FramebufferGL(const gl::FramebufferState &data, GLuint id, bool isDefault);
    ~FramebufferGL() override;

    void destroy(const gl::Context *context) override;

    gl::Error discard(const gl::Context *context, size_t count, const GLenum *attachments) override;
    gl::Error invalidate(const gl::Context *context,
                         size_t count,
                         const GLenum *attachments) override;
    gl::Error invalidateSub(const gl::Context *context,
                            size_t count,
                            const GLenum *attachments,
                            const gl::Rectangle &area) override;

    gl::Error clear(const gl::Context *context, GLbitfield mask) override;
    gl::Error clearBufferfv(const gl::Context *context,
                            GLenum buffer,
                            GLint drawbuffer,
                            const GLfloat *values) override;
    gl::Error clearBufferuiv(const gl::Context *context,
                             GLenum buffer,
                             GLint drawbuffer,
                             const GLuint *values) override;
    gl::Error clearBufferiv(const gl::Context *context,
                            GLenum buffer,
                            GLint drawbuffer,
                            const GLint *values) override;
    gl::Error clearBufferfi(const gl::Context *context,
                            GLenum buffer,
                            GLint drawbuffer,
                            GLfloat depth,
                            GLint stencil) override;

    GLenum getImplementationColorReadFormat(const gl::Context *context) const override;
    GLenum getImplementationColorReadType(const gl::Context *context) const override;

    gl::Error readPixels(const gl::Context *context,
                         const gl::Rectangle &area,
                         GLenum format,
                         GLenum type,
                         void *pixels) override;

    gl::Error blit(const gl::Context *context,
                   const gl::Rectangle &sourceArea,
                   const gl::Rectangle &destArea,
                   GLbitfield mask,
                   GLenum filter) override;

    gl::Error getSamplePosition(const gl::Context *context,
                                size_t index,
                                GLfloat *xy) const override;

    bool checkStatus(const gl::Context *context) const override;

    angle::Result syncState(const gl::Context *context,
                            const gl::Framebuffer::DirtyBits &dirtyBits) override;

    GLuint getFramebufferID() const;
    bool isDefault() const;

    void maskOutInactiveOutputDrawBuffers(const gl::Context *context,
                                          GLenum binding,
                                          gl::DrawBufferMask maxSet);

  private:
    void syncClearState(const gl::Context *context, GLbitfield mask);
    void syncClearBufferState(const gl::Context *context, GLenum buffer, GLint drawBuffer);

    bool modifyInvalidateAttachmentsForEmulatedDefaultFBO(
        size_t count,
        const GLenum *attachments,
        std::vector<GLenum> *modifiedAttachments) const;

    angle::Result readPixelsRowByRow(const gl::Context *context,
                                     const gl::Rectangle &area,
                                     GLenum format,
                                     GLenum type,
                                     const gl::PixelPackState &pack,
                                     GLubyte *pixels) const;

    angle::Result readPixelsAllAtOnce(const gl::Context *context,
                                      const gl::Rectangle &area,
                                      GLenum format,
                                      GLenum type,
                                      const gl::PixelPackState &pack,
                                      GLubyte *pixels,
                                      bool readLastRowSeparately) const;

    GLuint mFramebufferID;
    bool mIsDefault;

    gl::DrawBufferMask mAppliedEnabledDrawBuffers;
};
}

#endif  // LIBANGLE_RENDERER_GL_FRAMEBUFFERGL_H_
