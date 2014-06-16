#include "precompiled.h"
//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.cpp: the gl::FramebufferAttachment class and its derived classes
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/renderer/RenderTarget.h"

#include "libGLESv2/Texture.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "common/utilities.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Renderbuffer.h"

namespace gl
{

FramebufferAttachmentImpl::FramebufferAttachmentImpl()
{
}

// The default case for classes inherited from FramebufferAttachmentImpl is not to
// need to do anything upon the reference count to the parent FramebufferAttachment incrementing
// or decrementing.
void FramebufferAttachmentImpl::addProxyRef(const FramebufferAttachment *proxy)
{
}

void FramebufferAttachmentImpl::releaseProxy(const FramebufferAttachment *proxy)
{
}

///// Texture2DAttachment Implementation ////////

Texture2DAttachment::Texture2DAttachment(Texture2D *texture, GLint level) : mLevel(level)
{
    mTexture2D.set(texture);
}

Texture2DAttachment::~Texture2DAttachment()
{
    mTexture2D.set(NULL);
}

// Textures need to maintain their own reference count for references via
// Renderbuffers acting as proxies. Here, we notify the texture of a reference.
void Texture2DAttachment::addProxyRef(const FramebufferAttachment *proxy)
{
    mTexture2D->addProxyRef(proxy);
}

void Texture2DAttachment::releaseProxy(const FramebufferAttachment *proxy)
{
    mTexture2D->releaseProxy(proxy);
}

rx::RenderTarget *Texture2DAttachment::getRenderTarget()
{
    return mTexture2D->getRenderTarget(mLevel);
}

rx::RenderTarget *Texture2DAttachment::getDepthStencil()
{
    return mTexture2D->getDepthSencil(mLevel);
}

rx::TextureStorage *Texture2DAttachment::getTextureStorage()
{
    return mTexture2D->getNativeTexture()->getStorageInstance();
}

GLsizei Texture2DAttachment::getWidth() const
{
    return mTexture2D->getWidth(mLevel);
}

GLsizei Texture2DAttachment::getHeight() const
{
    return mTexture2D->getHeight(mLevel);
}

GLenum Texture2DAttachment::getInternalFormat() const
{
    return mTexture2D->getInternalFormat(mLevel);
}

GLenum Texture2DAttachment::getActualFormat() const
{
    return mTexture2D->getActualFormat(mLevel);
}

GLsizei Texture2DAttachment::getSamples() const
{
    return 0;
}

unsigned int Texture2DAttachment::getSerial() const
{
    return mTexture2D->getRenderTargetSerial(mLevel);
}

GLuint Texture2DAttachment::id() const
{
    return mTexture2D->id();
}

GLenum Texture2DAttachment::type() const
{
    return GL_TEXTURE_2D;
}

GLint Texture2DAttachment::mipLevel() const
{
    return mLevel;
}

GLint Texture2DAttachment::layer() const
{
    return 0;
}

unsigned int Texture2DAttachment::getTextureSerial() const
{
    return mTexture2D->getTextureSerial();
}

///// TextureCubeMapAttachment Implementation ////////

TextureCubeMapAttachment::TextureCubeMapAttachment(TextureCubeMap *texture, GLenum faceTarget, GLint level)
    : mFaceTarget(faceTarget), mLevel(level)
{
    mTextureCubeMap.set(texture);
}

TextureCubeMapAttachment::~TextureCubeMapAttachment()
{
    mTextureCubeMap.set(NULL);
}

// Textures need to maintain their own reference count for references via
// Renderbuffers acting as proxies. Here, we notify the texture of a reference.
void TextureCubeMapAttachment::addProxyRef(const FramebufferAttachment *proxy)
{
    mTextureCubeMap->addProxyRef(proxy);
}

void TextureCubeMapAttachment::releaseProxy(const FramebufferAttachment *proxy)
{
    mTextureCubeMap->releaseProxy(proxy);
}

rx::RenderTarget *TextureCubeMapAttachment::getRenderTarget()
{
    return mTextureCubeMap->getRenderTarget(mFaceTarget, mLevel);
}

rx::RenderTarget *TextureCubeMapAttachment::getDepthStencil()
{
    return mTextureCubeMap->getDepthStencil(mFaceTarget, mLevel);
}

rx::TextureStorage *TextureCubeMapAttachment::getTextureStorage()
{
    return mTextureCubeMap->getNativeTexture()->getStorageInstance();
}

GLsizei TextureCubeMapAttachment::getWidth() const
{
    return mTextureCubeMap->getWidth(mFaceTarget, mLevel);
}

GLsizei TextureCubeMapAttachment::getHeight() const
{
    return mTextureCubeMap->getHeight(mFaceTarget, mLevel);
}

GLenum TextureCubeMapAttachment::getInternalFormat() const
{
    return mTextureCubeMap->getInternalFormat(mFaceTarget, mLevel);
}

GLenum TextureCubeMapAttachment::getActualFormat() const
{
    return mTextureCubeMap->getActualFormat(mFaceTarget, mLevel);
}

GLsizei TextureCubeMapAttachment::getSamples() const
{
    return 0;
}

unsigned int TextureCubeMapAttachment::getSerial() const
{
    return mTextureCubeMap->getRenderTargetSerial(mFaceTarget, mLevel);
}

GLuint TextureCubeMapAttachment::id() const
{
    return mTextureCubeMap->id();
}

GLenum TextureCubeMapAttachment::type() const
{
    return mFaceTarget;
}

GLint TextureCubeMapAttachment::mipLevel() const
{
    return mLevel;
}

GLint TextureCubeMapAttachment::layer() const
{
    return 0;
}

unsigned int TextureCubeMapAttachment::getTextureSerial() const
{
    return mTextureCubeMap->getTextureSerial();
}

///// Texture3DAttachment Implementation ////////

Texture3DAttachment::Texture3DAttachment(Texture3D *texture, GLint level, GLint layer)
    : mLevel(level), mLayer(layer)
{
    mTexture3D.set(texture);
}

Texture3DAttachment::~Texture3DAttachment()
{
    mTexture3D.set(NULL);
}

// Textures need to maintain their own reference count for references via
// Renderbuffers acting as proxies. Here, we notify the texture of a reference.
void Texture3DAttachment::addProxyRef(const FramebufferAttachment *proxy)
{
    mTexture3D->addProxyRef(proxy);
}

void Texture3DAttachment::releaseProxy(const FramebufferAttachment *proxy)
{
    mTexture3D->releaseProxy(proxy);
}

rx::RenderTarget *Texture3DAttachment::getRenderTarget()
{
    return mTexture3D->getRenderTarget(mLevel, mLayer);
}

rx::RenderTarget *Texture3DAttachment::getDepthStencil()
{
    return mTexture3D->getDepthStencil(mLevel, mLayer);
}

rx::TextureStorage *Texture3DAttachment::getTextureStorage()
{
    return mTexture3D->getNativeTexture()->getStorageInstance();
}

GLsizei Texture3DAttachment::getWidth() const
{
    return mTexture3D->getWidth(mLevel);
}

GLsizei Texture3DAttachment::getHeight() const
{
    return mTexture3D->getHeight(mLevel);
}

GLenum Texture3DAttachment::getInternalFormat() const
{
    return mTexture3D->getInternalFormat(mLevel);
}

GLenum Texture3DAttachment::getActualFormat() const
{
    return mTexture3D->getActualFormat(mLevel);
}

GLsizei Texture3DAttachment::getSamples() const
{
    return 0;
}

unsigned int Texture3DAttachment::getSerial() const
{
    return mTexture3D->getRenderTargetSerial(mLevel, mLayer);
}

GLuint Texture3DAttachment::id() const
{
    return mTexture3D->id();
}

GLenum Texture3DAttachment::type() const
{
    return GL_TEXTURE_3D;
}

GLint Texture3DAttachment::mipLevel() const
{
    return mLevel;
}

GLint Texture3DAttachment::layer() const
{
    return mLayer;
}

unsigned int Texture3DAttachment::getTextureSerial() const
{
    return mTexture3D->getTextureSerial();
}

////// Texture2DArrayAttachment Implementation //////

Texture2DArrayAttachment::Texture2DArrayAttachment(Texture2DArray *texture, GLint level, GLint layer)
    : mLevel(level), mLayer(layer)
{
    mTexture2DArray.set(texture);
}

Texture2DArrayAttachment::~Texture2DArrayAttachment()
{
    mTexture2DArray.set(NULL);
}

void Texture2DArrayAttachment::addProxyRef(const FramebufferAttachment *proxy)
{
    mTexture2DArray->addProxyRef(proxy);
}

void Texture2DArrayAttachment::releaseProxy(const FramebufferAttachment *proxy)
{
    mTexture2DArray->releaseProxy(proxy);
}

rx::RenderTarget *Texture2DArrayAttachment::getRenderTarget()
{
    return mTexture2DArray->getRenderTarget(mLevel, mLayer);
}

rx::RenderTarget *Texture2DArrayAttachment::getDepthStencil()
{
    return mTexture2DArray->getDepthStencil(mLevel, mLayer);
}

rx::TextureStorage *Texture2DArrayAttachment::getTextureStorage()
{
    return mTexture2DArray->getNativeTexture()->getStorageInstance();
}

GLsizei Texture2DArrayAttachment::getWidth() const
{
    return mTexture2DArray->getWidth(mLevel);
}

GLsizei Texture2DArrayAttachment::getHeight() const
{
    return mTexture2DArray->getHeight(mLevel);
}

GLenum Texture2DArrayAttachment::getInternalFormat() const
{
    return mTexture2DArray->getInternalFormat(mLevel);
}

GLenum Texture2DArrayAttachment::getActualFormat() const
{
    return mTexture2DArray->getActualFormat(mLevel);
}

GLsizei Texture2DArrayAttachment::getSamples() const
{
    return 0;
}

unsigned int Texture2DArrayAttachment::getSerial() const
{
    return mTexture2DArray->getRenderTargetSerial(mLevel, mLayer);
}

GLuint Texture2DArrayAttachment::id() const
{
    return mTexture2DArray->id();
}

GLenum Texture2DArrayAttachment::type() const
{
    return GL_TEXTURE_2D_ARRAY;
}

GLint Texture2DArrayAttachment::mipLevel() const
{
    return mLevel;
}

GLint Texture2DArrayAttachment::layer() const
{
    return mLayer;
}

unsigned int Texture2DArrayAttachment::getTextureSerial() const
{
    return mTexture2DArray->getTextureSerial();
}

////// FramebufferAttachment Implementation //////

FramebufferAttachment::FramebufferAttachment(GLuint id, FramebufferAttachmentImpl *instance)
  : RefCountObject(id),
    mImpl(instance)
{
    ASSERT(mImpl != NULL);
}

FramebufferAttachment::~FramebufferAttachment()
{
    SafeDelete(mImpl);
}

// The FramebufferAttachmentImpl contained in this FramebufferAttachment may need to maintain
// its own reference count, so we pass it on here.
void FramebufferAttachment::addRef() const
{
    mImpl->addProxyRef(this);

    RefCountObject::addRef();
}

void FramebufferAttachment::release() const
{
    mImpl->releaseProxy(this);

    RefCountObject::release();
}

rx::RenderTarget *FramebufferAttachment::getRenderTarget()
{
    return mImpl->getRenderTarget();
}

rx::RenderTarget *FramebufferAttachment::getDepthStencil()
{
    return mImpl->getDepthStencil();
}

rx::TextureStorage *FramebufferAttachment::getTextureStorage()
{
    return mImpl->getTextureStorage();
}

GLsizei FramebufferAttachment::getWidth() const
{
    return mImpl->getWidth();
}

GLsizei FramebufferAttachment::getHeight() const
{
    return mImpl->getHeight();
}

GLenum FramebufferAttachment::getInternalFormat() const
{
    return mImpl->getInternalFormat();
}

GLenum FramebufferAttachment::getActualFormat() const
{
    return mImpl->getActualFormat();
}

GLuint FramebufferAttachment::getRedSize(int clientVersion) const
{
    if (gl::GetRedBits(getInternalFormat(), clientVersion) > 0)
    {
        return gl::GetRedBits(getActualFormat(), clientVersion);
    }
    else
    {
        return 0;
    }
}

GLuint FramebufferAttachment::getGreenSize(int clientVersion) const
{
    if (gl::GetGreenBits(getInternalFormat(), clientVersion) > 0)
    {
        return gl::GetGreenBits(getActualFormat(), clientVersion);
    }
    else
    {
        return 0;
    }
}

GLuint FramebufferAttachment::getBlueSize(int clientVersion) const
{
    if (gl::GetBlueBits(getInternalFormat(), clientVersion) > 0)
    {
        return gl::GetBlueBits(getActualFormat(), clientVersion);
    }
    else
    {
        return 0;
    }
}

GLuint FramebufferAttachment::getAlphaSize(int clientVersion) const
{
    if (gl::GetAlphaBits(getInternalFormat(), clientVersion) > 0)
    {
        return gl::GetAlphaBits(getActualFormat(), clientVersion);
    }
    else
    {
        return 0;
    }
}

GLuint FramebufferAttachment::getDepthSize(int clientVersion) const
{
    if (gl::GetDepthBits(getInternalFormat(), clientVersion) > 0)
    {
        return gl::GetDepthBits(getActualFormat(), clientVersion);
    }
    else
    {
        return 0;
    }
}

GLuint FramebufferAttachment::getStencilSize(int clientVersion) const
{
    if (gl::GetStencilBits(getInternalFormat(), clientVersion) > 0)
    {
        return gl::GetStencilBits(getActualFormat(), clientVersion);
    }
    else
    {
        return 0;
    }
}

GLenum FramebufferAttachment::getComponentType(int clientVersion) const
{
    return gl::GetComponentType(getActualFormat(), clientVersion);
}

GLenum FramebufferAttachment::getColorEncoding(int clientVersion) const
{
    return gl::GetColorEncoding(getActualFormat(), clientVersion);
}

bool FramebufferAttachment::isTexture() const
{
    return (type() != GL_RENDERBUFFER);
}

GLsizei FramebufferAttachment::getSamples() const
{
    return mImpl->getSamples();
}

unsigned int FramebufferAttachment::getSerial() const
{
    return mImpl->getSerial();
}

GLuint FramebufferAttachment::id() const
{
    return mImpl->id();
}

GLuint FramebufferAttachment::type() const
{
    return mImpl->type();
}

GLint FramebufferAttachment::mipLevel() const
{
    return mImpl->mipLevel();
}

GLint FramebufferAttachment::layer() const
{
    return mImpl->layer();
}

unsigned int FramebufferAttachment::getTextureSerial() const
{
    return mImpl->getTextureSerial();
}

void FramebufferAttachment::setImplementation(FramebufferAttachmentImpl *newImpl)
{
    ASSERT(newImpl != NULL);

    delete mImpl;
    mImpl = newImpl;
}

RenderbufferAttachment::RenderbufferAttachment(Renderbuffer *renderbuffer)
{
    ASSERT(renderbuffer);
    mRenderbuffer.set(renderbuffer);
}

RenderbufferAttachment::~RenderbufferAttachment()
{
    mRenderbuffer.set(NULL);
}

rx::RenderTarget *RenderbufferAttachment::getRenderTarget()
{
    return mRenderbuffer->getStorage()->getRenderTarget();
}

rx::RenderTarget *RenderbufferAttachment::getDepthStencil()
{
    return mRenderbuffer->getStorage()->getDepthStencil();
}

rx::TextureStorage *RenderbufferAttachment::getTextureStorage()
{
    UNREACHABLE();
    return NULL;
}

GLsizei RenderbufferAttachment::getWidth() const
{
    return mRenderbuffer->getWidth();
}

GLsizei RenderbufferAttachment::getHeight() const
{
    return mRenderbuffer->getHeight();
}

GLenum RenderbufferAttachment::getInternalFormat() const
{
    return mRenderbuffer->getInternalFormat();
}

GLenum RenderbufferAttachment::getActualFormat() const
{
    return mRenderbuffer->getActualFormat();
}

GLsizei RenderbufferAttachment::getSamples() const
{
    return mRenderbuffer->getStorage()->getSamples();
}

unsigned int RenderbufferAttachment::getSerial() const
{
    return mRenderbuffer->getStorage()->getSerial();
}

GLuint RenderbufferAttachment::id() const
{
    return mRenderbuffer->id();
}

GLenum RenderbufferAttachment::type() const
{
    return GL_RENDERBUFFER;
}

GLint RenderbufferAttachment::mipLevel() const
{
    return 0;
}

GLint RenderbufferAttachment::layer() const
{
    return 0;
}

unsigned int RenderbufferAttachment::getTextureSerial() const
{
    UNREACHABLE();
    return 0;
}

}
