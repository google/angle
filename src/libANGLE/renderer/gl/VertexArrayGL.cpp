//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexArrayGL.cpp: Implements the class methods for VertexArrayGL.

#include "libANGLE/renderer/gl/VertexArrayGL.h"

#include "common/debug.h"
#include "libANGLE/Buffer.h"
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
      mElementArrayBuffer(),
      mAttributes(),
      mAppliedElementArrayBuffer(0),
      mAppliedAttributes()
{
    ASSERT(mFunctions);
    ASSERT(mStateManager);
    mFunctions->genVertexArrays(1, &mVertexArrayID);

    // Set the cached vertex attribute array size
    GLint maxVertexAttribs;
    mFunctions->getIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    mAttributes.resize(maxVertexAttribs);
    mAppliedAttributes.resize(maxVertexAttribs);
}

VertexArrayGL::~VertexArrayGL()
{
    if (mVertexArrayID != 0)
    {
        mFunctions->deleteVertexArrays(1, &mVertexArrayID);
        mVertexArrayID = 0;
    }

    mElementArrayBuffer.set(nullptr);
    for (size_t idx = 0; idx < mAppliedAttributes.size(); idx++)
    {
        mAppliedAttributes[idx].buffer.set(NULL);
    }
}

void VertexArrayGL::setElementArrayBuffer(const gl::Buffer *buffer)
{
    mElementArrayBuffer.set(buffer);
}

void VertexArrayGL::setAttribute(size_t idx, const gl::VertexAttribute &attr)
{
    mAttributes[idx] = attr;
}

void VertexArrayGL::setAttributeDivisor(size_t idx, GLuint divisor)
{
    mAttributes[idx].divisor = divisor;
}

void VertexArrayGL::enableAttribute(size_t idx, bool enabledState)
{
    mAttributes[idx].enabled = enabledState;
}

void VertexArrayGL::syncState() const
{
    mStateManager->bindVertexArray(mVertexArrayID);

    GLuint elementArrayBufferID = 0;
    if (mElementArrayBuffer.get() != nullptr)
    {
        const BufferGL *bufferGL = GetImplAs<BufferGL>(mElementArrayBuffer.get());
        elementArrayBufferID = bufferGL->getBufferID();
    }

    if (elementArrayBufferID != mAppliedElementArrayBuffer)
    {
        mStateManager->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBufferID);
        mAppliedElementArrayBuffer = elementArrayBufferID;
    }

    for (size_t idx = 0; idx < mAttributes.size(); idx++)
    {
        if (mAppliedAttributes[idx] != mAttributes[idx])
        {
            if (mAttributes[idx].enabled)
            {
                mFunctions->enableVertexAttribArray(idx);
            }
            else
            {
                mFunctions->disableVertexAttribArray(idx);
            }

            const gl::Buffer *arrayBuffer = mAttributes[idx].buffer.get();
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

            if (mAttributes[idx].pureInteger)
            {
                mFunctions->vertexAttribIPointer(idx, mAttributes[idx].size, mAttributes[idx].type,
                                                 mAttributes[idx].stride, mAttributes[idx].pointer);
            }
            else
            {
                mFunctions->vertexAttribPointer(idx, mAttributes[idx].size, mAttributes[idx].type,
                                                mAttributes[idx].normalized, mAttributes[idx].stride,
                                                mAttributes[idx].pointer);
            }

            mFunctions->vertexAttribDivisor(idx, mAttributes[idx].divisor);

            mAppliedAttributes[idx] = mAttributes[idx];
        }
    }
}

GLuint VertexArrayGL::getVertexArrayID() const
{
    return mVertexArrayID;
}

}
