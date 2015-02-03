//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer.cpp: Implements the gl::Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#include "libANGLE/Framebuffer.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/Texture.h"
#include "libANGLE/Context.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/renderer/FramebufferImpl.h"
#include "libANGLE/renderer/Renderer.h"
#include "libANGLE/renderer/RenderbufferImpl.h"
#include "libANGLE/renderer/Workarounds.h"

#include "common/utilities.h"

namespace gl
{

Framebuffer::Framebuffer(rx::FramebufferImpl *impl, GLuint id)
    : mImpl(impl),
      mId(id),
      mReadBufferState(GL_COLOR_ATTACHMENT0_EXT),
      mDepthbuffer(NULL),
      mStencilbuffer(NULL)
{
    ASSERT(mImpl != nullptr);

    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        mColorbuffers[colorAttachment] = NULL;
        mDrawBufferStates[colorAttachment] = GL_NONE;
    }
    mDrawBufferStates[0] = GL_COLOR_ATTACHMENT0_EXT;
}

Framebuffer::~Framebuffer()
{
    SafeDelete(mImpl);
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        SafeDelete(mColorbuffers[colorAttachment]);
    }
    SafeDelete(mDepthbuffer);
    SafeDelete(mStencilbuffer);
}

void Framebuffer::detachTexture(GLuint textureId)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        FramebufferAttachment *attachment = mColorbuffers[colorAttachment];

        if (attachment && attachment->isTextureWithId(textureId))
        {
            SafeDelete(mColorbuffers[colorAttachment]);
        }
    }

    if (mDepthbuffer && mDepthbuffer->isTextureWithId(textureId))
    {
        SafeDelete(mDepthbuffer);
    }

    if (mStencilbuffer && mStencilbuffer->isTextureWithId(textureId))
    {
        SafeDelete(mStencilbuffer);
    }
}

void Framebuffer::detachRenderbuffer(GLuint renderbufferId)
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        FramebufferAttachment *attachment = mColorbuffers[colorAttachment];

        if (attachment && attachment->isRenderbufferWithId(renderbufferId))
        {
            SafeDelete(mColorbuffers[colorAttachment]);
        }
    }

    if (mDepthbuffer && mDepthbuffer->isRenderbufferWithId(renderbufferId))
    {
        SafeDelete(mDepthbuffer);
    }

    if (mStencilbuffer && mStencilbuffer->isRenderbufferWithId(renderbufferId))
    {
        SafeDelete(mStencilbuffer);
    }
}

FramebufferAttachment *Framebuffer::getColorbuffer(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    return mColorbuffers[colorAttachment];
}

FramebufferAttachment *Framebuffer::getDepthbuffer() const
{
    return mDepthbuffer;
}

FramebufferAttachment *Framebuffer::getStencilbuffer() const
{
    return mStencilbuffer;
}

FramebufferAttachment *Framebuffer::getDepthStencilBuffer() const
{
    return (hasValidDepthStencil() ? mDepthbuffer : NULL);
}

FramebufferAttachment *Framebuffer::getDepthOrStencilbuffer() const
{
    FramebufferAttachment *depthstencilbuffer = mDepthbuffer;
    
    if (!depthstencilbuffer)
    {
        depthstencilbuffer = mStencilbuffer;
    }

    return depthstencilbuffer;
}

FramebufferAttachment *Framebuffer::getReadColorbuffer() const
{
    size_t readIndex = (mReadBufferState == GL_BACK ? 0 : static_cast<size_t>(mReadBufferState - GL_COLOR_ATTACHMENT0));
    ASSERT(readIndex < IMPLEMENTATION_MAX_DRAW_BUFFERS);
    return mColorbuffers[readIndex];
}

GLenum Framebuffer::getReadColorbufferType() const
{
    FramebufferAttachment *readAttachment = getReadColorbuffer();
    return (readAttachment ? readAttachment->type() : GL_NONE);
}

