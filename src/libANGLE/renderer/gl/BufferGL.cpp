//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferGL.cpp: Implements the class methods for BufferGL.

#include "libANGLE/renderer/gl/BufferGL.h"

#include "common/debug.h"

namespace rx
{

BufferGL::BufferGL()
    : BufferImpl()
{}

BufferGL::~BufferGL()
{}

gl::Error BufferGL::setData(const void* data, size_t size, GLenum usage)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error BufferGL::setSubData(const void* data, size_t size, size_t offset)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error BufferGL::copySubData(BufferImpl* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error BufferGL::map(size_t offset, size_t length, GLbitfield access, GLvoid **mapPtr)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error BufferGL::unmap()
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

void BufferGL::markTransformFeedbackUsage()
{
    UNIMPLEMENTED();
}

gl::Error BufferGL::getData(const uint8_t **outData)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

}
