//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.h: Defines the wrapper class gl::FramebufferAttachment, as well as the
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBANGLE_FRAMEBUFFERATTACHMENT_H_
#define LIBANGLE_FRAMEBUFFERATTACHMENT_H_

#include "angle_gl.h"
#include "common/angleutils.h"
#include "libANGLE/Texture.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/Surface.h"

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
    FramebufferAttachment(GLenum binding,
                          const ImageIndex &textureIndex,
                          RefCountObject *resource);
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

    GLuint id() const;

    // These methods are only legal to call on Texture attachments
    const ImageIndex *getTextureImageIndex() const;
    GLenum cubeMapFace() const;
    GLint mipLevel() const;
    GLint layer() const;

    // Child class interface
    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    virtual GLenum type() const = 0;

    virtual Texture *getTexture() const = 0;
    virtual Renderbuffer *getRenderbuffer() const = 0;

  protected:
    GLenum mBinding;
    ImageIndex mTextureIndex;
    BindingPointer<RefCountObject> mResource;
};

class TextureAttachment : public FramebufferAttachment
{
  public:
    TextureAttachment(GLenum binding, Texture *texture, const ImageIndex &index);
    virtual ~TextureAttachment();

    virtual GLsizei getSamples() const;

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;

    virtual GLenum type() const;

    virtual Renderbuffer *getRenderbuffer() const;

    Texture *getTexture() const override
    {
        return rx::GetAs<Texture>(mResource.get());
    }
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

    virtual GLenum type() const;

    virtual Texture *getTexture() const;

    Renderbuffer *getRenderbuffer() const override
    {
        return rx::GetAs<Renderbuffer>(mResource.get());
    }
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

    virtual GLenum type() const;

    virtual Texture *getTexture() const;
    virtual Renderbuffer *getRenderbuffer() const;

    const egl::Surface *getSurface() const
    {
        return rx::GetAs<egl::Surface>(mResource.get());
    }
};

}

#endif // LIBANGLE_FRAMEBUFFERATTACHMENT_H_
