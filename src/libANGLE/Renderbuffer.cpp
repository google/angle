//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderbuffer.cpp: Implements the renderer-agnostic gl::Renderbuffer class,
// GL renderbuffer objects and related functionality.
// [OpenGL ES 2.0.24] section 4.4.3 page 108.

#include "libANGLE/Renderbuffer.h"

#include "common/utilities.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/Texture.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/RenderTarget.h"
#include "libANGLE/renderer/RenderbufferImpl.h"

namespace gl
{
Renderbuffer::Renderbuffer(rx::RenderbufferImpl *impl, GLuint id)
  : RefCountObject(id),
    mRenderbuffer(impl),
    mWidth(0),
    mHeight(0),
    mInternalFormat(GL_RGBA4),
    mActualFormat(GL_RGBA4),
    mSamples(0)
{
    ASSERT(mRenderbuffer);
}

Renderbuffer::~Renderbuffer()
{
    SafeDelete(mRenderbuffer);
}

Error Renderbuffer::setStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples)
{
    Error error = mRenderbuffer->setStorage(width, height, internalformat, samples);
    if (error.isError())
    {
        return error;
    }

    mWidth = width;
    mHeight = height;
    mInternalFormat = internalformat;
    mSamples = samples;
    mActualFormat = mRenderbuffer->getActualFormat();

    return Error(GL_NO_ERROR);
}

rx::RenderbufferImpl *Renderbuffer::getImplementation()
{
    ASSERT(mRenderbuffer);
    return mRenderbuffer;
}

GLsizei Renderbuffer::getWidth() const
{
    return mWidth;
}

GLsizei Renderbuffer::getHeight() const
{
    return mHeight;
}

GLenum Renderbuffer::getInternalFormat() const
{
    return mInternalFormat;
}

GLenum Renderbuffer::getActualFormat() const
{
    return mActualFormat;
}

GLsizei Renderbuffer::getSamples() const
{
    return mSamples;
}

GLuint Renderbuffer::getRedSize() const
{
    return GetInternalFormatInfo(getActualFormat()).redBits;
}

GLuint Renderbuffer::getGreenSize() const
{
    return GetInternalFormatInfo(getActualFormat()).greenBits;
}

GLuint Renderbuffer::getBlueSize() const
{
    return GetInternalFormatInfo(getActualFormat()).blueBits;
}

GLuint Renderbuffer::getAlphaSize() const
{
    return GetInternalFormatInfo(getActualFormat()).alphaBits;
}

GLuint Renderbuffer::getDepthSize() const
{
    return GetInternalFormatInfo(getActualFormat()).depthBits;
}

GLuint Renderbuffer::getStencilSize() const
{
    return GetInternalFormatInfo(getActualFormat()).stencilBits;
}

}
