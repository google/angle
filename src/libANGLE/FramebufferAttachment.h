//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.h: Defines the wrapper class gl::FramebufferAttachment, as well as the
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBANGLE_FRAMEBUFFERATTACHMENT_H_
#define LIBANGLE_FRAMEBUFFERATTACHMENT_H_

#include "libANGLE/Texture.h"
#include "libANGLE/RefCountObject.h"

#include "common/angleutils.h"

#include "angle_gl.h"

namespace gl
{
class Renderbuffer;

// FramebufferAttachment implements a GL framebuffer attachment.
// Attachments are "light" containers, which store pointers to ref-counted GL objects.
// We support GL texture (2D/3D/Cube/2D array) and renderbuffer object attachments.
// Note: Our old naming scheme used the term "Renderbuffer" for both GL renderbuffers and for
// framebuffer attachments, which confused their usage.

class FramebufferAttachment : angle::NonCopyable
{
  public:
    explicit FramebufferAttachment(GLenum binding);
    virtual ~FramebufferAttachment();

    // Helper methods
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    GLenum getComponentType() const;
    GLenum getColorEncoding() const;

    bool isTextureWithId(GLuint textureId) const { return type() == GL_TEXTURE && id() == textureId; }
    bool isRenderbufferWithId(GLuint renderbufferId) const { return type() == GL_RENDERBUFFER && id() == renderbufferId; }

    GLenum getBinding() const { return mBinding; }

    // Child class interface
    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    virtual GLuint id() const = 0;
    virtual GLenum type() const = 0;
    virtual GLint mipLevel() const = 0;
    virtual GLenum cubeMapFace() const = 0;
    virtual GLint layer() const = 0;

    virtual Texture *getTexture() const = 0;
    virtual const ImageIndex *getTextureImageIndex() const = 0;
    virtual Renderbuffer *getRenderbuffer() const = 0;

  private:
    GLenum mBinding;
};

class TextureAttachment : public FramebufferAttachment
{
  public:
    TextureAttachment(GLenum binding, Texture *texture, const ImageIndex &index);
    virtual ~TextureAttachment();

    virtual GLsizei getSamples() const;
    virtual GLuint id() const;

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;

    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLenum cubeMapFace() const;
    virtual GLint layer() const;

    virtual Texture *getTexture() const;
    virtual const ImageIndex *getTextureImageIndex() const;
    virtual Renderbuffer *getRenderbuffer() const;

  private:
    BindingPointer<Texture> mTexture;
    ImageIndex mIndex;
};

class RenderbufferAttachment : public FramebufferAttachment
{
  public:
    RenderbufferAttachment(GLenum binding, Renderbuffer *renderbuffer);

    virtual ~RenderbufferAttachment();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLsizei getSamples() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLenum cubeMapFace() const;
    virtual GLint layer() const;

    virtual Texture *getTexture() const;
    virtual const ImageIndex *getTextureImageIndex() const;
    virtual Renderbuffer *getRenderbuffer() const;

  private:
    BindingPointer<Renderbuffer> mRenderbuffer;
};

class DefaultAttachment : public FramebufferAttachment
{
  public:
    DefaultAttachment(GLenum binding, egl::Surface *surface);

    virtual ~DefaultAttachment();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLsizei getSamples() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLenum cubeMapFace() const;
    virtual GLint layer() const;

    virtual Texture *getTexture() const;
    virtual const ImageIndex *getTextureImageIndex() const;
    virtual Renderbuffer *getRenderbuffer() const;

    const egl::Surface *getSurface() const { return mSurface.get(); }

  private:
    BindingPointer<egl::Surface> mSurface;
};

}

#endif // LIBANGLE_FRAMEBUFFERATTACHMENT_H_