FramebufferAttachment *Framebuffer::getFirstColorbuffer() const
{
    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (mColorbuffers[colorAttachment])
        {
            return mColorbuffers[colorAttachment];
        }
    }

    return NULL;
}

FramebufferAttachment *Framebuffer::getAttachment(GLenum attachment) const
{
    if (attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT15)
    {
        return getColorbuffer(attachment - GL_COLOR_ATTACHMENT0);
    }
    else
    {
        switch (attachment)
        {
          case GL_COLOR:
          case GL_BACK:
            return getColorbuffer(0);
          case GL_DEPTH:
          case GL_DEPTH_ATTACHMENT:
            return getDepthbuffer();
          case GL_STENCIL:
          case GL_STENCIL_ATTACHMENT:
            return getStencilbuffer();
          case GL_DEPTH_STENCIL:
          case GL_DEPTH_STENCIL_ATTACHMENT:
            return getDepthStencilBuffer();
          default:
            UNREACHABLE();
            return NULL;
        }
    }
}

GLenum Framebuffer::getDrawBufferState(unsigned int colorAttachment) const
{
    return mDrawBufferStates[colorAttachment];
}

void Framebuffer::setDrawBuffers(size_t count, const GLenum *buffers)
{
    ASSERT(count <= ArraySize(mDrawBufferStates));
    std::copy(buffers, buffers + count, mDrawBufferStates);
    std::fill(mDrawBufferStates + count, mDrawBufferStates + ArraySize(mDrawBufferStates), GL_NONE);
    mImpl->setDrawBuffers(count, buffers);
}

GLenum Framebuffer::getReadBufferState() const
{
    return mReadBufferState;
}

void Framebuffer::setReadBuffer(GLenum buffer)
{
    ASSERT(buffer == GL_BACK || buffer == GL_NONE ||
           (buffer >= GL_COLOR_ATTACHMENT0 &&
            (buffer - GL_COLOR_ATTACHMENT0) < IMPLEMENTATION_MAX_DRAW_BUFFERS));
    mReadBufferState = buffer;
    mImpl->setReadBuffer(buffer);
}

bool Framebuffer::isEnabledColorAttachment(unsigned int colorAttachment) const
{
    return (mColorbuffers[colorAttachment] && mDrawBufferStates[colorAttachment] != GL_NONE);
}

bool Framebuffer::hasEnabledColorAttachment() const
{
    for (unsigned int colorAttachment = 0; colorAttachment < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (isEnabledColorAttachment(colorAttachment))
        {
            return true;
        }
    }

    return false;
}

bool Framebuffer::hasStencil() const
{
    return (mStencilbuffer && mStencilbuffer->getStencilSize() > 0);
}

bool Framebuffer::usingExtendedDrawBuffers() const
{
    for (unsigned int colorAttachment = 1; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        if (isEnabledColorAttachment(colorAttachment))
        {
            return true;
        }
    }

    return false;
}

