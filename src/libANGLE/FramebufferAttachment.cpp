//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferAttachment.cpp: the gl::FramebufferAttachment class and its derived classes
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#include "libANGLE/FramebufferAttachment.h"

#include "common/utilities.h"
#include "libANGLE/Config.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/Surface.h"
#include "libANGLE/Texture.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/FramebufferImpl.h"

namespace gl
{

////// FramebufferAttachment Implementation //////

FramebufferAttachment::FramebufferAttachment(GLenum binding, RefCountObject *resource)
    : mBinding(binding)
{
    mResource.set(resource);
}

FramebufferAttachment::~FramebufferAttachment()
{
    mResource.set(nullptr);
}

GLuint FramebufferAttachment::getRedSize() const
{
    return GetInternalFormatInfo(getInternalFormat()).redBits;
}

GLuint FramebufferAttachment::getGreenSize() const
{
    return GetInternalFormatInfo(getInternalFormat()).greenBits;
}

GLuint FramebufferAttachment::getBlueSize() const
{
    return GetInternalFormatInfo(getInternalFormat()).blueBits;
}

GLuint FramebufferAttachment::getAlphaSize() const
{
    return GetInternalFormatInfo(getInternalFormat()).alphaBits;
}

GLuint FramebufferAttachment::getDepthSize() const
{
    return GetInternalFormatInfo(getInternalFormat()).depthBits;
}

GLuint FramebufferAttachment::getStencilSize() const
{
    return GetInternalFormatInfo(getInternalFormat()).stencilBits;
}

GLenum FramebufferAttachment::getComponentType() const
{
    return GetInternalFormatInfo(getInternalFormat()).componentType;
}

GLenum FramebufferAttachment::getColorEncoding() const
{
    return GetInternalFormatInfo(getInternalFormat()).colorEncoding;
}

GLuint FramebufferAttachment::id() const
{
    return mResource->id();
}

///// TextureAttachment Implementation ////////

TextureAttachment::TextureAttachment(GLenum binding, Texture *texture, const ImageIndex &index)
    : FramebufferAttachment(binding, texture),
      mIndex(index)
{
}

TextureAttachment::~TextureAttachment()
{
}

GLsizei TextureAttachment::getSamples() const
{
    return 0;
}

GLsizei TextureAttachment::getWidth() const
{
    return getTexture()->getWidth(mIndex.type, mIndex.mipIndex);
}

GLsizei TextureAttachment::getHeight() const
{
    return getTexture()->getHeight(mIndex.type, mIndex.mipIndex);
}

GLenum TextureAttachment::getInternalFormat() const
{
    return getTexture()->getInternalFormat(mIndex.type, mIndex.mipIndex);
}

GLenum TextureAttachment::type() const
{
    return GL_TEXTURE;
}

GLint TextureAttachment::mipLevel() const
{
    return mIndex.mipIndex;
}

GLenum TextureAttachment::cubeMapFace() const
{
    return IsCubeMapTextureTarget(mIndex.type) ? mIndex.type : GL_NONE;
}

GLint TextureAttachment::layer() const
{
    return mIndex.layerIndex;
}

const ImageIndex *TextureAttachment::getTextureImageIndex() const
{
    return &mIndex;
}

Renderbuffer *TextureAttachment::getRenderbuffer() const
{
    UNREACHABLE();
    return nullptr;
}

////// RenderbufferAttachment Implementation //////

RenderbufferAttachment::RenderbufferAttachment(GLenum binding, Renderbuffer *renderbuffer)
    : FramebufferAttachment(binding, renderbuffer)
{
    ASSERT(renderbuffer);
}

RenderbufferAttachment::~RenderbufferAttachment()
{
}

GLsizei RenderbufferAttachment::getWidth() const
{
    return getRenderbuffer()->getWidth();
}

GLsizei RenderbufferAttachment::getHeight() const
{
    return getRenderbuffer()->getHeight();
}

GLenum RenderbufferAttachment::getInternalFormat() const
{
    return getRenderbuffer()->getInternalFormat();
}

GLsizei RenderbufferAttachment::getSamples() const
{
    return getRenderbuffer()->getSamples();
}

GLenum RenderbufferAttachment::type() const
{
    return GL_RENDERBUFFER;
}

GLint RenderbufferAttachment::mipLevel() const
{
    return 0;
}

GLenum RenderbufferAttachment::cubeMapFace() const
{
    return GL_NONE;
}

GLint RenderbufferAttachment::layer() const
{
    return 0;
}

Texture *RenderbufferAttachment::getTexture() const
{
    UNREACHABLE();
    return nullptr;
}

const ImageIndex *RenderbufferAttachment::getTextureImageIndex() const
{
    UNREACHABLE();
    return nullptr;
}

DefaultAttachment::DefaultAttachment(GLenum binding, egl::Surface *surface)
    : FramebufferAttachment(binding, surface)
{
}

DefaultAttachment::~DefaultAttachment()
{
}

GLsizei DefaultAttachment::getWidth() const
{
    return getSurface()->getWidth();
}

GLsizei DefaultAttachment::getHeight() const
{
    return getSurface()->getHeight();
}

GLenum DefaultAttachment::getInternalFormat() const
{
    const egl::Config *config = getSurface()->getConfig();
    return (getBinding() == GL_BACK ? config->renderTargetFormat : config->depthStencilFormat);
}

GLsizei DefaultAttachment::getSamples() const
{
    const egl::Config *config = getSurface()->getConfig();
    return config->samples;
}

GLenum DefaultAttachment::type() const
{
    return GL_FRAMEBUFFER_DEFAULT;
}

GLint DefaultAttachment::mipLevel() const
{
    return 0;
}

GLenum DefaultAttachment::cubeMapFace() const
{
    return GL_NONE;
}

GLint DefaultAttachment::layer() const
{
    return 0;
}

Texture *DefaultAttachment::getTexture() const
{
    UNREACHABLE();
    return nullptr;
}

const ImageIndex *DefaultAttachment::getTextureImageIndex() const
{
    UNREACHABLE();
    return nullptr;
}

Renderbuffer *DefaultAttachment::getRenderbuffer() const
{
    UNREACHABLE();
    return nullptr;
}

}
