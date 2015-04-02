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

FramebufferAttachment::FramebufferAttachment(GLenum binding,
                                             const ImageIndex &textureIndex,
                                             RefCountObject *resource)
    : mBinding(binding),
      mTextureIndex(textureIndex)
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

const ImageIndex *FramebufferAttachment::getTextureImageIndex() const
{
    ASSERT(type() == GL_TEXTURE);
    return &mTextureIndex;
}

GLenum FramebufferAttachment::cubeMapFace() const
{
    ASSERT(type() == GL_TEXTURE);
    return IsCubeMapTextureTarget(mTextureIndex.type) ? mTextureIndex.type : GL_NONE;
}

GLint FramebufferAttachment::mipLevel() const
{
    ASSERT(type() == GL_TEXTURE);
    return mTextureIndex.mipIndex;
}

GLint FramebufferAttachment::layer() const
{
    ASSERT(type() == GL_TEXTURE);
    if (mTextureIndex.type == GL_TEXTURE_2D_ARRAY || mTextureIndex.type == GL_TEXTURE_3D)
    {
        return mTextureIndex.layerIndex;
    }
    return 0;
}

///// TextureAttachment Implementation ////////

TextureAttachment::TextureAttachment(GLenum binding, Texture *texture, const ImageIndex &index)
    : FramebufferAttachment(binding, index, texture)
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
    return getTexture()->getWidth(mTextureIndex.type, mTextureIndex.mipIndex);
}

GLsizei TextureAttachment::getHeight() const
{
    return getTexture()->getHeight(mTextureIndex.type, mTextureIndex.mipIndex);
}

GLenum TextureAttachment::getInternalFormat() const
{
    return getTexture()->getInternalFormat(mTextureIndex.type, mTextureIndex.mipIndex);
}

GLenum TextureAttachment::type() const
{
    return GL_TEXTURE;
}

Renderbuffer *TextureAttachment::getRenderbuffer() const
{
    UNREACHABLE();
    return nullptr;
}

////// RenderbufferAttachment Implementation //////

RenderbufferAttachment::RenderbufferAttachment(GLenum binding, Renderbuffer *renderbuffer)
    : FramebufferAttachment(binding, ImageIndex::MakeInvalid(), renderbuffer)
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

Texture *RenderbufferAttachment::getTexture() const
{
    UNREACHABLE();
    return nullptr;
}

DefaultAttachment::DefaultAttachment(GLenum binding, egl::Surface *surface)
    : FramebufferAttachment(binding, ImageIndex::MakeInvalid(), surface)
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

Texture *DefaultAttachment::getTexture() const
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
