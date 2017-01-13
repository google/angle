//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BufferNULL.cpp:
//    Implements the class methods for BufferNULL.
//

#include "libANGLE/renderer/null/BufferNULL.h"

#include "common/debug.h"
#include "common/utilities.h"
#include "libANGLE/angletypes.h"

namespace rx
{

BufferNULL::BufferNULL(const gl::BufferState &state) : BufferImpl(state)
{
}

BufferNULL::~BufferNULL()
{
}

gl::Error BufferNULL::setData(ContextImpl *context,
                              GLenum target,
                              const void *data,
                              size_t size,
                              GLenum usage)
{
    mData.resize(size, 0);
    if (size > 0 && data != nullptr)
    {
        memcpy(mData.data(), data, size);
    }
    return gl::NoError();
}

gl::Error BufferNULL::setSubData(ContextImpl *context,
                                 GLenum target,
                                 const void *data,
                                 size_t size,
                                 size_t offset)
{
    if (size > 0)
    {
        memcpy(mData.data() + offset, data, size);
    }
    return gl::NoError();
}

gl::Error BufferNULL::copySubData(ContextImpl *context,
                                  BufferImpl *source,
                                  GLintptr sourceOffset,
                                  GLintptr destOffset,
                                  GLsizeiptr size)
{
    BufferNULL *sourceNULL = GetAs<BufferNULL>(source);
    if (size > 0)
    {
        memcpy(mData.data() + destOffset, sourceNULL->mData.data() + sourceOffset, size);
    }
    return gl::NoError();
}

gl::Error BufferNULL::map(ContextImpl *context, GLenum access, GLvoid **mapPtr)
{
    *mapPtr = mData.data();
    return gl::NoError();
}

gl::Error BufferNULL::mapRange(ContextImpl *context,
                               size_t offset,
                               size_t length,
                               GLbitfield access,
                               GLvoid **mapPtr)
{
    *mapPtr = mData.data() + offset;
    return gl::NoError();
}

gl::Error BufferNULL::unmap(ContextImpl *context, GLboolean *result)
{
    *result = GL_TRUE;
    return gl::NoError();
}

gl::Error BufferNULL::getIndexRange(GLenum type,
                                    size_t offset,
                                    size_t count,
                                    bool primitiveRestartEnabled,
                                    gl::IndexRange *outRange)
{
    *outRange = gl::ComputeIndexRange(type, mData.data() + offset, count, primitiveRestartEnabled);
    return gl::NoError();
}

}  // namespace rx
