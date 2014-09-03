//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.h: Defines the wrapper class gl::FramebufferAttachment, as well as the
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
#define LIBGLESV2_FRAMEBUFFERATTACHMENT_H_

#include "common/angleutils.h"
#include "common/RefCountObject.h"
#include "Texture.h"

#include "angle_gl.h"

namespace rx
{
class Renderer;
class RenderTarget;
class TextureStorage;
}

namespace gl
{
class Renderbuffer;

// FramebufferAttachment implements a GL framebuffer attachment.
// Attachments are "light" containers, which store pointers to ref-counted GL objects.
// We support GL texture (2D/3D/Cube/2D array) and renderbuffer object attachments.
// Note: Our old naming scheme used the term "Renderbuffer" for both GL renderbuffers and for
// framebuffer attachments, which confused their usage.

class FramebufferAttachment
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
    bool isTexture() const;

    bool isTextureWithId(GLuint textureId) const { return isTexture() && id() == textureId; }
    bool isRenderbufferWithId(GLuint renderbufferId) const { return !isTexture() && id() == renderbufferId; }

    GLenum getBinding() const { return mBinding; }

    // Child class interface
    virtual rx::RenderTarget *getRenderTarget() = 0;
    virtual rx::TextureStorage *getTextureStorage() = 0;

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    virtual unsigned int getSerial() const = 0;

    virtual GLuint id() const = 0;
    virtual GLenum type() const = 0;
    virtual GLint mipLevel() const = 0;
    virtual GLint layer() const = 0;
    virtual unsigned int getTextureSerial() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(FramebufferAttachment);

    GLenum mBinding;
};

class TextureAttachment : public FramebufferAttachment
{
  public:
    TextureAttachment(GLenum binding, const ImageIndex &index);

    rx::TextureStorage *getTextureStorage();
    virtual GLsizei getSamples() const;
    virtual GLuint id() const;
    virtual unsigned int getTextureSerial() const;

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;

    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;

    virtual rx::RenderTarget *getRenderTarget();
    virtual unsigned int getSerial() const;

  protected:
    virtual Texture *getTexture() const = 0;
    ImageIndex mIndex;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureAttachment);
};

class Texture2DAttachment : public TextureAttachment
{
  public:
    Texture2DAttachment(GLenum binding, Texture2D *texture, GLint level);

    virtual ~Texture2DAttachment();
    virtual Texture *getTexture() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DAttachment);

    BindingPointer<Texture2D> mTexture2D;
    const GLint mLevel;
};

class TextureCubeMapAttachment : public TextureAttachment
{
  public:
    TextureCubeMapAttachment(GLenum binding, TextureCubeMap *texture, GLenum faceTarget, GLint level);

    virtual ~TextureCubeMapAttachment();
    virtual Texture *getTexture() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMapAttachment);

    BindingPointer<TextureCubeMap> mTextureCubeMap;
    const GLint mLevel;
    const GLenum mFaceTarget;
};

class Texture3DAttachment : public TextureAttachment
{
  public:
    Texture3DAttachment(GLenum binding, Texture3D *texture, GLint level, GLint layer);

    virtual ~Texture3DAttachment();
    virtual Texture *getTexture() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture3DAttachment);

    BindingPointer<Texture3D> mTexture3D;
    const GLint mLevel;
    const GLint mLayer;
};

class Texture2DArrayAttachment : public TextureAttachment
{
  public:
    Texture2DArrayAttachment(GLenum binding, Texture2DArray *texture, GLint level, GLint layer);

    virtual ~Texture2DArrayAttachment();
    virtual Texture *getTexture() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DArrayAttachment);

    BindingPointer<Texture2DArray> mTexture2DArray;
    const GLint mLevel;
    const GLint mLayer;
};

class RenderbufferAttachment : public FramebufferAttachment
{
  public:
    RenderbufferAttachment(GLenum binding, Renderbuffer *renderbuffer);

    virtual ~RenderbufferAttachment();

    rx::RenderTarget *getRenderTarget();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual GLuint id() const;
    virtual GLenum type() const;
    virtual GLint mipLevel() const;
    virtual GLint layer() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferAttachment);

    BindingPointer<Renderbuffer> mRenderbuffer;
};

}

#endif // LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
