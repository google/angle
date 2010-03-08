//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer.cpp: Implements the gl::Buffer class, representing storage of vertex and/or
// index data. Implements GL buffer objects and related functionality.
// [OpenGL ES 2.0.24] section 2.9 page 21.

#include "Buffer.h"

#include "main.h"

namespace gl
{
Buffer::Buffer()
{
    mSize = 0;
    mData = NULL;

    mVertexBuffer = NULL;
    mIndexBuffer = NULL;
}

Buffer::~Buffer()
{
    erase();
}

void Buffer::storeData(GLsizeiptr size, const void *data)
{
    erase();

    mSize = size;
    mData = new unsigned char[size];

    if (data)
    {
        memcpy(mData, data, size);
    }
}

IDirect3DVertexBuffer9 *Buffer::getVertexBuffer()
{
    if (!mVertexBuffer)
    {
        IDirect3DDevice9 *device = getDevice();

        HRESULT result = device->CreateVertexBuffer(mSize, 0, 0, D3DPOOL_MANAGED, &mVertexBuffer, NULL);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            return error(GL_OUT_OF_MEMORY, (IDirect3DVertexBuffer9*)NULL);
        }

        ASSERT(SUCCEEDED(result));

        if (mVertexBuffer && mData)
        {
            void *dataStore;
            mVertexBuffer->Lock(0, mSize, &dataStore, 0);
            memcpy(dataStore, mData, mSize);
            mVertexBuffer->Unlock();
        }
    }

    return mVertexBuffer;
}

IDirect3DIndexBuffer9 *Buffer::getIndexBuffer()
{
    if (!mIndexBuffer)
    {
        IDirect3DDevice9 *device = getDevice();

        HRESULT result = device->CreateIndexBuffer(mSize, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &mIndexBuffer, NULL);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            return error(GL_OUT_OF_MEMORY, (IDirect3DIndexBuffer9*)NULL);
        }

        ASSERT(SUCCEEDED(result));

        if (mIndexBuffer && mData)
        {
            void *dataStore;
            mIndexBuffer->Lock(0, mSize, &dataStore, 0);
            memcpy(dataStore, mData, mSize);
            mIndexBuffer->Unlock();
        }
    }

    return mIndexBuffer;
}

void Buffer::erase()
{
    mSize = 0;

    if (mData)
    {
        delete[] mData;
        mData = NULL;
    }

    if (mVertexBuffer)
    {
        mVertexBuffer->Release();
        mVertexBuffer = NULL;
    }

    if (mIndexBuffer)
    {
        mIndexBuffer->Release();
        mIndexBuffer = NULL;
    }
}
}
