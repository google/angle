//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer.cpp: Implements the gl::Framebuffer class. Implements GL framebuffer
// objects and related functionality. [OpenGL ES 2.0.24] section 4.4 page 105.

#include "libANGLE/Framebuffer.h"

#include "common/utilities.h"
#include "libANGLE/Config.h"
#include "libANGLE/Context.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/Surface.h"
#include "libANGLE/Texture.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/FramebufferImpl.h"
#include "libANGLE/renderer/ImplFactory.h"
#include "libANGLE/renderer/RenderbufferImpl.h"
#include "libANGLE/renderer/Workarounds.h"

namespace gl
{

namespace
{
void DeleteMatchingAttachment(FramebufferAttachment *&attachment, GLenum matchType, GLuint matchId)
{
    if (attachment && attachment->type() == matchType && attachment->id() == matchId)
    {
        SafeDelete(attachment);
    }
}
}

Framebuffer::Data::Data(const Caps &caps)
    : mColorAttachments(caps.maxColorAttachments, nullptr),
      mDepthAttachment(nullptr),
      mStencilAttachment(nullptr),
      mDrawBufferStates(caps.maxDrawBuffers, GL_NONE),
      mReadBufferState(GL_COLOR_ATTACHMENT0_EXT)
{
    mDrawBufferStates[0] = GL_COLOR_ATTACHMENT0_EXT;
}

Framebuffer::Data::~Data()
{
    for (auto &colorAttachment : mColorAttachments)
    {
        SafeDelete(colorAttachment);
    }
    SafeDelete(mDepthAttachment);
    SafeDelete(mStencilAttachment);
}

const FramebufferAttachment *Framebuffer::Data::getReadAttachment() const
{
    ASSERT(mReadBufferState == GL_BACK || (mReadBufferState >= GL_COLOR_ATTACHMENT0 && mReadBufferState <= GL_COLOR_ATTACHMENT15));
    size_t readIndex = (mReadBufferState == GL_BACK ? 0 : static_cast<size_t>(mReadBufferState - GL_COLOR_ATTACHMENT0));
    ASSERT(readIndex < mColorAttachments.size());
    return mColorAttachments[readIndex];
}

const FramebufferAttachment *Framebuffer::Data::getFirstColorAttachment() const
{
    for (FramebufferAttachment *colorAttachment : mColorAttachments)
    {
        if (colorAttachment != nullptr)
        {
            return colorAttachment;
        }
    }

    return nullptr;
}

const FramebufferAttachment *Framebuffer::Data::getDepthOrStencilAttachment() const
{
    return (mDepthAttachment != nullptr ? mDepthAttachment : mStencilAttachment);
}

FramebufferAttachment *Framebuffer::Data::getColorAttachment(unsigned int colorAttachment)
{
    ASSERT(colorAttachment < mColorAttachments.size());
    return mColorAttachments[colorAttachment];
}

const FramebufferAttachment *Framebuffer::Data::getColorAttachment(unsigned int colorAttachment) const
{
    return const_cast<Framebuffer::Data *>(this)->getColorAttachment(colorAttachment);
}

FramebufferAttachment *Framebuffer::Data::getDepthAttachment()
{
    return mDepthAttachment;
}

const FramebufferAttachment *Framebuffer::Data::getDepthAttachment() const
{
    return const_cast<Framebuffer::Data *>(this)->getDepthAttachment();
}

FramebufferAttachment *Framebuffer::Data::getStencilAttachment()
{
    return mStencilAttachment;
}

const FramebufferAttachment *Framebuffer::Data::getStencilAttachment() const
{
    return const_cast<Framebuffer::Data *>(this)->getStencilAttachment();
}

FramebufferAttachment *Framebuffer::Data::getDepthStencilAttachment()
{
    // A valid depth-stencil attachment has the same resource bound to both the
    // depth and stencil attachment points.
    if (mDepthAttachment && mStencilAttachment &&
        mDepthAttachment->type() == mStencilAttachment->type() &&
        mDepthAttachment->id() == mStencilAttachment->id())
    {
        return mDepthAttachment;
    }

    return nullptr;
}

const FramebufferAttachment *Framebuffer::Data::getDepthStencilAttachment() const
{
    return const_cast<Framebuffer::Data *>(this)->getDepthStencilAttachment();
}

Framebuffer::Framebuffer(const Caps &caps, rx::ImplFactory *factory, GLuint id)
    : mData(caps),
      mImpl(nullptr),
      mId(id)
{
    if (mId == 0)
    {
        mImpl = factory->createDefaultFramebuffer(mData);
    }
    else
    {
        mImpl = factory->createFramebuffer(mData);
    }
    ASSERT(mImpl != nullptr);
}

Framebuffer::~Framebuffer()
{
    SafeDelete(mImpl);
}

void Framebuffer::detachTexture(GLuint textureId)
{
    detachResourceById(GL_TEXTURE, textureId);
}

void Framebuffer::detachRenderbuffer(GLuint renderbufferId)
{
    detachResourceById(GL_RENDERBUFFER, renderbufferId);
}

void Framebuffer::detachResourceById(GLenum resourceType, GLuint resourceId)
{
    for (auto &colorAttachment : mData.mColorAttachments)
    {
        DeleteMatchingAttachment(colorAttachment, resourceType, resourceId);
    }

    DeleteMatchingAttachment(mData.mDepthAttachment, resourceType, resourceId);
    DeleteMatchingAttachment(mData.mStencilAttachment, resourceType, resourceId);
}

FramebufferAttachment *Framebuffer::getColorbuffer(unsigned int colorAttachment)
{
    return mData.getColorAttachment(colorAttachment);
}

const FramebufferAttachment *Framebuffer::getColorbuffer(unsigned int colorAttachment) const
{
    return mData.getColorAttachment(colorAttachment);
}

FramebufferAttachment *Framebuffer::getDepthbuffer()
{
    return mData.getDepthAttachment();
}

const FramebufferAttachment *Framebuffer::getDepthbuffer() const
{
    return mData.getDepthAttachment();
}

FramebufferAttachment *Framebuffer::getStencilbuffer()
{
    return mData.getStencilAttachment();
}

const FramebufferAttachment *Framebuffer::getStencilbuffer() const
{
    return mData.getStencilAttachment();
}

FramebufferAttachment *Framebuffer::getDepthStencilBuffer()
{
    return mData.getDepthStencilAttachment();
}

const FramebufferAttachment *Framebuffer::getDepthStencilBuffer() const
{
    return mData.getDepthStencilAttachment();
}

const FramebufferAttachment *Framebuffer::getDepthOrStencilbuffer() const
{
    return mData.getDepthOrStencilAttachment();
}

const FramebufferAttachment *Framebuffer::getReadColorbuffer() const
{
    return mData.getReadAttachment();
}

GLenum Framebuffer::getReadColorbufferType() const
{
    const FramebufferAttachment *readAttachment = mData.getReadAttachment();
    return (readAttachment != nullptr ? readAttachment->type() : GL_NONE);
}

const FramebufferAttachment *Framebuffer::getFirstColorbuffer() const
{
    return mData.getFirstColorAttachment();
}

FramebufferAttachment *Framebuffer::getAttachment(GLenum attachment)
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
            return nullptr;
        }
    }
}

