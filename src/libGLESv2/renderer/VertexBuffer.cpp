//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer.cpp: Defines the VertexBuffer and derivations, classes that
// perform graphics API agnostic vertex buffer operations.

#include "libGLESv2/renderer/VertexBuffer.h"

#include "libGLESv2/renderer/Renderer9.h"

namespace rx
{

unsigned int VertexBuffer::mCurrentSerial = 1;

VertexBuffer::VertexBuffer(rx::Renderer9 *renderer, std::size_t size, DWORD usageFlags) : mRenderer(renderer), mVertexBuffer(NULL)
{
    if (size > 0)
    {
        // D3D9_REPLACE
        HRESULT result = mRenderer->createVertexBuffer(size, usageFlags,&mVertexBuffer);
        mSerial = issueSerial();

        if (FAILED(result))
        {
            ERR("Out of memory allocating a vertex buffer of size %lu.", size);
        }
    }
}

VertexBuffer::~VertexBuffer()
{
    if (mVertexBuffer)
    {
        mVertexBuffer->Release();
    }
}

void VertexBuffer::unmap()
{
    if (mVertexBuffer)
    {
        mVertexBuffer->Unlock();
    }
}

IDirect3DVertexBuffer9 *VertexBuffer::getBuffer() const
{
    return mVertexBuffer;
}

unsigned int VertexBuffer::getSerial() const
{
    return mSerial;
}

unsigned int VertexBuffer::issueSerial()
{
    return mCurrentSerial++;
}

ArrayVertexBuffer::ArrayVertexBuffer(rx::Renderer9 *renderer, std::size_t size, DWORD usageFlags) : VertexBuffer(renderer, size, usageFlags)
{
    mBufferSize = size;
    mWritePosition = 0;
    mRequiredSpace = 0;
}

ArrayVertexBuffer::~ArrayVertexBuffer()
{
}

void ArrayVertexBuffer::addRequiredSpace(UINT requiredSpace)
{
    mRequiredSpace += requiredSpace;
}

StreamingVertexBuffer::StreamingVertexBuffer(rx::Renderer9 *renderer, std::size_t initialSize) : ArrayVertexBuffer(renderer, initialSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY)
{
}

StreamingVertexBuffer::~StreamingVertexBuffer()
{
}

void *StreamingVertexBuffer::map(const gl::VertexAttribute &attribute, std::size_t requiredSpace, std::size_t *offset)
{
    void *mapPtr = NULL;

    if (mVertexBuffer)
    {
        HRESULT result = mVertexBuffer->Lock(mWritePosition, requiredSpace, &mapPtr, D3DLOCK_NOOVERWRITE);

        if (FAILED(result))
        {
            ERR("Lock failed with error 0x%08x", result);
            return NULL;
        }

        *offset = mWritePosition;
        mWritePosition += requiredSpace;
    }

    return mapPtr;
}

void StreamingVertexBuffer::reserveRequiredSpace()
{
    if (mRequiredSpace > mBufferSize)
    {
        if (mVertexBuffer)
        {
            mVertexBuffer->Release();
            mVertexBuffer = NULL;
        }

        mBufferSize = std::max(mRequiredSpace, 3 * mBufferSize / 2);   // 1.5 x mBufferSize is arbitrary and should be checked to see we don't have too many reallocations.

        // D3D9_REPLACE
        HRESULT result = mRenderer->createVertexBuffer(mBufferSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, &mVertexBuffer);
        mSerial = issueSerial();

        if (FAILED(result))
        {
            ERR("Out of memory allocating a vertex buffer of size %lu.", mBufferSize);
        }

        mWritePosition = 0;
    }
    else if (mWritePosition + mRequiredSpace > mBufferSize)   // Recycle
    {
        if (mVertexBuffer)
        {
            void *dummy;
            mVertexBuffer->Lock(0, 1, &dummy, D3DLOCK_DISCARD);
            mVertexBuffer->Unlock();
        }

        mWritePosition = 0;
    }

    mRequiredSpace = 0;
}

StaticVertexBuffer::StaticVertexBuffer(rx::Renderer9 *renderer) : ArrayVertexBuffer(renderer, 0, D3DUSAGE_WRITEONLY)
{
}

StaticVertexBuffer::~StaticVertexBuffer()
{
}

void *StaticVertexBuffer::map(const gl::VertexAttribute &attribute, std::size_t requiredSpace, std::size_t *streamOffset)
{
    void *mapPtr = NULL;

    if (mVertexBuffer)
    {
        HRESULT result = mVertexBuffer->Lock(mWritePosition, requiredSpace, &mapPtr, 0);

        if (FAILED(result))
        {
            ERR("Lock failed with error 0x%08x", result);
            return NULL;
        }

        int attributeOffset = attribute.mOffset % attribute.stride();
        VertexElement element = {attribute.mType, attribute.mSize, attribute.stride(), attribute.mNormalized, attributeOffset, mWritePosition};
        mCache.push_back(element);

        *streamOffset = mWritePosition;
        mWritePosition += requiredSpace;
    }

    return mapPtr;
}

void StaticVertexBuffer::reserveRequiredSpace()
{
    if (!mVertexBuffer && mBufferSize == 0)
    {
        // D3D9_REPLACE
        HRESULT result = mRenderer->createVertexBuffer(mRequiredSpace, D3DUSAGE_WRITEONLY, &mVertexBuffer);
        mSerial = issueSerial();

        if (FAILED(result))
        {
            ERR("Out of memory allocating a vertex buffer of size %lu.", mRequiredSpace);
        }

        mBufferSize = mRequiredSpace;
    }
    else if (mVertexBuffer && mBufferSize >= mRequiredSpace)
    {
        // Already allocated
    }
    else UNREACHABLE();   // Static vertex buffers can't be resized

    mRequiredSpace = 0;
}

std::size_t StaticVertexBuffer::lookupAttribute(const gl::VertexAttribute &attribute)
{
    for (unsigned int element = 0; element < mCache.size(); element++)
    {
        if (mCache[element].type == attribute.mType &&
            mCache[element].size == attribute.mSize &&
            mCache[element].stride == attribute.stride() &&
            mCache[element].normalized == attribute.mNormalized)
        {
            if (mCache[element].attributeOffset == attribute.mOffset % attribute.stride())
            {
                return mCache[element].streamOffset;
            }
        }
    }

    return -1;
}

}
