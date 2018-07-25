//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// QueryVk.cpp:
//    Implements the class methods for QueryVk.
//

#include "libANGLE/renderer/vulkan/QueryVk.h"

#include "common/debug.h"

namespace rx
{

QueryVk::QueryVk(gl::QueryType type) : QueryImpl(type)
{
}

QueryVk::~QueryVk()
{
}

gl::Error QueryVk::begin(const gl::Context *context)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error QueryVk::end(const gl::Context *context)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error QueryVk::queryCounter(const gl::Context *context)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLint *params)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLuint *params)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLint64 *params)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error QueryVk::getResult(const gl::Context *context, GLuint64 *params)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error QueryVk::isResultAvailable(const gl::Context *context, bool *available)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

}  // namespace rx
