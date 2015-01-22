//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FenceSyncGL.cpp: Implements the class methods for FenceSyncGL.

#include "libANGLE/renderer/gl/FenceSyncGL.h"

#include "common/debug.h"

namespace rx
{

FenceSyncGL::FenceSyncGL()
    : FenceSyncImpl()
{}

FenceSyncGL::~FenceSyncGL()
{}

gl::Error FenceSyncGL::set()
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FenceSyncGL::clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FenceSyncGL::serverWait(GLbitfield flags, GLuint64 timeout)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FenceSyncGL::getStatus(GLint *outResult)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

}
