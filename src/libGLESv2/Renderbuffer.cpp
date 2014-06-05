#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderbuffer.cpp: the gl::Renderbuffer class and its derived classes
// Colorbuffer, Depthbuffer and Stencilbuffer. Implements GL renderbuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4.3 page 108.

#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/renderer/RenderTarget.h"

#include "libGLESv2/Texture.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "common/utilities.h"
#include "libGLESv2/formatutils.h"

namespace gl
{
unsigned int RenderbufferStorage::mCurrentSerial = 1;

FramebufferAttachmentInterface::FramebufferAttachmentInterface()
{
}

// The default case for classes inherited from FramebufferAttachmentInterface is not to
// need to do anything upon the reference count to the parent FramebufferAttachment incrementing
// or decrementing. 
void FramebufferAttachmentInterface::addProxyRef(const FramebufferAttachment *proxy)
{
}

void FramebufferAttachmentInterface::releaseProxy(const FramebufferAttachment *proxy)
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

bool Texture2DAttachment::isTexture() const
{
    return true;
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

bool TextureCubeMapAttachment::isTexture() const
{
    return true;
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

bool Texture3DAttachment::isTexture() const
{
    return true;
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

bool Texture2DArrayAttachment::isTexture() const
{
    return true;
}

unsigned int Texture2DArrayAttachment::getTextureSerial() const
{
    return mTexture2DArray->getTextureSerial();
}

////// FramebufferAttachment Implementation //////

FramebufferAttachment::FramebufferAttachment(rx::Renderer *renderer, GLuint id, FramebufferAttachmentInterface *instance) : RefCountObject(id)
{
    ASSERT(instance != NULL);
    mInstance = instance;

    ASSERT(renderer != NULL);
    mRenderer = renderer;
}

FramebufferAttachment::~FramebufferAttachment()
{
    delete mInstance;
}

// The FramebufferAttachmentInterface contained in this FramebufferAttachment may need to maintain
// its own reference count, so we pass it on here.
void FramebufferAttachment::addRef() const
{
    mInstance->addProxyRef(this);

    RefCountObject::addRef();
}

void FramebufferAttachment::release() const
{
    mInstance->releaseProxy(this);

    RefCountObject::release();
}

rx::RenderTarget *FramebufferAttachment::getRenderTarget()
{
    return mInstance->getRenderTarget();
}

rx::RenderTarget *FramebufferAttachment::getDepthStencil()
{
    return mInstance->getDepthStencil();
}

rx::TextureStorage *FramebufferAttachment::getTextureStorage()
{
    return mInstance->getTextureStorage();
}

GLsizei FramebufferAttachment::getWidth() const
{
    return mInstance->getWidth();
}

GLsizei FramebufferAttachment::getHeight() const
{
    return mInstance->getHeight();
}

GLenum FramebufferAttachment::getInternalFormat() const
{
    return mInstance->getInternalFormat();
}

GLenum FramebufferAttachment::getActualFormat() const
{
    return mInstance->getActualFormat();
}

GLuint FramebufferAttachment::getRedSize() const
{
    return gl::GetRedBits(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLuint FramebufferAttachment::getGreenSize() const
{
    return gl::GetGreenBits(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLuint FramebufferAttachment::getBlueSize() const
{
    return gl::GetBlueBits(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLuint FramebufferAttachment::getAlphaSize() const
{
    return gl::GetAlphaBits(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLuint FramebufferAttachment::getDepthSize() const
{
    return gl::GetDepthBits(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLuint FramebufferAttachment::getStencilSize() const
{
    return gl::GetStencilBits(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLenum FramebufferAttachment::getComponentType() const
{
    return gl::GetComponentType(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLenum FramebufferAttachment::getColorEncoding() const
{
    return gl::GetColorEncoding(getActualFormat(), mRenderer->getCurrentClientVersion());
}

GLsizei FramebufferAttachment::getSamples() const
{
    return mInstance->getSamples();
}

unsigned int FramebufferAttachment::getSerial() const
{
    return mInstance->getSerial();
}

bool FramebufferAttachment::isTexture() const
{
    return mInstance->isTexture();
}

unsigned int FramebufferAttachment::getTextureSerial() const
{
    return mInstance->getTextureSerial();
}

void FramebufferAttachment::setStorage(RenderbufferStorage *newStorage)
{
    ASSERT(newStorage != NULL);

    delete mInstance;
    mInstance = newStorage;
}

RenderbufferStorage::RenderbufferStorage() : mSerial(issueSerials(1))
{
    mWidth = 0;
    mHeight = 0;
    mInternalFormat = GL_RGBA4;
    mActualFormat = GL_RGBA8_OES;
    mSamples = 0;
}

RenderbufferStorage::~RenderbufferStorage()
{
}

rx::RenderTarget *RenderbufferStorage::getRenderTarget()
{
    return NULL;
}

rx::RenderTarget *RenderbufferStorage::getDepthStencil()
{
    return NULL;
}

rx::TextureStorage *RenderbufferStorage::getTextureStorage()
{
    return NULL;
}

GLsizei RenderbufferStorage::getWidth() const
{
    return mWidth;
}

GLsizei RenderbufferStorage::getHeight() const
{
    return mHeight;
}

GLenum RenderbufferStorage::getInternalFormat() const
{
    return mInternalFormat;
}

GLenum RenderbufferStorage::getActualFormat() const
{
    return mActualFormat;
}

GLsizei RenderbufferStorage::getSamples() const
{
    return mSamples;
}

unsigned int RenderbufferStorage::getSerial() const
{
    return mSerial;
}

unsigned int RenderbufferStorage::issueSerials(GLuint count)
{
    unsigned int firstSerial = mCurrentSerial;
    mCurrentSerial += count;
    return firstSerial;
}

bool RenderbufferStorage::isTexture() const
{
    return false;
}

unsigned int RenderbufferStorage::getTextureSerial() const
{
    return -1;
}

Colorbuffer::Colorbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain)
{
    mRenderTarget = renderer->createRenderTarget(swapChain, false); 

    if (mRenderTarget)
    {
        mWidth = mRenderTarget->getWidth();
        mHeight = mRenderTarget->getHeight();
        mInternalFormat = mRenderTarget->getInternalFormat();
        mActualFormat = mRenderTarget->getActualFormat();
        mSamples = mRenderTarget->getSamples();
    }
}

Colorbuffer::Colorbuffer(rx::Renderer *renderer, int width, int height, GLenum format, GLsizei samples) : mRenderTarget(NULL)
{
    mRenderTarget = renderer->createRenderTarget(width, height, format, samples);

    if (mRenderTarget)
    {
        mWidth = width;
        mHeight = height;
        mInternalFormat = format;
        mActualFormat = mRenderTarget->getActualFormat();
        mSamples = mRenderTarget->getSamples();
    }
}

Colorbuffer::~Colorbuffer()
{
    if (mRenderTarget)
    {
        delete mRenderTarget;
    }
}

rx::RenderTarget *Colorbuffer::getRenderTarget()
{
    return mRenderTarget;
}

DepthStencilbuffer::DepthStencilbuffer(rx::Renderer *renderer, rx::SwapChain *swapChain)
{
    mDepthStencil = renderer->createRenderTarget(swapChain, true);
    if (mDepthStencil)
    {
        mWidth = mDepthStencil->getWidth();
        mHeight = mDepthStencil->getHeight();
        mInternalFormat = mDepthStencil->getInternalFormat();
        mSamples = mDepthStencil->getSamples();
        mActualFormat = mDepthStencil->getActualFormat();
    }
}

DepthStencilbuffer::DepthStencilbuffer(rx::Renderer *renderer, int width, int height, GLsizei samples)
{

    mDepthStencil = renderer->createRenderTarget(width, height, GL_DEPTH24_STENCIL8_OES, samples);

    mWidth = mDepthStencil->getWidth();
    mHeight = mDepthStencil->getHeight();
    mInternalFormat = GL_DEPTH24_STENCIL8_OES;
    mActualFormat = mDepthStencil->getActualFormat();
    mSamples = mDepthStencil->getSamples();
}

DepthStencilbuffer::~DepthStencilbuffer()
{
    if (mDepthStencil)
    {
        delete mDepthStencil;
    }
}

rx::RenderTarget *DepthStencilbuffer::getDepthStencil()
{
    return mDepthStencil;
}

Depthbuffer::Depthbuffer(rx::Renderer *renderer, int width, int height, GLsizei samples) : DepthStencilbuffer(renderer, width, height, samples)
{
    if (mDepthStencil)
    {
        mInternalFormat = GL_DEPTH_COMPONENT16;   // If the renderbuffer parameters are queried, the calling function
                                                  // will expect one of the valid renderbuffer formats for use in 
                                                  // glRenderbufferStorage
    }
}

Depthbuffer::~Depthbuffer()
{
}

Stencilbuffer::Stencilbuffer(rx::Renderer *renderer, int width, int height, GLsizei samples) : DepthStencilbuffer(renderer, width, height, samples)
{
    if (mDepthStencil)
    {
        mInternalFormat = GL_STENCIL_INDEX8;   // If the renderbuffer parameters are queried, the calling function
                                               // will expect one of the valid renderbuffer formats for use in 
                                               // glRenderbufferStorage
    }
}

Stencilbuffer::~Stencilbuffer()
{
}

}
