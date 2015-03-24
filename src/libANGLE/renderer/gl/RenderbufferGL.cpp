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

gl::Error RenderbufferGL::setStorage(GLenum internalformat, size_t width, size_t height)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error RenderbufferGL::setStorageMultisample(size_t samples, GLenum internalformat, size_t width, size_t height)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

}
