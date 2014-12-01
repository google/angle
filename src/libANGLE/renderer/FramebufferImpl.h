//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FramebufferImpl.h: Defines the abstract rx::DefaultAttachmentImpl class.

#ifndef LIBANGLE_RENDERER_FRAMBUFFERIMPL_H_
#define LIBANGLE_RENDERER_FRAMBUFFERIMPL_H_

#include "angle_gl.h"

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
};

}

#endif // LIBANGLE_RENDERER_FRAMBUFFERIMPL_H_
