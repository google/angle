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

unsigned int IndexBufferInterface::mCurrentSerial = 1;

IndexBufferInterface::IndexBufferInterface(rx::Renderer9 *renderer, UINT size, D3DFORMAT format) : mRenderer(renderer), mBufferSize(size), mIndexBuffer(NULL)
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

IndexBufferInterface::~IndexBufferInterface()
{
    if (mIndexBuffer)
    {
        mIndexBuffer->Release();
    }
}

IDirect3DIndexBuffer9 *IndexBufferInterface::getBuffer() const
{
    return mIndexBuffer;
}

unsigned int IndexBufferInterface::getSerial() const
{
    return mSerial;
}

unsigned int IndexBufferInterface::issueSerial()
{
    return mCurrentSerial++;
}

void IndexBufferInterface::unmap()
{
    if (mIndexBuffer)
    {
        mIndexBuffer->Unlock();
    }
}

StreamingIndexBufferInterface::StreamingIndexBufferInterface(rx::Renderer9 *renderer, UINT initialSize, D3DFORMAT format) : IndexBufferInterface(renderer, initialSize, format)
{
    mWritePosition = 0;
}

StreamingIndexBufferInterface::~StreamingIndexBufferInterface()
{
}

void *StreamingIndexBufferInterface::map(UINT requiredSpace, UINT *offset)
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

void StreamingIndexBufferInterface::reserveSpace(UINT requiredSpace, GLenum type)
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

StaticIndexBufferInterface::StaticIndexBufferInterface(rx::Renderer9 *renderer) : IndexBufferInterface(renderer, 0, D3DFMT_UNKNOWN)
{
    mCacheType = GL_NONE;
}

StaticIndexBufferInterface::~StaticIndexBufferInterface()
{
}

void *StaticIndexBufferInterface::map(UINT requiredSpace, UINT *offset)
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

void StaticIndexBufferInterface::reserveSpace(UINT requiredSpace, GLenum type)
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

bool StaticIndexBufferInterface::lookupType(GLenum type)
{
    return mCacheType == type;
}

UINT StaticIndexBufferInterface::lookupRange(intptr_t offset, GLsizei count, UINT *minIndex, UINT *maxIndex)
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

void StaticIndexBufferInterface::addRange(intptr_t offset, GLsizei count, UINT minIndex, UINT maxIndex, UINT streamOffset)
{
    IndexRange indexRange = {offset, count};
    IndexResult indexResult = {minIndex, maxIndex, streamOffset};
    mCache[indexRange] = indexResult;
}

}

