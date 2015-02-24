//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexArrayGL.cpp: Implements the class methods for VertexArrayGL.

#include "libANGLE/renderer/gl/VertexArrayGL.h"

#include "common/debug.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/gl/BufferGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/StateManagerGL.h"

namespace rx
{

VertexArrayGL::VertexArrayGL(const FunctionsGL *functions, StateManagerGL *stateManager)
    : VertexArrayImpl(),
      mFunctions(functions),
      mStateManager(stateManager),
      mVertexArrayID(0),
      mAppliedElementArrayBuffer(0),
      mAppliedAttributes()
{
    ASSERT(mFunctions);
    ASSERT(mStateManager);
    mFunctions->genVertexArrays(1, &mVertexArrayID);

    // Set the cached vertex attribute array size
    GLint maxVertexAttribs;
    mFunctions->getIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    mAppliedAttributes.resize(maxVertexAttribs);
}

VertexArrayGL::~VertexArrayGL()
{
    if (mVertexArrayID != 0)
    {
        mFunctions->deleteVertexArrays(1, &mVertexArrayID);
        mVertexArrayID = 0;
    }

    for (size_t idx = 0; idx < mAppliedAttributes.size(); idx++)
    {
        mAppliedAttributes[idx].buffer.set(NULL);
    }
}

void VertexArrayGL::setElementArrayBuffer(const gl::Buffer *buffer)
{
    GLuint elementArrayBufferID = 0;
    if (buffer != nullptr)
    {
        const BufferGL *bufferGL = GetImplAs<BufferGL>(buffer);
        elementArrayBufferID = bufferGL->getBufferID();
    }

    if (elementArrayBufferID != mAppliedElementArrayBuffer)
    {
        mStateManager->bindVertexArray(mVertexArrayID);
        mStateManager->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBufferID);
        mStateManager->bindVertexArray(0);

        mAppliedElementArrayBuffer = elementArrayBufferID;
    }
}

void VertexArrayGL::setAttribute(size_t idx, const gl::VertexAttribute &attr)
{
    if (mAppliedAttributes[idx].type != attr.type ||
        mAppliedAttributes[idx].size != attr.size ||
        mAppliedAttributes[idx].normalized != attr.normalized ||
        mAppliedAttributes[idx].pureInteger != attr.pureInteger ||
        mAppliedAttributes[idx].stride != attr.stride ||
        mAppliedAttributes[idx].pointer != attr.pointer ||
        mAppliedAttributes[idx].buffer.get() != attr.buffer.get())
    {
        mStateManager->bindVertexArray(mVertexArrayID);

        const gl::Buffer *arrayBuffer = attr.buffer.get();
        if (arrayBuffer != nullptr)
        {
            const BufferGL *arrayBufferGL = GetImplAs<BufferGL>(arrayBuffer);
            mStateManager->bindBuffer(GL_ARRAY_BUFFER, arrayBufferGL->getBufferID());
        }
        else
        {
            // This will take some extra work, core OpenGL doesn't support binding raw data pointers
            // to VAOs
            UNIMPLEMENTED();
        }

        if (attr.pureInteger)
        {
            mFunctions->vertexAttribIPointer(idx, attr.size, attr.type, attr.stride, attr.pointer);
        }
        else
        {
            mFunctions->vertexAttribPointer(idx, attr.size, attr.type, attr.normalized, attr.stride, attr.pointer);
        }
        mAppliedAttributes[idx].type = attr.type;
        mAppliedAttributes[idx].size = attr.size;
        mAppliedAttributes[idx].normalized = attr.normalized;
        mAppliedAttributes[idx].pureInteger = attr.pureInteger;
        mAppliedAttributes[idx].stride = attr.stride;
        mAppliedAttributes[idx].pointer = attr.pointer;
        mAppliedAttributes[idx].buffer.set(attr.buffer.get());

        mStateManager->bindVertexArray(0);
    }
}

void VertexArrayGL::setAttributeDivisor(size_t idx, GLuint divisor)
{
    if (mAppliedAttributes[idx].divisor != divisor)
    {
        mStateManager->bindVertexArray(mVertexArrayID);

        mFunctions->vertexAttribDivisor(idx, divisor);
        mAppliedAttributes[idx].divisor = divisor;

        mStateManager->bindVertexArray(0);
    }
}

void VertexArrayGL::enableAttribute(size_t idx, bool enabledState)
{
    if (mAppliedAttributes[idx].enabled != enabledState)
    {
        mStateManager->bindVertexArray(mVertexArrayID);

        if (enabledState)
        {
            mFunctions->enableVertexAttribArray(idx);
        }
        else
        {
            mFunctions->disableVertexAttribArray(idx);
        }
        mAppliedAttributes[idx].enabled = enabledState;

        mStateManager->bindVertexArray(0);
    }
}

GLuint VertexArrayGL::getVertexArrayID() const
{
    return mVertexArrayID;
}

}
