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

TextureAttachment::TextureAttachment(GLenum binding, Texture *texture, const ImageIndex &index)
    : FramebufferAttachment(binding),
      mIndex(index)
{
    mTexture.set(texture);
}

TextureAttachment::~TextureAttachment()
{
    mTexture.set(NULL);
}

rx::TextureStorage *TextureAttachment::getTextureStorage()
{
    return mTexture->getNativeTexture()->getStorageInstance();
}

GLsizei TextureAttachment::getSamples() const
{
    return 0;
}

GLuint TextureAttachment::id() const
{
    return mTexture->id();
}

unsigned int TextureAttachment::getTextureSerial() const
{
    return mTexture->getTextureSerial();
}

GLsizei TextureAttachment::getWidth() const
{
    return mTexture->getWidth(mIndex);
}

GLsizei TextureAttachment::getHeight() const
{
    return mTexture->getHeight(mIndex);
}

GLenum TextureAttachment::getInternalFormat() const
{
    return mTexture->getInternalFormat(mIndex);
}

GLenum TextureAttachment::getActualFormat() const
{
    return mTexture->getActualFormat(mIndex);
}

GLenum TextureAttachment::type() const
{
    return mIndex.type;
}

GLint TextureAttachment::mipLevel() const
{
    return mIndex.mipIndex;
}

GLint TextureAttachment::layer() const
{
    return mIndex.layerIndex;
}

rx::RenderTarget *TextureAttachment::getRenderTarget()
{
    return mTexture->getRenderTarget(mIndex);
}

unsigned int TextureAttachment::getSerial() const
{
    return mTexture->getRenderTargetSerial(mIndex);
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
