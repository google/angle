//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DefaultAttachmentGL.h: Defines the class interface for DefaultAttachmentGL.

#ifndef LIBANGLE_RENDERER_GL_DEFAULTATTACHMENTGL_H_
#define LIBANGLE_RENDERER_GL_DEFAULTATTACHMENTGL_H_

#include "libANGLE/renderer/DefaultAttachmentImpl.h"

namespace rx
{

class SurfaceGL;

class DefaultAttachmentGL : public DefaultAttachmentImpl
{
  public:
    DefaultAttachmentGL(GLenum type, SurfaceGL *surface);
    ~DefaultAttachmentGL() override;

    GLsizei getWidth() const override;
    GLsizei getHeight() const override;
    GLenum getInternalFormat() const override;
    GLsizei getSamples() const override;

  private:
    DISALLOW_COPY_AND_ASSIGN(DefaultAttachmentGL);

    GLenum mType;
    SurfaceGL *mSurface;
};

}

#endif // LIBANGLE_RENDERER_GL_DEFAULTATTACHMENTGL_H_
