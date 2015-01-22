//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DefaultAttachmentGL.cpp: Implements the class methods for DefaultAttachmentGL.

#include "libANGLE/renderer/gl/DefaultAttachmentGL.h"

#include "common/debug.h"

namespace rx
{

DefaultAttachmentGL::DefaultAttachmentGL()
    : DefaultAttachmentImpl()
{}

DefaultAttachmentGL::~DefaultAttachmentGL()
{}

GLsizei DefaultAttachmentGL::getWidth() const
{
    UNIMPLEMENTED();
    return GLsizei();
}

GLsizei DefaultAttachmentGL::getHeight() const
{
    UNIMPLEMENTED();
    return GLsizei();
}

GLenum DefaultAttachmentGL::getInternalFormat() const
{
    UNIMPLEMENTED();
    return GLenum();
}

GLsizei DefaultAttachmentGL::getSamples() const
{
    UNIMPLEMENTED();
    return GLsizei();
}

}
