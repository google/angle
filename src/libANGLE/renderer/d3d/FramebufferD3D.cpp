//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferD3D.cpp: Implements the DefaultAttachmentD3D and FramebufferD3D classes.

#include "libANGLE/renderer/d3d/FramebufferD3D.h"
#include "libANGLE/renderer/d3d/TextureD3D.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/RenderTargetD3D.h"
#include "libANGLE/renderer/d3d/RenderbufferD3D.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"

namespace rx
{

DefaultAttachmentD3D::DefaultAttachmentD3D(RenderTargetD3D *renderTarget)
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

GLsizei DefaultAttachmentD3D::getSamples() const
{
    return mRenderTarget->getSamples();
}

RenderTargetD3D *DefaultAttachmentD3D::getRenderTarget() const
{
    return mRenderTarget;
}


FramebufferD3D::FramebufferD3D(RendererD3D *renderer)
    : mRenderer(renderer),
      mColorBuffers(renderer->getRendererCaps().maxColorAttachments),
      mDepthbuffer(nullptr),
      mStencilbuffer(nullptr),
      mDrawBuffers(renderer->getRendererCaps().maxDrawBuffers),
      mReadBuffer(GL_COLOR_ATTACHMENT0)
{
    ASSERT(mRenderer != nullptr);

    std::fill(mColorBuffers.begin(), mColorBuffers.end(), nullptr);

    ASSERT(mDrawBuffers.size() > 0);
    mDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    std::fill(mDrawBuffers.begin() + 1, mDrawBuffers.end(), GL_NONE);
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
    mDepthbuffer = attachment;
}

void FramebufferD3D::setStencilAttachment(const gl::FramebufferAttachment *attachment)
{
    mStencilbuffer = attachment;
}

void FramebufferD3D::setDepthStencilAttachment(const gl::FramebufferAttachment *attachment)
{
    mDepthbuffer = attachment;
    mStencilbuffer = attachment;
}

void FramebufferD3D::setDrawBuffers(size_t count, const GLenum *buffers)
{
    std::copy_n(buffers, count, mDrawBuffers.begin());
    std::fill(mDrawBuffers.begin() + count, mDrawBuffers.end(), GL_NONE);
}

void FramebufferD3D::setReadBuffer(GLenum buffer)
{
    mReadBuffer = buffer;
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

gl::Error FramebufferD3D::clear(const gl::State &state, GLbitfield mask)
{
    gl::ClearParameters clearParams = state.getClearParameters(mask);
    return clear(state, clearParams);
}

gl::Error FramebufferD3D::clearBufferfv(const gl::State &state, GLenum buffer, GLint drawbuffer, const GLfloat *values)
{
    // glClearBufferfv can be called to clear the color buffer or depth buffer
    gl::ClearParameters clearParams = state.getClearParameters(0);

    if (buffer == GL_COLOR)
    {
        for (unsigned int i = 0; i < ArraySize(clearParams.clearColor); i++)
        {
            clearParams.clearColor[i] = (drawbuffer == static_cast<int>(i));
        }
        clearParams.colorFClearValue = gl::ColorF(values[0], values[1], values[2], values[3]);
        clearParams.colorClearType = GL_FLOAT;
    }

    if (buffer == GL_DEPTH)
    {
        clearParams.clearDepth = true;
        clearParams.depthClearValue = values[0];
    }

    return clear(state, clearParams);
}

gl::Error FramebufferD3D::clearBufferuiv(const gl::State &state, GLenum buffer, GLint drawbuffer, const GLuint *values)
{
    // glClearBufferuiv can only be called to clear a color buffer
    gl::ClearParameters clearParams = state.getClearParameters(0);
    for (unsigned int i = 0; i < ArraySize(clearParams.clearColor); i++)
    {
        clearParams.clearColor[i] = (drawbuffer == static_cast<int>(i));
    }
    clearParams.colorUIClearValue = gl::ColorUI(values[0], values[1], values[2], values[3]);
    clearParams.colorClearType = GL_UNSIGNED_INT;

    return clear(state, clearParams);
}

gl::Error FramebufferD3D::clearBufferiv(const gl::State &state, GLenum buffer, GLint drawbuffer, const GLint *values)
{
    // glClearBufferiv can be called to clear the color buffer or stencil buffer
    gl::ClearParameters clearParams = state.getClearParameters(0);

    if (buffer == GL_COLOR)
    {
        for (unsigned int i = 0; i < ArraySize(clearParams.clearColor); i++)
        {
            clearParams.clearColor[i] = (drawbuffer == static_cast<int>(i));
        }
        clearParams.colorIClearValue = gl::ColorI(values[0], values[1], values[2], values[3]);
        clearParams.colorClearType = GL_INT;
    }

    if (buffer == GL_STENCIL)
    {
        clearParams.clearStencil = true;
        clearParams.stencilClearValue = values[1];
    }

    return clear(state, clearParams);
}

gl::Error FramebufferD3D::clearBufferfi(const gl::State &state, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    // glClearBufferfi can only be called to clear a depth stencil buffer
    gl::ClearParameters clearParams = state.getClearParameters(0);
    clearParams.clearDepth = true;
    clearParams.depthClearValue = depth;
    clearParams.clearStencil = true;
    clearParams.stencilClearValue = stencil;

    return clear(state, clearParams);
}

const gl::FramebufferAttachment *FramebufferD3D::getReadAttachment() const
{
    ASSERT(mReadBuffer == GL_BACK || (mReadBuffer >= GL_COLOR_ATTACHMENT0 && mReadBuffer <= GL_COLOR_ATTACHMENT15));
    size_t readIndex = (mReadBuffer == GL_BACK ? 0 : static_cast<size_t>(mReadBuffer - GL_COLOR_ATTACHMENT0));
    ASSERT(readIndex < mColorBuffers.size());
    return mColorBuffers[readIndex];
}

GLenum FramebufferD3D::getImplementationColorReadFormat() const
{
    const gl::FramebufferAttachment *readAttachment = getReadAttachment();

    if (readAttachment == nullptr)
    {
        return GL_NONE;
    }

    RenderTargetD3D *attachmentRenderTarget = NULL;
    gl::Error error = GetAttachmentRenderTarget(readAttachment, &attachmentRenderTarget);
    if (error.isError())
    {
        return GL_NONE;
    }

    GLenum implementationFormat = getRenderTargetImplementationFormat(attachmentRenderTarget);
    const gl::InternalFormat &implementationFormatInfo = gl::GetInternalFormatInfo(implementationFormat);

    return implementationFormatInfo.format;
}

GLenum FramebufferD3D::getImplementationColorReadType() const
{
    const gl::FramebufferAttachment *readAttachment = getReadAttachment();

    if (readAttachment == nullptr)
    {
        return GL_NONE;
    }

    RenderTargetD3D *attachmentRenderTarget = NULL;
    gl::Error error = GetAttachmentRenderTarget(readAttachment, &attachmentRenderTarget);
    if (error.isError())
    {
        return GL_NONE;
    }

    GLenum implementationFormat = getRenderTargetImplementationFormat(attachmentRenderTarget);
    const gl::InternalFormat &implementationFormatInfo = gl::GetInternalFormatInfo(implementationFormat);

    return implementationFormatInfo.type;
}

gl::Error FramebufferD3D::readPixels(const gl::State &state, const gl::Rectangle &area, GLenum format, GLenum type, GLvoid *pixels) const
{
    GLenum sizedInternalFormat = gl::GetSizedInternalFormat(format, type);
    const gl::InternalFormat &sizedFormatInfo = gl::GetInternalFormatInfo(sizedInternalFormat);
    GLuint outputPitch = sizedFormatInfo.computeRowPitch(type, area.width, state.getPackAlignment(), 0);

    return readPixels(area, format, type, outputPitch, state.getPackState(), reinterpret_cast<uint8_t*>(pixels));
}

gl::Error FramebufferD3D::blit(const gl::State &state, const gl::Rectangle &sourceArea, const gl::Rectangle &destArea,
                               GLbitfield mask, GLenum filter, const gl::Framebuffer *sourceFramebuffer)
{
    bool blitRenderTarget = false;
    if ((mask & GL_COLOR_BUFFER_BIT) &&
        sourceFramebuffer->getReadColorbuffer() != nullptr &&
        std::any_of(mColorBuffers.begin(), mColorBuffers.end(), [](const gl::FramebufferAttachment* attachment){ return attachment != nullptr; }))
    {
        blitRenderTarget = true;
    }

    bool blitStencil = false;
    if ((mask & GL_STENCIL_BUFFER_BIT) &&
        sourceFramebuffer->getStencilbuffer() != nullptr &&
        mStencilbuffer != nullptr)
    {
        blitStencil = true;
    }

    bool blitDepth = false;
    if ((mask & GL_DEPTH_BUFFER_BIT) &&
        sourceFramebuffer->getDepthbuffer() != nullptr &&
        mDepthbuffer != nullptr)
    {
        blitDepth = true;
    }

    if (blitRenderTarget || blitDepth || blitStencil)
    {
        const gl::Rectangle *scissor = state.isScissorTestEnabled() ? &state.getScissor() : NULL;
        gl::Error error = blit(sourceArea, destArea, scissor, blitRenderTarget, blitDepth, blitStencil,
                               filter, sourceFramebuffer);
        if (error.isError())
        {
            return error;
        }
    }

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

gl::Error GetAttachmentRenderTarget(const gl::FramebufferAttachment *attachment, RenderTargetD3D **outRT)
{
    if (attachment->type() == GL_TEXTURE)
    {
        gl::Texture *texture = attachment->getTexture();
        ASSERT(texture);
        TextureD3D *textureD3D = GetImplAs<TextureD3D>(texture);
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
        TextureD3D *textureD3D = GetImplAs<TextureD3D>(texture);
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
