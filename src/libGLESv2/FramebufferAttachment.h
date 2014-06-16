//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.h: Defines the wrapper class gl::FramebufferAttachment, as well as the
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#ifndef LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
#define LIBGLESV2_FRAMEBUFFERATTACHMENT_H_

#include <GLES3/gl3.h>
#include <GLES2/gl2.h>

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class Renderer;
class RenderTarget;
class TextureStorage;
}

namespace gl
{
class Texture2D;
class TextureCubeMap;
class Texture3D;
class Texture2DArray;
class FramebufferAttachmentImpl;
class Renderbuffer;

// FramebufferAttachment implements a GL framebuffer attachment.
// Attachments are "light" containers, which store pointers to ref-counted GL objects.
// We support GL texture (2D/3D/Cube/2D array) and renderbuffer object attachments.
// Note: Renderbuffers are specialized storage for depth and stencil buffes. Our old
// naming scheme used the term "Renderbuffer" for both GL renderbuffers and for
// framebuffer attachments, which confused their usage.

class FramebufferAttachment : public RefCountObject
{
  public:
    FramebufferAttachment(rx::Renderer *renderer, GLuint id, FramebufferAttachmentImpl *storage);

    virtual ~FramebufferAttachment();

    // These functions from RefCountObject are overloaded here because
    // Textures need to maintain their own count of references to them via
    // Renderbuffers/RenderbufferTextures. These functions invoke those
    // reference counting functions on the FramebufferAttachmentImpl.
    void addRef() const;
    void release() const;

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();
    rx::TextureStorage *getTextureStorage();

    GLsizei getWidth() const;
    GLsizei getHeight() const;
    GLenum getInternalFormat() const;
    GLenum getActualFormat() const;
    GLuint getRedSize() const;
    GLuint getGreenSize() const;
    GLuint getBlueSize() const;
    GLuint getAlphaSize() const;
    GLuint getDepthSize() const;
    GLuint getStencilSize() const;
    GLenum getComponentType() const;
    GLenum getColorEncoding() const;
    GLsizei getSamples() const;

    unsigned int getSerial() const;

    bool isTexture() const;
    unsigned int getTextureSerial() const;

    void setImplementation(FramebufferAttachmentImpl *newImpl);

  private:
    DISALLOW_COPY_AND_ASSIGN(FramebufferAttachment);

    rx::Renderer const *mRenderer;
    FramebufferAttachmentImpl *mImpl;
};

class FramebufferAttachmentImpl
{
  public:
    FramebufferAttachmentImpl();

    virtual ~FramebufferAttachmentImpl() {};

    virtual void addProxyRef(const FramebufferAttachment *proxy);
    virtual void releaseProxy(const FramebufferAttachment *proxy);

    virtual rx::RenderTarget *getRenderTarget() = 0;
    virtual rx::RenderTarget *getDepthStencil() = 0;
    virtual rx::TextureStorage *getTextureStorage() = 0;

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

    virtual unsigned int getSerial() const = 0;

    virtual bool isTexture() const = 0;
    virtual unsigned int getTextureSerial() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(FramebufferAttachmentImpl);
};

class Texture2DAttachment : public FramebufferAttachmentImpl
{
  public:
    Texture2DAttachment(Texture2D *texture, GLint level);

    virtual ~Texture2DAttachment();

    void addProxyRef(const FramebufferAttachment *proxy);
    void releaseProxy(const FramebufferAttachment *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual bool isTexture() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DAttachment);

    BindingPointer <Texture2D> mTexture2D;
    const GLint mLevel;
};

class TextureCubeMapAttachment : public FramebufferAttachmentImpl
{
  public:
    TextureCubeMapAttachment(TextureCubeMap *texture, GLenum faceTarget, GLint level);

    virtual ~TextureCubeMapAttachment();

    void addProxyRef(const FramebufferAttachment *proxy);
    void releaseProxy(const FramebufferAttachment *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual bool isTexture() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMapAttachment);

    BindingPointer <TextureCubeMap> mTextureCubeMap;
    const GLint mLevel;
    const GLenum mFaceTarget;
};

class Texture3DAttachment : public FramebufferAttachmentImpl
{
  public:
    Texture3DAttachment(Texture3D *texture, GLint level, GLint layer);

    virtual ~Texture3DAttachment();

    void addProxyRef(const FramebufferAttachment *proxy);
    void releaseProxy(const FramebufferAttachment *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual bool isTexture() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture3DAttachment);

    BindingPointer<Texture3D> mTexture3D;
    const GLint mLevel;
    const GLint mLayer;
};

class Texture2DArrayAttachment : public FramebufferAttachmentImpl
{
  public:
    Texture2DArrayAttachment(Texture2DArray *texture, GLint level, GLint layer);

    virtual ~Texture2DArrayAttachment();

    void addProxyRef(const FramebufferAttachment *proxy);
    void releaseProxy(const FramebufferAttachment *proxy);

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual bool isTexture() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DArrayAttachment);

    BindingPointer<Texture2DArray> mTexture2DArray;
    const GLint mLevel;
    const GLint mLayer;
};

class RenderbufferAttachment : public FramebufferAttachmentImpl
{
  public:
    RenderbufferAttachment(Renderbuffer *renderbuffer);

    virtual ~RenderbufferAttachment();

    rx::RenderTarget *getRenderTarget();
    rx::RenderTarget *getDepthStencil();
    rx::TextureStorage *getTextureStorage();

    virtual GLsizei getWidth() const;
    virtual GLsizei getHeight() const;
    virtual GLenum getInternalFormat() const;
    virtual GLenum getActualFormat() const;
    virtual GLsizei getSamples() const;

    virtual unsigned int getSerial() const;

    virtual bool isTexture() const;
    virtual unsigned int getTextureSerial() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(RenderbufferAttachment);

    BindingPointer<Renderbuffer> mRenderbuffer;
};

}

#endif // LIBGLESV2_FRAMEBUFFERATTACHMENT_H_