const FramebufferAttachment *Framebuffer::getAttachment(GLenum attachment) const
{
    return const_cast<Framebuffer*>(this)->getAttachment(attachment);
}

GLenum Framebuffer::getDrawBufferState(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < mData.mDrawBufferStates.size());
    return mData.mDrawBufferStates[colorAttachment];
}

void Framebuffer::setDrawBuffers(size_t count, const GLenum *buffers)
{
    auto &drawStates = mData.mDrawBufferStates;

    ASSERT(count <= drawStates.size());
    std::copy(buffers, buffers + count, drawStates.begin());
    std::fill(drawStates.begin() + count, drawStates.end(), GL_NONE);
    mImpl->setDrawBuffers(count, buffers);
}

GLenum Framebuffer::getReadBufferState() const
{
    return mData.mReadBufferState;
}

void Framebuffer::setReadBuffer(GLenum buffer)
{
    ASSERT(buffer == GL_BACK || buffer == GL_NONE ||
           (buffer >= GL_COLOR_ATTACHMENT0 &&
            (buffer - GL_COLOR_ATTACHMENT0) < mData.mColorAttachments.size()));
    mData.mReadBufferState = buffer;
    mImpl->setReadBuffer(buffer);
}

bool Framebuffer::isEnabledColorAttachment(unsigned int colorAttachment) const
{
    ASSERT(colorAttachment < mData.mColorAttachments.size());
    return (mData.mColorAttachments[colorAttachment] &&
            mData.mDrawBufferStates[colorAttachment] != GL_NONE);
}

