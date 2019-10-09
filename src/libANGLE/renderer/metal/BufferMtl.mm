//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BufferMtl.mm:
//    Implements the class methods for BufferMtl.
//

#include "libANGLE/renderer/metal/BufferMtl.h"

namespace rx
{

// BufferMtl implementation
BufferMtl::BufferMtl(const gl::BufferState &state) : BufferImpl(state)
{
    UNIMPLEMENTED();
}

BufferMtl::~BufferMtl() {}

void BufferMtl::destroy(const gl::Context *context)
{
    UNIMPLEMENTED();
}

angle::Result BufferMtl::setData(const gl::Context *context,
                                 gl::BufferBinding target,
                                 const void *data,
                                 size_t size,
                                 gl::BufferUsage usage)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result BufferMtl::setSubData(const gl::Context *context,
                                    gl::BufferBinding target,
                                    const void *data,
                                    size_t size,
                                    size_t offset)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result BufferMtl::copySubData(const gl::Context *context,
                                     BufferImpl *source,
                                     GLintptr sourceOffset,
                                     GLintptr destOffset,
                                     GLsizeiptr size)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result BufferMtl::map(const gl::Context *context, GLenum access, void **mapPtr)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result BufferMtl::mapRange(const gl::Context *context,
                                  size_t offset,
                                  size_t length,
                                  GLbitfield access,
                                  void **mapPtr)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result BufferMtl::unmap(const gl::Context *context, GLboolean *result)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result BufferMtl::getIndexRange(const gl::Context *context,
                                       gl::DrawElementsType type,
                                       size_t offset,
                                       size_t count,
                                       bool primitiveRestartEnabled,
                                       gl::IndexRange *outRange)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

}  // namespace rx
