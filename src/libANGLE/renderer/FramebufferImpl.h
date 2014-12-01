//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferImpl.h: Defines the abstract rx::DefaultAttachmentImpl class.

#ifndef LIBANGLE_RENDERER_FRAMBUFFERIMPL_H_
#define LIBANGLE_RENDERER_FRAMBUFFERIMPL_H_

#include "libANGLE/Error.h"

#include "angle_gl.h"

namespace gl
{
struct Rectangle;
}

namespace rx
{

class DefaultAttachmentImpl
{
  public:
    virtual ~DefaultAttachmentImpl() {};

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLenum getActualFormat() const = 0;
    virtual GLsizei getSamples() const = 0;
};

class FramebufferImpl
{
  public:
    virtual ~FramebufferImpl() {};

    virtual gl::Error invalidate(size_t count, const GLenum *attachments) = 0;
    virtual gl::Error invalidateSub(size_t count, const GLenum *attachments, const gl::Rectangle &area) = 0;
};

}

#endif // LIBANGLE_RENDERER_FRAMBUFFERIMPL_H_
