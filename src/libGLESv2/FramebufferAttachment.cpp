//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.cpp: the gl::FramebufferAttachment class and its derived classes
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#include "libGLESv2/FramebufferAttachment.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/RenderTarget.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/d3d/TextureStorage.h"

#include "common/utilities.h"

namespace gl
{

////// FramebufferAttachment Implementation //////

FramebufferAttachment::FramebufferAttachment(GLenum binding)
    : mBinding(binding)
{
}

FramebufferAttachment::~FramebufferAttachment()
{
}

GLuint FramebufferAttachment::getRedSize() const
{
    return (GetInternalFormatInfo(getInternalFormat()).redBits > 0) ? GetInternalFormatInfo(getActualFormat()).redBits : 0;
}

GLuint FramebufferAttachment::getGreenSize() const
{
    return (GetInternalFormatInfo(getInternalFormat()).greenBits > 0) ? GetInternalFormatInfo(getActualFormat()).greenBits : 0;
}

GLuint FramebufferAttachment::getBlueSize() const
{
    return (GetInternalFormatInfo(getInternalFormat()).blueBits > 0) ? GetInternalFormatInfo(getActualFormat()).blueBits : 0;
}

GLuint FramebufferAttachment::getAlphaSize() const
{
    return (GetInternalFormatInfo(getInternalFormat()).alphaBits > 0) ? GetInternalFormatInfo(getActualFormat()).alphaBits : 0;
}

GLuint FramebufferAttachment::getDepthSize() const
{
    return (GetInternalFormatInfo(getInternalFormat()).depthBits > 0) ? GetInternalFormatInfo(getActualFormat()).depthBits : 0;
}

GLuint FramebufferAttachment::getStencilSize() const
{
    return (GetInternalFormatInfo(getInternalFormat()).stencilBits > 0) ? GetInternalFormatInfo(getActualFormat()).stencilBits : 0;
}

GLenum FramebufferAttachment::getComponentType() const
{
    return GetInternalFormatInfo(getActualFormat()).componentType;
}

GLenum FramebufferAttachment::getColorEncoding() const
{
    return GetInternalFormatInfo(getActualFormat()).colorEncoding;
}

bool FramebufferAttachment::isTexture() const
{
    return (type() != GL_RENDERBUFFER);
}

///// TextureAttachment Implementation ////////

TextureAttachment::TextureAttachment(GLenum binding)
    : FramebufferAttachment(binding)
{}

rx::TextureStorage *TextureAttachment::getTextureStorage()
{
    return getTexture()->getNativeTexture()->getStorageInstance();
}

GLsizei TextureAttachment::getSamples() const
{
    return 0;
}

GLuint TextureAttachment::id() const
{
    return getTexture()->id();
}

unsigned int TextureAttachment::getTextureSerial() const
{
    return getTexture()->getTextureSerial();
}

///// Texture2DAttachment Implementation ////////

Texture2DAttachment::Texture2DAttachment(GLenum binding, Texture2D *texture, GLint level)
    : TextureAttachment(binding),
      mLevel(level)
{
    mTexture2D.set(texture);
}

Texture2DAttachment::~Texture2DAttachment()
{
    mTexture2D.set(NULL);
}

rx::RenderTarget *Texture2DAttachment::getRenderTarget()
{
    return mTexture2D->getRenderTarget(mLevel);
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

unsigned int Texture2DAttachment::getSerial() const
{
    return mTexture2D->getRenderTargetSerial(mLevel);
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

Texture *Texture2DAttachment::getTexture() const
{
    return mTexture2D.get();
}

///// TextureCubeMapAttachment Implementation ////////

TextureCubeMapAttachment::TextureCubeMapAttachment(GLenum binding, TextureCubeMap *texture, GLenum faceTarget, GLint level)
    : TextureAttachment(binding),
      mFaceTarget(faceTarget), mLevel(level)
{
    mTextureCubeMap.set(texture);
}

TextureCubeMapAttachment::~TextureCubeMapAttachment()
{
    mTextureCubeMap.set(NULL);
}

rx::RenderTarget *TextureCubeMapAttachment::getRenderTarget()
{
    return mTextureCubeMap->getRenderTarget(mFaceTarget, mLevel);
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

unsigned int TextureCubeMapAttachment::getSerial() const
{
    return mTextureCubeMap->getRenderTargetSerial(mFaceTarget, mLevel);
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

Texture *TextureCubeMapAttachment::getTexture() const
{
    return mTextureCubeMap.get();
}

///// Texture3DAttachment Implementation ////////

Texture3DAttachment::Texture3DAttachment(GLenum binding, Texture3D *texture, GLint level, GLint layer)
    : TextureAttachment(binding),
      mLevel(level),
      mLayer(layer)
{
    mTexture3D.set(texture);
}

Texture3DAttachment::~Texture3DAttachment()
{
    mTexture3D.set(NULL);
}

rx::RenderTarget *Texture3DAttachment::getRenderTarget()
{
    return mTexture3D->getRenderTarget(mLevel, mLayer);
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

unsigned int Texture3DAttachment::getSerial() const
{
    return mTexture3D->getRenderTargetSerial(mLevel, mLayer);
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

Texture *Texture3DAttachment::getTexture() const
{
    return mTexture3D.get();
}

////// Texture2DArrayAttachment Implementation //////

Texture2DArrayAttachment::Texture2DArrayAttachment(GLenum binding, Texture2DArray *texture, GLint level, GLint layer)
    : TextureAttachment(binding),
      mLevel(level),
      mLayer(layer)
{
    mTexture2DArray.set(texture);
}

Texture2DArrayAttachment::~Texture2DArrayAttachment()
{
    mTexture2DArray.set(NULL);
}

rx::RenderTarget *Texture2DArrayAttachment::getRenderTarget()
{
    return mTexture2DArray->getRenderTarget(mLevel, mLayer);
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

unsigned int Texture2DArrayAttachment::getSerial() const
{
    return mTexture2DArray->getRenderTargetSerial(mLevel, mLayer);
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

Texture *Texture2DArrayAttachment::getTexture() const
{
    return mTexture2DArray.get();
}

////// RenderbufferAttachment Implementation //////

RenderbufferAttachment::RenderbufferAttachment(GLenum binding, Renderbuffer *renderbuffer)
    : FramebufferAttachment(binding)
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
