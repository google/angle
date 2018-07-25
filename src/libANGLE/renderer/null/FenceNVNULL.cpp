//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FenceNVNULL.cpp:
//    Implements the class methods for FenceNVNULL.
//

#include "libANGLE/renderer/null/FenceNVNULL.h"

#include "common/debug.h"

namespace rx
{

FenceNVNULL::FenceNVNULL() : FenceNVImpl()
{
}

FenceNVNULL::~FenceNVNULL()
{
}

gl::Error FenceNVNULL::set(const gl::Context *context, GLenum condition)
{
    return gl::NoError();
}

gl::Error FenceNVNULL::test(const gl::Context *context, GLboolean *outFinished)
{
    *outFinished = GL_TRUE;
    return gl::NoError();
}

gl::Error FenceNVNULL::finish(const gl::Context *context)
{
    return gl::NoError();
}

}  // namespace rx
