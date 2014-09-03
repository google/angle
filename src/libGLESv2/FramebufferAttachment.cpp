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

TextureAttachment::TextureAttachment(GLenum binding, const ImageIndex &index)
    : FramebufferAttachment(binding),
      mIndex(index)
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

GLsizei TextureAttachment::getWidth() const
{
    return getTexture()->getWidth(mIndex);
}

GLsizei TextureAttachment::getHeight() const
{
    return getTexture()->getHeight(mIndex);
}

GLenum TextureAttachment::getInternalFormat() const
{
    return getTexture()->getInternalFormat(mIndex);
}

GLenum TextureAttachment::getActualFormat() const
{
    return getTexture()->getActualFormat(mIndex);
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
    return getTexture()->getRenderTarget(mIndex);
}

unsigned int TextureAttachment::getSerial() const
{
    return getTexture()->getRenderTargetSerial(mIndex);
}

///// Texture2DAttachment Implementation ////////

Texture2DAttachment::Texture2DAttachment(GLenum binding, Texture2D *texture, GLint level)
    : TextureAttachment(binding, ImageIndex::Make2D(level)),
      mLevel(level)
{
    mTexture2D.set(texture);
}

Texture2DAttachment::~Texture2DAttachment()
{
    mTexture2D.set(NULL);
}

Texture *Texture2DAttachment::getTexture() const
{
    return mTexture2D.get();
}

///// TextureCubeMapAttachment Implementation ////////

TextureCubeMapAttachment::TextureCubeMapAttachment(GLenum binding, TextureCubeMap *texture, GLenum faceTarget, GLint level)
    : TextureAttachment(binding, ImageIndex::MakeCube(faceTarget, level)),
      mFaceTarget(faceTarget),
      mLevel(level)
{
    mTextureCubeMap.set(texture);
}

TextureCubeMapAttachment::~TextureCubeMapAttachment()
{
    mTextureCubeMap.set(NULL);
}

Texture *TextureCubeMapAttachment::getTexture() const
{
    return mTextureCubeMap.get();
}

///// Texture3DAttachment Implementation ////////

Texture3DAttachment::Texture3DAttachment(GLenum binding, Texture3D *texture, GLint level, GLint layer)
    : TextureAttachment(binding, ImageIndex::Make3D(level, layer)),
      mLevel(level),
      mLayer(layer)
{
    mTexture3D.set(texture);
}

Texture3DAttachment::~Texture3DAttachment()
{
    mTexture3D.set(NULL);
}

Texture *Texture3DAttachment::getTexture() const
{
    return mTexture3D.get();
}

////// Texture2DArrayAttachment Implementation //////

Texture2DArrayAttachment::Texture2DArrayAttachment(GLenum binding, Texture2DArray *texture, GLint level, GLint layer)
    : TextureAttachment(binding, ImageIndex::Make2DArray(level, layer)),
      mLevel(level),
      mLayer(layer)
{
    mTexture2DArray.set(texture);
}

Texture2DArrayAttachment::~Texture2DArrayAttachment()
{
    mTexture2DArray.set(NULL);
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
