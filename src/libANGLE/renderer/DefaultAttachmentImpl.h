//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DefaultAttachmentImpl.h: Defines the abstract rx::DefaultAttachmentImpl class.

#ifndef LIBANGLE_RENDERER_DEFAULTATTACHMENTIMPL_H_
#define LIBANGLE_RENDERER_DEFAULTATTACHMENTIMPL_H_

#include "angle_gl.h"
#include "common/angleutils.h"

namespace rx
{

class DefaultAttachmentImpl
{
  public:
    DefaultAttachmentImpl() {}
    virtual ~DefaultAttachmentImpl() {}

    virtual GLsizei getWidth() const = 0;
    virtual GLsizei getHeight() const = 0;
    virtual GLenum getInternalFormat() const = 0;
    virtual GLsizei getSamples() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(DefaultAttachmentImpl);
};

}

#endif // LIBANGLE_RENDERER_DEFAULTATTACHMENTIMPL_H_
