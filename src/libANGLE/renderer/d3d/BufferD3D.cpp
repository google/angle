//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferD3D.cpp Defines common functionality between the Buffer9 and Buffer11 classes.

#include "libANGLE/renderer/d3d/BufferD3D.h"
#include "libANGLE/renderer/d3d/VertexBuffer.h"
#include "libANGLE/renderer/d3d/IndexBuffer.h"

namespace rx
{

unsigned int BufferD3D::mNextSerial = 1;

BufferD3D::BufferD3D()
    : BufferImpl(),
      mStaticVertexBuffer(NULL),
      mStaticIndexBuffer(NULL)
{
    updateSerial();
}

BufferD3D::~BufferD3D()
{
    SafeDelete(mStaticVertexBuffer);
    SafeDelete(mStaticIndexBuffer);
}

void BufferD3D::updateSerial()
{
    mSerial = mNextSerial++;
}

void BufferD3D::initializeStaticData()
{
    if (!mStaticVertexBuffer)
    {
        mStaticVertexBuffer = new StaticVertexBufferInterface(getRenderer());
    }
    if (!mStaticIndexBuffer)
    {
        mStaticIndexBuffer = new StaticIndexBufferInterface(getRenderer());
    }
}

void BufferD3D::invalidateStaticData()
{
    if ((mStaticVertexBuffer && mStaticVertexBuffer->getBufferSize() != 0) || (mStaticIndexBuffer && mStaticIndexBuffer->getBufferSize() != 0))
    {
        SafeDelete(mStaticVertexBuffer);
        SafeDelete(mStaticIndexBuffer);

        // Re-init static data to track that we're in a static buffer
        initializeStaticData();
    }

    mUnmodifiedDataUse = 0;
}

// Creates static buffers if sufficient used data has been left unmodified
void BufferD3D::promoteStaticUsage(int dataSize)
{
    if (!mStaticVertexBuffer && !mStaticIndexBuffer)
    {
        mUnmodifiedDataUse += dataSize;

        if (mUnmodifiedDataUse > 3 * getSize())
        {
            initializeStaticData();
        }
    }
}

}