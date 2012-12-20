//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer.cpp: Defines the abstract IndexBuffer class and IndexBufferInterface
// class with derivations, classes that perform graphics API agnostic index buffer operations.

#include "libGLESv2/renderer/IndexBuffer.h"

#include "libGLESv2/renderer/Renderer9.h"

namespace rx
{

unsigned int IndexBuffer::mCurrentSerial = 1;

IndexBuffer::IndexBuffer(rx::Renderer9 *renderer, UINT size, D3DFORMAT format) : mRenderer(renderer), mBufferSize(size), mIndexBuffer(NULL)
{
    if (size > 0)
    {
        // D3D9_REPLACE
        HRESULT result = mRenderer->createIndexBuffer(size, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, format, &mIndexBuffer);
        mSerial = issueSerial();

        if (FAILED(result))
        {
            ERR("Out of memory allocating an index buffer of size %lu.", size);
        }
    }
}

IndexBuffer::~IndexBuffer()
{
    if (mIndexBuffer)
    {
        mIndexBuffer->Release();
    }
}

IDirect3DIndexBuffer9 *IndexBuffer::getBuffer() const
{
    return mIndexBuffer;
}

unsigned int IndexBuffer::getSerial() const
{
    return mSerial;
}

unsigned int IndexBuffer::issueSerial()
{
    return mCurrentSerial++;
}

void IndexBuffer::unmap()
{
    if (mIndexBuffer)
    {
        mIndexBuffer->Unlock();
    }
}

StreamingIndexBuffer::StreamingIndexBuffer(rx::Renderer9 *renderer, UINT initialSize, D3DFORMAT format) : IndexBuffer(renderer, initialSize, format)
{
    mWritePosition = 0;
}

StreamingIndexBuffer::~StreamingIndexBuffer()
{
}

void *StreamingIndexBuffer::map(UINT requiredSpace, UINT *offset)
{
    void *mapPtr = NULL;

    if (mIndexBuffer)
    {
        HRESULT result = mIndexBuffer->Lock(mWritePosition, requiredSpace, &mapPtr, D3DLOCK_NOOVERWRITE);

        if (FAILED(result))
        {
            ERR(" Lock failed with error 0x%08x", result);
            return NULL;
        }

        *offset = mWritePosition;
        mWritePosition += requiredSpace;
    }

    return mapPtr;
}

void StreamingIndexBuffer::reserveSpace(UINT requiredSpace, GLenum type)
{
    if (requiredSpace > mBufferSize)
    {
        if (mIndexBuffer)
        {
            mIndexBuffer->Release();
            mIndexBuffer = NULL;
        }

        mBufferSize = std::max(requiredSpace, 2 * mBufferSize);

        // D3D9_REPLACE
        HRESULT result = mRenderer->createIndexBuffer(mBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, type == GL_UNSIGNED_INT ? D3DFMT_INDEX32 : D3DFMT_INDEX16, &mIndexBuffer);
        mSerial = issueSerial();

        if (FAILED(result))
        {
            ERR("Out of memory allocating a vertex buffer of size %lu.", mBufferSize);
        }

        mWritePosition = 0;
    }
    else if (mWritePosition + requiredSpace > mBufferSize)   // Recycle
    {
        void *dummy;
        mIndexBuffer->Lock(0, 1, &dummy, D3DLOCK_DISCARD);
        mIndexBuffer->Unlock();

        mWritePosition = 0;
    }
}

StaticIndexBuffer::StaticIndexBuffer(rx::Renderer9 *renderer) : IndexBuffer(renderer, 0, D3DFMT_UNKNOWN)
{
    mCacheType = GL_NONE;
}

StaticIndexBuffer::~StaticIndexBuffer()
{
}

void *StaticIndexBuffer::map(UINT requiredSpace, UINT *offset)
{
    void *mapPtr = NULL;

    if (mIndexBuffer)
    {
        HRESULT result = mIndexBuffer->Lock(0, requiredSpace, &mapPtr, 0);

        if (FAILED(result))
        {
            ERR(" Lock failed with error 0x%08x", result);
            return NULL;
        }

        *offset = 0;
    }

    return mapPtr;
}

void StaticIndexBuffer::reserveSpace(UINT requiredSpace, GLenum type)
{
    if (!mIndexBuffer && mBufferSize == 0)
    {
        // D3D9_REPLACE
        HRESULT result = mRenderer->createIndexBuffer(requiredSpace, D3DUSAGE_WRITEONLY, type == GL_UNSIGNED_INT ? D3DFMT_INDEX32 : D3DFMT_INDEX16, &mIndexBuffer);
        mSerial = issueSerial();

        if (FAILED(result))
        {
            ERR("Out of memory allocating a vertex buffer of size %lu.", mBufferSize);
        }

        mBufferSize = requiredSpace;
        mCacheType = type;
    }
    else if (mIndexBuffer && mBufferSize >= requiredSpace && mCacheType == type)
    {
        // Already allocated
    }
    else UNREACHABLE();   // Static index buffers can't be resized
}

bool StaticIndexBuffer::lookupType(GLenum type)
{
    return mCacheType == type;
}

UINT StaticIndexBuffer::lookupRange(intptr_t offset, GLsizei count, UINT *minIndex, UINT *maxIndex)
{
    IndexRange range = {offset, count};

    std::map<IndexRange, IndexResult>::iterator res = mCache.find(range);

    if (res == mCache.end())
    {
        return -1;
    }

    *minIndex = res->second.minIndex;
    *maxIndex = res->second.maxIndex;
    return res->second.streamOffset;
}

void StaticIndexBuffer::addRange(intptr_t offset, GLsizei count, UINT minIndex, UINT maxIndex, UINT streamOffset)
{
    IndexRange indexRange = {offset, count};
    IndexResult indexResult = {minIndex, maxIndex, streamOffset};
    mCache[indexRange] = indexResult;
}

}

