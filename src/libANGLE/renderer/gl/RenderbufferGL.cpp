//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferGL.cpp: Implements the class methods for RenderbufferGL.

#include "libANGLE/renderer/gl/RenderbufferGL.h"

#include "common/debug.h"

namespace rx
{

RenderbufferGL::RenderbufferGL()
    : RenderbufferImpl()
{}

RenderbufferGL::~RenderbufferGL()
{}

gl::Error RenderbufferGL::setStorage(GLsizei width, GLsizei height, GLenum internalformat, GLsizei samples)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

}
