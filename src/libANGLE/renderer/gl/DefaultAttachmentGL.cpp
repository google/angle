//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DefaultAttachmentGL.cpp: Implements the class methods for DefaultAttachmentGL.

#include "libANGLE/renderer/gl/DefaultAttachmentGL.h"

#include "common/debug.h"
#include "libANGLE/Config.h"
#include "libANGLE/renderer/gl/SurfaceGL.h"

namespace rx
{

DefaultAttachmentGL::DefaultAttachmentGL(GLenum type, SurfaceGL *surface)
    : DefaultAttachmentImpl(),
      mType(type),
      mSurface(surface)
{
    ASSERT(mSurface != nullptr);
}

DefaultAttachmentGL::~DefaultAttachmentGL()
{
}

GLsizei DefaultAttachmentGL::getWidth() const
{
    return mSurface->getWidth();
}

GLsizei DefaultAttachmentGL::getHeight() const
{
    return mSurface->getHeight();
}

GLenum DefaultAttachmentGL::getInternalFormat() const
{
    const egl::Config *config = mSurface->getConfig();
    return (mType == GL_COLOR ? config->renderTargetFormat : config->depthStencilFormat);
}

GLsizei DefaultAttachmentGL::getSamples() const
{
    const egl::Config *config = mSurface->getConfig();
    return config->samples;
}

}
