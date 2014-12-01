//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferD3D.cpp: Implements the DefaultAttachmentD3D and FramebufferD3D classes.

#include "libANGLE/renderer/d3d/FramebufferD3D.h"
#include "libANGLE/renderer/d3d/TextureD3D.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/RenderbufferD3D.h"
#include "libANGLE/renderer/RenderTarget.h"
#include "libANGLE/FramebufferAttachment.h"

namespace rx
{

DefaultAttachmentD3D::DefaultAttachmentD3D(RenderTarget *renderTarget)
    : mRenderTarget(renderTarget)
{
    ASSERT(mRenderTarget);
}

DefaultAttachmentD3D::~DefaultAttachmentD3D()
{
    SafeDelete(mRenderTarget);
}

DefaultAttachmentD3D *DefaultAttachmentD3D::makeDefaultAttachmentD3D(DefaultAttachmentImpl* impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(DefaultAttachmentD3D*, impl));
    return static_cast<DefaultAttachmentD3D*>(impl);
}

GLsizei DefaultAttachmentD3D::getWidth() const
{
    return mRenderTarget->getWidth();
}

GLsizei DefaultAttachmentD3D::getHeight() const
{
    return mRenderTarget->getHeight();
}

GLenum DefaultAttachmentD3D::getInternalFormat() const
{
    return mRenderTarget->getInternalFormat();
}

GLenum DefaultAttachmentD3D::getActualFormat() const
{
    return mRenderTarget->getActualFormat();
}

GLsizei DefaultAttachmentD3D::getSamples() const
{
    return mRenderTarget->getSamples();
}

RenderTarget *DefaultAttachmentD3D::getRenderTarget() const
{
    return mRenderTarget;
}


FramebufferD3D::FramebufferD3D(RendererD3D *renderer)
    : mRenderer(renderer),
      mColorBuffers(renderer->getRendererCaps().maxColorAttachments)
{
    ASSERT(mRenderer != nullptr);

    std::fill(mColorBuffers.begin(), mColorBuffers.end(), nullptr);
}

FramebufferD3D::~FramebufferD3D()
{
}

void FramebufferD3D::setColorAttachment(size_t index, const gl::FramebufferAttachment *attachment)
{
    ASSERT(index < mColorBuffers.size());
    mColorBuffers[index] = attachment;
}

void FramebufferD3D::setDepthttachment(const gl::FramebufferAttachment *attachment)
{
}

void FramebufferD3D::setStencilAttachment(const gl::FramebufferAttachment *attachment)
{
}

void FramebufferD3D::setDepthStencilAttachment(const gl::FramebufferAttachment *attachment)
{
}

void FramebufferD3D::setDrawBuffers(size_t count, const GLenum *buffers)
{
}

void FramebufferD3D::setReadBuffer(GLenum buffer)
{
}

gl::Error FramebufferD3D::invalidate(size_t, const GLenum *)
{
    // No-op in D3D
    return gl::Error(GL_NO_ERROR);
}

gl::Error FramebufferD3D::invalidateSub(size_t, const GLenum *, const gl::Rectangle &)
{
    // No-op in D3D
    return gl::Error(GL_NO_ERROR);
}

GLenum FramebufferD3D::checkStatus() const
{
    // D3D11 does not allow for overlapping RenderTargetViews, so ensure uniqueness
    for (size_t colorAttachment = 0; colorAttachment < mColorBuffers.size(); colorAttachment++)
    {
        const gl::FramebufferAttachment *attachment = mColorBuffers[colorAttachment];
        if (attachment != nullptr)
        {
            for (size_t prevColorAttachment = 0; prevColorAttachment < colorAttachment; prevColorAttachment++)
            {
                const gl::FramebufferAttachment *prevAttachment = mColorBuffers[prevColorAttachment];
                if (prevAttachment != nullptr &&
                    (attachment->id() == prevAttachment->id() &&
                     attachment->type() == prevAttachment->type()))
                {
                    return GL_FRAMEBUFFER_UNSUPPORTED;
                }
            }
        }
    }

    return GL_FRAMEBUFFER_COMPLETE;
}

gl::Error GetAttachmentRenderTarget(const gl::FramebufferAttachment *attachment, RenderTarget **outRT)
{
    if (attachment->type() == GL_TEXTURE)
    {
        gl::Texture *texture = attachment->getTexture();
        ASSERT(texture);
        TextureD3D *textureD3D = TextureD3D::makeTextureD3D(texture->getImplementation());
        const gl::ImageIndex *index = attachment->getTextureImageIndex();
        ASSERT(index);
        return textureD3D->getRenderTarget(*index, outRT);
    }
    else if (attachment->type() == GL_RENDERBUFFER)
    {
        gl::Renderbuffer *renderbuffer = attachment->getRenderbuffer();
        ASSERT(renderbuffer);
        RenderbufferD3D *renderbufferD3D = RenderbufferD3D::makeRenderbufferD3D(renderbuffer->getImplementation());
        *outRT = renderbufferD3D->getRenderTarget();
        return gl::Error(GL_NO_ERROR);
    }
    else if (attachment->type() == GL_FRAMEBUFFER_DEFAULT)
    {
        const gl::DefaultAttachment *defaultAttachment = static_cast<const gl::DefaultAttachment *>(attachment);
        DefaultAttachmentD3D *defaultAttachmentD3D = DefaultAttachmentD3D::makeDefaultAttachmentD3D(defaultAttachment->getImplementation());
        ASSERT(defaultAttachmentD3D);

        *outRT = defaultAttachmentD3D->getRenderTarget();
        return gl::Error(GL_NO_ERROR);
    }
    else
    {
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION);
    }
}

// Note: RenderTarget serials should ideally be in the RenderTargets themselves.
unsigned int GetAttachmentSerial(const gl::FramebufferAttachment *attachment)
{
    if (attachment->type() == GL_TEXTURE)
    {
        gl::Texture *texture = attachment->getTexture();
        ASSERT(texture);
        TextureD3D *textureD3D = TextureD3D::makeTextureD3D(texture->getImplementation());
        const gl::ImageIndex *index = attachment->getTextureImageIndex();
        ASSERT(index);
        return textureD3D->getRenderTargetSerial(*index);
    }
    else if (attachment->type() == GL_RENDERBUFFER)
    {
        gl::Renderbuffer *renderbuffer = attachment->getRenderbuffer();
        ASSERT(renderbuffer);
        RenderbufferD3D *renderbufferD3D = RenderbufferD3D::makeRenderbufferD3D(renderbuffer->getImplementation());
        return renderbufferD3D->getRenderTargetSerial();
    }
    else if (attachment->type() == GL_FRAMEBUFFER_DEFAULT)
    {
        const gl::DefaultAttachment *defaultAttachment = static_cast<const gl::DefaultAttachment *>(attachment);
        DefaultAttachmentD3D *defaultAttachmentD3D = DefaultAttachmentD3D::makeDefaultAttachmentD3D(defaultAttachment->getImplementation());
        ASSERT(defaultAttachmentD3D);
        return defaultAttachmentD3D->getRenderTarget()->getSerial();
    }
    else
    {
        UNREACHABLE();
        return 0;
    }
}

}