bool Framebuffer::hasEnabledColorAttachment() const
{
    for (size_t colorAttachment = 0; colorAttachment < mData.mColorAttachments.size(); ++colorAttachment)
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
    return (mData.mStencilAttachment && mData.mStencilAttachment->getStencilSize() > 0);
}

bool Framebuffer::usingExtendedDrawBuffers() const
{
    for (size_t colorAttachment = 1; colorAttachment < mData.mColorAttachments.size(); ++colorAttachment)
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

    for (const FramebufferAttachment *colorAttachment : mData.mColorAttachments)
    {
        if (colorAttachment != nullptr)
        {
            if (colorAttachment->getWidth() == 0 || colorAttachment->getHeight() == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }

            GLenum internalformat = colorAttachment->getInternalFormat();
            const TextureCaps &formatCaps = data.textureCaps->get(internalformat);
            const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
            if (colorAttachment->type() == GL_TEXTURE)
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
            else if (colorAttachment->type() == GL_RENDERBUFFER)
            {
                if (!formatCaps.renderable || formatInfo.depthBits > 0 || formatInfo.stencilBits > 0)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
                }
            }

            if (!missingAttachment)
            {
                // all color attachments must have the same width and height
                if (colorAttachment->getWidth() != width || colorAttachment->getHeight() != height)
                {
                    return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
                }

                // APPLE_framebuffer_multisample, which EXT_draw_buffers refers to, requires that
                // all color attachments have the same number of samples for the FBO to be complete.
                if (colorAttachment->getSamples() != samples)
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
                width = colorAttachment->getWidth();
                height = colorAttachment->getHeight();
                samples = colorAttachment->getSamples();
                colorbufferSize = formatInfo.pixelBytes;
                missingAttachment = false;
            }
        }
    }

    const FramebufferAttachment *depthAttachment = mData.mDepthAttachment;
    if (depthAttachment != nullptr)
    {
        if (depthAttachment->getWidth() == 0 || depthAttachment->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        GLenum internalformat = depthAttachment->getInternalFormat();
        const TextureCaps &formatCaps = data.textureCaps->get(internalformat);
        const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
        if (depthAttachment->type() == GL_TEXTURE)
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
        else if (depthAttachment->type() == GL_RENDERBUFFER)
        {
            if (!formatCaps.renderable || formatInfo.depthBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }

        if (missingAttachment)
        {
            width = depthAttachment->getWidth();
            height = depthAttachment->getHeight();
            samples = depthAttachment->getSamples();
            missingAttachment = false;
        }
        else if (width != depthAttachment->getWidth() || height != depthAttachment->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != depthAttachment->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    const FramebufferAttachment *stencilAttachment = mData.mStencilAttachment;
    if (stencilAttachment)
    {
        if (stencilAttachment->getWidth() == 0 || stencilAttachment->getHeight() == 0)
        {
            return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
        }

        GLenum internalformat = stencilAttachment->getInternalFormat();
        const TextureCaps &formatCaps = data.textureCaps->get(internalformat);
        const InternalFormat &formatInfo = GetInternalFormatInfo(internalformat);
        if (stencilAttachment->type() == GL_TEXTURE)
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
        else if (stencilAttachment->type() == GL_RENDERBUFFER)
        {
            if (!formatCaps.renderable || formatInfo.stencilBits == 0)
            {
                return GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT;
            }
        }

        if (missingAttachment)
        {
            width = stencilAttachment->getWidth();
            height = stencilAttachment->getHeight();
            samples = stencilAttachment->getSamples();
            missingAttachment = false;
        }
        else if (width != stencilAttachment->getWidth() || height != stencilAttachment->getHeight())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS;
        }
        else if (samples != stencilAttachment->getSamples())
        {
            return GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_ANGLE;
        }
    }

    // if we have both a depth and stencil buffer, they must refer to the same object
    // since we only support packed_depth_stencil and not separate depth and stencil
    if (depthAttachment && stencilAttachment && !hasValidDepthStencil())
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

Error Framebuffer::clear(const gl::Data &data, GLbitfield mask)
{
    return mImpl->clear(data, mask);
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
        for (const FramebufferAttachment *colorAttachment : mData.mColorAttachments)
        {
            if (colorAttachment != nullptr)
            {
                return colorAttachment->getSamples();
            }
        }
    }

    return 0;
}

bool Framebuffer::hasValidDepthStencil() const
{
    return mData.getDepthStencilAttachment() != nullptr;
}

void Framebuffer::setTextureAttachment(GLenum attachment, Texture *texture, const ImageIndex &imageIndex)
{
    setAttachment(attachment, new FramebufferAttachment(GL_TEXTURE, attachment, imageIndex, texture));
}

void Framebuffer::setRenderbufferAttachment(GLenum attachment, Renderbuffer *renderbuffer)
{
    setAttachment(attachment, new FramebufferAttachment(GL_RENDERBUFFER, attachment, ImageIndex::MakeInvalid(), renderbuffer));
}

void Framebuffer::setNULLAttachment(GLenum attachment)
{
    setAttachment(attachment, NULL);
}

void Framebuffer::setAttachment(GLenum attachment, FramebufferAttachment *attachmentObj)
{
    if (attachment >= GL_COLOR_ATTACHMENT0 && attachment < (GL_COLOR_ATTACHMENT0 + mData.mColorAttachments.size()))
    {
        size_t colorAttachment = attachment - GL_COLOR_ATTACHMENT0;
        SafeDelete(mData.mColorAttachments[colorAttachment]);
        mData.mColorAttachments[colorAttachment] = attachmentObj;
        mImpl->setColorAttachment(colorAttachment, attachmentObj);
    }
    else if (attachment == GL_BACK)
    {
        SafeDelete(mData.mColorAttachments[0]);
        mData.mColorAttachments[0] = attachmentObj;
        mImpl->setColorAttachment(0, attachmentObj);
    }
    else if (attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH)
    {
        SafeDelete(mData.mDepthAttachment);
        mData.mDepthAttachment = attachmentObj;
        mImpl->setDepthAttachment(attachmentObj);
    }
    else if (attachment == GL_STENCIL_ATTACHMENT || attachment == GL_STENCIL)
    {
        SafeDelete(mData.mStencilAttachment);
        mData.mStencilAttachment = attachmentObj;
        mImpl->setStencilAttachment(attachmentObj);
    }
    else if (attachment == GL_DEPTH_STENCIL_ATTACHMENT || attachment == GL_DEPTH_STENCIL)
    {
        SafeDelete(mData.mDepthAttachment);
        SafeDelete(mData.mStencilAttachment);

        // ensure this is a legitimate depth+stencil format
        if (attachmentObj && attachmentObj->getDepthSize() > 0 && attachmentObj->getStencilSize() > 0)
        {
            mData.mDepthAttachment = attachmentObj;
            mImpl->setDepthAttachment(attachmentObj);

            // Make a new attachment object to ensure we do not double-delete
            // See angle issue 686
            if (attachmentObj->type() == GL_TEXTURE)
            {
                mData.mStencilAttachment = new FramebufferAttachment(GL_TEXTURE,
                                                                     GL_DEPTH_STENCIL_ATTACHMENT,
                                                                     attachmentObj->getTextureImageIndex(),
                                                                     attachmentObj->getTexture());
                mImpl->setStencilAttachment(mData.mStencilAttachment);
            }
            else if (attachmentObj->type() == GL_RENDERBUFFER)
            {
                mData.mStencilAttachment = new FramebufferAttachment(GL_RENDERBUFFER,
                                                                     GL_DEPTH_STENCIL_ATTACHMENT,
                                                                     ImageIndex::MakeInvalid(),
                                                                     attachmentObj->getRenderbuffer());
                mImpl->setStencilAttachment(mData.mStencilAttachment);
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

DefaultFramebuffer::DefaultFramebuffer(const Caps &caps, rx::ImplFactory *factory, egl::Surface *surface)
    : Framebuffer(caps, factory, 0)
{
    const egl::Config *config = surface->getConfig();

    setAttachment(GL_BACK, new FramebufferAttachment(GL_FRAMEBUFFER_DEFAULT, GL_BACK, ImageIndex::MakeInvalid(), surface));

    if (config->depthSize > 0)
    {
        setAttachment(GL_DEPTH, new FramebufferAttachment(GL_FRAMEBUFFER_DEFAULT, GL_DEPTH, ImageIndex::MakeInvalid(), surface));
    }
    if (config->stencilSize > 0)
    {
        setAttachment(GL_STENCIL, new FramebufferAttachment(GL_FRAMEBUFFER_DEFAULT, GL_STENCIL, ImageIndex::MakeInvalid(), surface));
    }

    GLenum drawBufferState = GL_BACK;
    setDrawBuffers(1, &drawBufferState);

    setReadBuffer(GL_BACK);
}

}
