//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferGL.cpp: Implements the class methods for BufferGL.

#include "libANGLE/renderer/gl/BufferGL.h"

#include "common/debug.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/StateManagerGL.h"

namespace rx
{

static const GLenum SourceBufferOperationTarget = GL_ELEMENT_ARRAY_BUFFER;
static const GLenum DestBufferOperationTarget = GL_ARRAY_BUFFER;

BufferGL::BufferGL(const FunctionsGL *functions, StateManagerGL *stateManager)
    : BufferImpl(),
      mFunctions(functions),
      mStateManager(stateManager),
      mBufferID(0)
{
    ASSERT(mFunctions);
    ASSERT(mStateManager);

    mFunctions->genBuffers(1, &mBufferID);
}

BufferGL::~BufferGL()
{
    if (mBufferID)
    {
        mFunctions->deleteBuffers(1, &mBufferID);
        mBufferID = 0;
    }
}

gl::Error BufferGL::setData(const void* data, size_t size, GLenum usage)
{
    mStateManager->bindBuffer(DestBufferOperationTarget, mBufferID);
    mFunctions->bufferData(DestBufferOperationTarget, size, data, usage);
    return gl::Error(GL_NO_ERROR);
}

gl::Error BufferGL::setSubData(const void* data, size_t size, size_t offset)
{
    mStateManager->bindBuffer(DestBufferOperationTarget, mBufferID);
    mFunctions->bufferSubData(DestBufferOperationTarget, offset, size, data);
    return gl::Error(GL_NO_ERROR);
}

gl::Error BufferGL::copySubData(BufferImpl* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size)
{
    BufferGL *sourceGL = GetAs<BufferGL>(source);

    mStateManager->bindBuffer(DestBufferOperationTarget, mBufferID);
    mStateManager->bindBuffer(SourceBufferOperationTarget, sourceGL->getBufferID());

    mFunctions->copyBufferSubData(SourceBufferOperationTarget, GL_ARRAY_BUFFER, sourceOffset, destOffset, size);

    return gl::Error(GL_NO_ERROR);
}

gl::Error BufferGL::map(size_t offset, size_t length, GLbitfield access, GLvoid **mapPtr)
{
    // TODO: look into splitting this into two functions, glMapBuffer is available in 1.5, but
    // glMapBufferRange requires 3.0

    mStateManager->bindBuffer(DestBufferOperationTarget, mBufferID);
    *mapPtr = mFunctions->mapBufferRange(DestBufferOperationTarget, offset, length, access);
    return gl::Error(GL_NO_ERROR);
}

gl::Error BufferGL::unmap()
{
    mStateManager->bindBuffer(DestBufferOperationTarget, mBufferID);
    mFunctions->unmapBuffer(DestBufferOperationTarget);
    return gl::Error(GL_NO_ERROR);
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

GLuint BufferGL::getBufferID() const
{
    return mBufferID;
}

}