GLenum Framebuffer::checkStatus(const gl::Data &data) const
{
    // The default framebuffer *must* always be complete, though it may not be
    // subject to the same rules as application FBOs. ie, it could have 0x0 size.
    if (mId == 0)
    {
        return GL_FRAMEBUFFER_COMPLETE;
    }

    int width = 0;
    int height = 0;
    unsigned int colorbufferSize = 0;
    int samples = -1;
    bool missingAttachment = true;

    for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
    {
        const FramebufferAttachment *colorbuffer = mColorbuffers[colorAttachment];

        if (colorbuffer)
        {
            if (colorbuffer->getWidth() == 0 || colorbuffer->getHeight() == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            GLenum internalformat = colorbuffer->getInternalFormat();
            const TextureCaps &formatCaps = data.textureCaps->get(internalformat);
            const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
            if (colorbuffer->type() == GL_TEXTURE)
            {
                if (!formatCaps.renderable)
                {
                    return GL_FRAMEBUFFER_UNSUPPORTED;
                }

                if (formatInfo.depthBits > 0 || formatInfo.stencilBits > 0)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                }
            }
            else if (colorbuffer->type() == GL_RENDERBUFFER)
            {
                if (!formatCaps.renderable || formatInfo.depthBits > 0 || formatInfo.stencilBits > 0)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                }
            }

            if (!missingAttachment)
            {
                // all color attachments must have the same width and height
                if (colorbuffer->getWidth() != width || colorbuffer->getHeight() != height)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
                }

                // APPLE_framebuffer_multisample, which EXT_draw_buffers refers to, requires that
                // all color attachments have the same number of samples for the FBO to be complete.
                if (colorbuffer->getSamples() != samples)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT;
                }

                // in GLES 2.0, all color attachments attachments must have the same number of bitplanes
                // in GLES 3.0, there is no such restriction
                if (data.clientVersion < 3)
                {
                    if (formatInfo.pixelBytes != colorbufferSize)
                    {
                        return GL_FRAMEBUFFER_UNSUPPORTED;
                    }
                }
            }
            else
            {
                width = colorbuffer->getWidth();
                height = colorbuffer->getHeight();
                samples = colorbuffer->getSamples();
                colorbufferSize = formatInfo.pixelBytes;
                missingAttachment = false;
            }
        }
    }

    if (mDepthbuffer)
    {
        if (mDepthbuffer->getWidth() == 0 || mDepthbuffer->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        GLenum internalformat = mDepthbuffer->getInternalFormat();
        const TextureCaps &formatCaps = data.textureCaps->get(internalformat);
        const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
        if (mDepthbuffer->type() == GL_TEXTURE)
        {
            // depth texture attachments require OES/ANGLE_depth_texture
            if (!data.extensions->depthTextures)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (!formatCaps.renderable)
            {
                return GL_FRAMEBUFFER_UNSUPPORTED;
            }

            if (formatInfo.depthBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else if (mDepthbuffer->type() == GL_RENDERBUFFER)
        {
            if (!formatCaps.renderable || formatInfo.depthBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }

        if (missingAttachment)
        {
            width = mDepthbuffer->getWidth();
            height = mDepthbuffer->getHeight();
            samples = mDepthbuffer->getSamples();
            missingAttachment = false;
        }
        else if (width != mDepthbuffer->getWidth() || height != mDepthbuffer->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != mDepthbuffer->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    if (mStencilbuffer)
    {
        if (mStencilbuffer->getWidth() == 0 || mStencilbuffer->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        GLenum internalformat = mStencilbuffer->getInternalFormat();
        const TextureCaps &formatCaps = data.textureCaps->get(internalformat);
        const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
        if (mStencilbuffer->type() == GL_TEXTURE)
        {
            // texture stencil attachments come along as part
            // of OES_packed_depth_stencil + OES/ANGLE_depth_texture
            if (!data.extensions->depthTextures)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            if (!formatCaps.renderable)
            {
                return GL_FRAMEBUFFER_UNSUPPORTED;
            }

            if (formatInfo.stencilBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }
        else if (mStencilbuffer->type() == GL_RENDERBUFFER)
        {
            if (!formatCaps.renderable || formatInfo.stencilBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }

        if (missingAttachment)
        {
            width = mStencilbuffer->getWidth();
            height = mStencilbuffer->getHeight();
            samples = mStencilbuffer->getSamples();
            missingAttachment = false;
        }
        else if (width != mStencilbuffer->getWidth() || height != mStencilbuffer->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != mStencilbuffer->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    // if we have both a depth and stencil buffer, they must refer to the same object
    // since we only support packed_depth_stencil and not separate depth and stencil
    if (mDepthbuffer && mStencilbuffer && !hasValidDepthStencil())
    {
        return GL_FRAMEBUFFER_UNSUPPORTED;
    }

    // we need to have at least one attachment to be complete
    if (missingAttachment)
    {
        return GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT;
    }

    return mImpl->checkStatus();
}

Error Framebuffer::invalidate(size_t count, const GLenum *attachments)
{
    return mImpl->invalidate(count, attachments);
}

Error Framebuffer::invalidateSub(size_t count, const GLenum *attachments, const gl::Rectangle &area)
{
    return mImpl->invalidateSub(count, attachments, area);
}

Error Framebuffer::clear(const State &state, GLbitfield mask)
{
    return mImpl->clear(state, mask);
}

Error Framebuffer::clearBufferfv(const State &state, GLenum buffer, GLint drawbuffer, const GLfloat *values)
{
    return mImpl->clearBufferfv(state, buffer, drawbuffer, values);
}

Error Framebuffer::clearBufferuiv(const State &state, GLenum buffer, GLint drawbuffer, const GLuint *values)
{
    return mImpl->clearBufferuiv(state, buffer, drawbuffer, values);
}

Error Framebuffer::clearBufferiv(const State &state, GLenum buffer, GLint drawbuffer, const GLint *values)
{
    return mImpl->clearBufferiv(state, buffer, drawbuffer, values);
}

Error Framebuffer::clearBufferfi(const State &state, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    return mImpl->clearBufferfi(state, buffer, drawbuffer, depth, stencil);
}

GLenum Framebuffer::getImplementationColorReadFormat() const
{
    return mImpl->getImplementationColorReadFormat();
}

GLenum Framebuffer::getImplementationColorReadType() const
{
    return mImpl->getImplementationColorReadType();
}

Error Framebuffer::readPixels(const gl::State &state, const gl::Rectangle &area, GLenum format, GLenum type, GLvoid *pixels) const
{
    return mImpl->readPixels(state, area, format, type, pixels);
}

Error Framebuffer::blit(const gl::State &state, const gl::Rectangle &sourceArea, const gl::Rectangle &destArea,
                        GLbitfield mask, GLenum filter, const gl::Framebuffer *sourceFramebuffer)
{
    return mImpl->blit(state, sourceArea, destArea, mask, filter, sourceFramebuffer);
}

int Framebuffer::getSamples(const gl::Data &data) const
{
    if (checkStatus(data) == GL_FRAMEBUFFER_COMPLETE)
    {
        // for a complete framebuffer, all attachments must have the same sample count
        // in this case return the first nonzero sample size
        for (unsigned int colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; colorAttachment++)
        {
            if (mColorbuffers[colorAttachment])
            {
                return mColorbuffers[colorAttachment]->getSamples();
            }
        }
    }

    return 0;
}

bool Framebuffer::hasValidDepthStencil() const
{
    // A valid depth-stencil attachment has the same resource bound to both the
    // depth and stencil attachment points.
    return (mDepthbuffer && mStencilbuffer &&
            mDepthbuffer->type() == mStencilbuffer->type() &&
            mDepthbuffer->id() == mStencilbuffer->id());
}

ColorbufferInfo Framebuffer::getColorbuffersForRender(const rx::Workarounds &workarounds) const
{
    ColorbufferInfo colorbuffersForRender;

    for (size_t colorAttachment = 0; colorAttachment < IMPLEMENTATION_MAX_DRAW_BUFFERS; ++colorAttachment)
    {
        GLenum drawBufferState = mDrawBufferStates[colorAttachment];
        FramebufferAttachment *colorbuffer = mColorbuffers[colorAttachment];

        if (colorbuffer != NULL && drawBufferState != GL_NONE)
        {
            ASSERT(drawBufferState == GL_BACK || drawBufferState == (GL_COLOR_ATTACHMENT0_EXT + colorAttachment));
            colorbuffersForRender.push_back(colorbuffer);
        }
        else if (!workarounds.mrtPerfWorkaround)
        {
            colorbuffersForRender.push_back(NULL);
        }
    }

    return colorbuffersForRender;
}

void Framebuffer::setTextureAttachment(GLenum attachment, Texture *texture, const ImageIndex &imageIndex)
{
    setAttachment(attachment, new TextureAttachment(attachment, texture, imageIndex));
}

void Framebuffer::setRenderbufferAttachment(GLenum attachment, Renderbuffer *renderbuffer)
{
    setAttachment(attachment, new RenderbufferAttachment(attachment, renderbuffer));
}

void Framebuffer::setNULLAttachment(GLenum attachment)
{
    setAttachment(attachment, NULL);
}

void Framebuffer::setAttachment(GLenum attachment, FramebufferAttachment *attachmentObj)
{
    if (attachment >= GL_COLOR_ATTACHMENT0 && attachment < (GL_COLOR_ATTACHMENT0 + IMPLEMENTATION_MAX_DRAW_BUFFERS))
    {
        size_t colorAttachment = attachment - GL_COLOR_ATTACHMENT0;
        SafeDelete(mColorbuffers[colorAttachment]);
        mColorbuffers[colorAttachment] = attachmentObj;
        mImpl->setColorAttachment(colorAttachment, attachmentObj);
    }
    else if (attachment == GL_BACK)
    {
        SafeDelete(mColorbuffers[0]);
        mColorbuffers[0] = attachmentObj;
        mImpl->setColorAttachment(0, attachmentObj);
    }
    else if (attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH)
    {
        SafeDelete(mDepthbuffer);
        mDepthbuffer = attachmentObj;
        mImpl->setDepthttachment(attachmentObj);
    }
    else if (attachment == GL_STENCIL_ATTACHMENT || attachment == GL_STENCIL)
    {
        SafeDelete(mStencilbuffer);
        mStencilbuffer = attachmentObj;
        mImpl->setStencilAttachment(attachmentObj);
    }
    else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT || attachment == GL_DEPTH_STENCIL)
    {
        SafeDelete(mDepthbuffer);
        SafeDelete(mStencilbuffer);

        // ensure this is a legitimate depth+stencil format
        if (attachmentObj && attachmentObj->getDepthSize() > 0 && attachmentObj->getStencilSize() > 0)
        {
            mDepthbuffer = attachmentObj;
            mImpl->setDepthttachment(attachmentObj);

            // Make a new attachment object to ensure we do not double-delete
            // See angle issue 686
            if (attachmentObj->type() == GL_TEXTURE)
            {
                mStencilbuffer = new TextureAttachment(GL_DEPTH_STENCIL_ATTACHMENT, attachmentObj->getTexture(),
                                                       *attachmentObj->getTextureImageIndex());
                mImpl->setStencilAttachment(mStencilbuffer);
            }
            else if (attachmentObj->type() == GL_RENDERBUFFER)
            {
                mStencilbuffer = new RenderbufferAttachment(GL_DEPTH_STENCIL_ATTACHMENT, attachmentObj->getRenderbuffer());
                mImpl->setStencilAttachment(mStencilbuffer);
            }
            else
            {
                UNREACHABLE();
            }
        }
    }
    else
    {
        UNREACHABLE();
    }
}

DefaultFramebuffer::DefaultFramebuffer(rx::FramebufferImpl *impl, rx::DefaultAttachmentImpl *colorAttachment,
                                       rx::DefaultAttachmentImpl *depthAttachment, rx::DefaultAttachmentImpl *stencilAttachment)
    : Framebuffer(impl, 0)
{
    ASSERT(colorAttachment);
    setAttachment(GL_BACK, new DefaultAttachment(GL_BACK, colorAttachment));

    if (depthAttachment)
    {
        setAttachment(GL_DEPTH, new DefaultAttachment(GL_DEPTH, depthAttachment));
    }
    if (stencilAttachment)
    {
        setAttachment(GL_STENCIL, new DefaultAttachment(GL_STENCIL, stencilAttachment));
    }

    GLenum drawBufferState = GL_BACK;
    setDrawBuffers(1, &drawBufferState);

    setReadBuffer(GL_BACK);
}

}
