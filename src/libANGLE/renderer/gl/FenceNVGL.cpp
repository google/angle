//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FenceNVGL.cpp: Implements the class methods for FenceNVGL.

#include "libANGLE/renderer/gl/FenceNVGL.h"

#include "common/debug.h"

namespace rx
{

FenceNVGL::FenceNVGL()
    : FenceNVImpl()
{}

FenceNVGL::~FenceNVGL()
{}

gl::Error FenceNVGL::set()
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FenceNVGL::test(bool flushCommandBuffer, GLboolean *outFinished)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error FenceNVGL::finishFence(GLboolean *outFinished)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

}
