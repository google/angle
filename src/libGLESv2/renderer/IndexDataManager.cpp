//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexDataManager.cpp: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#include "libGLESv2/renderer/IndexDataManager.h"

#include "common/debug.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/main.h"

namespace rx
{

IndexDataManager::IndexDataManager(rx::Renderer9 *renderer) : mRenderer(renderer)
{
    mStreamingBufferShort = new StreamingIndexBufferInterface(mRenderer, INITIAL_INDEX_BUFFER_SIZE, D3DFMT_INDEX16);

    if (renderer->get32BitIndexSupport())
    {
        mStreamingBufferInt = new StreamingIndexBufferInterface(mRenderer, INITIAL_INDEX_BUFFER_SIZE, D3DFMT_INDEX32);

        if (!mStreamingBufferInt)
        {
            // Don't leave it in a half-initialized state
            delete mStreamingBufferShort;
            mStreamingBufferShort = NULL;
        }
    }
    else
    {
        mStreamingBufferInt = NULL;
    }

    if (!mStreamingBufferShort)
    {
        ERR("Failed to allocate the streaming index buffer(s).");
    }

    mCountingBuffer = NULL;
}

IndexDataManager::~IndexDataManager()
{
    delete mStreamingBufferShort;
    delete mStreamingBufferInt;
    delete mCountingBuffer;
}

void convertIndices(GLenum type, const void *input, GLsizei count, void *output)
{
    if (type == GL_UNSIGNED_BYTE)
    {
        const GLubyte *in = static_cast<const GLubyte*>(input);
        GLushort *out = static_cast<GLushort*>(output);

        for (GLsizei i = 0; i < count; i++)
        {
            out[i] = in[i];
        }
    }
    else if (type == GL_UNSIGNED_INT)
    {
        memcpy(output, input, count * sizeof(GLuint));
    }
    else if (type == GL_UNSIGNED_SHORT)
    {
        memcpy(output, input, count * sizeof(GLushort));
    }
    else UNREACHABLE();
}

template <class IndexType>
void computeRange(const IndexType *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
    *minIndex = indices[0];
    *maxIndex = indices[0];

    for (GLsizei i = 0; i < count; i++)
    {
        if (*minIndex > indices[i]) *minIndex = indices[i];
        if (*maxIndex < indices[i]) *maxIndex = indices[i];
    }
}

void computeRange(GLenum type, const GLvoid *indices, GLsizei count, GLuint *minIndex, GLuint *maxIndex)
{
    if (type == GL_UNSIGNED_BYTE)
    {
        computeRange(static_cast<const GLubyte*>(indices), count, minIndex, maxIndex);
    }
    else if (type == GL_UNSIGNED_INT)
    {
        computeRange(static_cast<const GLuint*>(indices), count, minIndex, maxIndex);
    }
    else if (type == GL_UNSIGNED_SHORT)
    {
        computeRange(static_cast<const GLushort*>(indices), count, minIndex, maxIndex);
    }
    else UNREACHABLE();
}

GLenum IndexDataManager::prepareIndexData(GLenum type, GLsizei count, gl::Buffer *buffer, const GLvoid *indices, TranslatedIndexData *translated, IDirect3DIndexBuffer9 **d3dIndexBuffer, unsigned int *serial)
{
    if (!mStreamingBufferShort)
    {
        return GL_OUT_OF_MEMORY;
    }

    D3DFORMAT format = (type == GL_UNSIGNED_INT) ? D3DFMT_INDEX32 : D3DFMT_INDEX16;
    intptr_t offset = reinterpret_cast<intptr_t>(indices);
    bool alignedOffset = false;

    if (buffer != NULL)
    {
        switch (type)
        {
          case GL_UNSIGNED_BYTE:  alignedOffset = (offset % sizeof(GLubyte) == 0);  break;
          case GL_UNSIGNED_SHORT: alignedOffset = (offset % sizeof(GLushort) == 0); break;
          case GL_UNSIGNED_INT:   alignedOffset = (offset % sizeof(GLuint) == 0);   break;
          default: UNREACHABLE(); alignedOffset = false;
        }

        if (typeSize(type) * count + offset > static_cast<std::size_t>(buffer->size()))
        {
            return GL_INVALID_OPERATION;
        }

        indices = static_cast<const GLubyte*>(buffer->data()) + offset;
    }

    StreamingIndexBufferInterface *streamingBuffer = (type == GL_UNSIGNED_INT) ? mStreamingBufferInt : mStreamingBufferShort;

    StaticIndexBufferInterface *staticBuffer = buffer ? buffer->getStaticIndexBuffer() : NULL;
    IndexBufferInterface *indexBuffer = streamingBuffer;
    UINT streamOffset = 0;

    if (staticBuffer && staticBuffer->lookupType(type) && alignedOffset)
    {
        indexBuffer = staticBuffer;
        streamOffset = staticBuffer->lookupRange(offset, count, &translated->minIndex, &translated->maxIndex);

        if (streamOffset == -1)
        {
            streamOffset = (offset / typeSize(type)) * indexSize(format);
            computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);
            staticBuffer->addRange(offset, count, translated->minIndex, translated->maxIndex, streamOffset);
        }
    }
    else
    {
        int convertCount = count;

        if (staticBuffer)
        {
            if (staticBuffer->size() == 0 && alignedOffset)
            {
                indexBuffer = staticBuffer;
                convertCount = buffer->size() / typeSize(type);
            }
            else
            {
                buffer->invalidateStaticData();
                staticBuffer = NULL;
            }
        }

        void *output = NULL;

        if (indexBuffer)
        {
            indexBuffer->reserveSpace(convertCount * indexSize(format), type);
            output = indexBuffer->map(indexSize(format) * convertCount, &streamOffset);
        }

        if (output == NULL)
        {
            ERR("Failed to map index buffer.");
            return GL_OUT_OF_MEMORY;
        }

        convertIndices(type, staticBuffer ? buffer->data() : indices, convertCount, output);
        indexBuffer->unmap();

        computeRange(type, indices, count, &translated->minIndex, &translated->maxIndex);

        if (staticBuffer)
        {
            streamOffset = (offset / typeSize(type)) * indexSize(format);
            staticBuffer->addRange(offset, count, translated->minIndex, translated->maxIndex, streamOffset);
        }
    }

    *d3dIndexBuffer = indexBuffer->getBuffer();
    *serial = indexBuffer->getSerial();
    translated->startIndex = streamOffset / indexSize(format);

    if (buffer)
    {
        buffer->promoteStaticUsage(count * typeSize(type));
    }

    return GL_NO_ERROR;
}

std::size_t IndexDataManager::indexSize(D3DFORMAT format) const
{
    return (format == D3DFMT_INDEX32) ? sizeof(unsigned int) : sizeof(unsigned short);
}

std::size_t IndexDataManager::typeSize(GLenum type) const
{
    switch (type)
    {
      case GL_UNSIGNED_INT:   return sizeof(GLuint);
      case GL_UNSIGNED_SHORT: return sizeof(GLushort);
      case GL_UNSIGNED_BYTE:  return sizeof(GLubyte);
      default: UNREACHABLE(); return sizeof(GLushort);
    }
}

StaticIndexBufferInterface *IndexDataManager::getCountingIndices(GLsizei count)
{
    if (count <= 65536)   // 16-bit indices
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned short);

        if (!mCountingBuffer || mCountingBuffer->size() < spaceNeeded)
        {
            delete mCountingBuffer;
            mCountingBuffer = new StaticIndexBufferInterface(mRenderer);
            mCountingBuffer->reserveSpace(spaceNeeded, GL_UNSIGNED_SHORT);

            UINT offset;
            unsigned short *data = static_cast<unsigned short*>(mCountingBuffer->map(spaceNeeded, &offset));

            if (data)
            {
                for(int i = 0; i < count; i++)
                {
                    data[i] = i;
                }

                mCountingBuffer->unmap();
            }
        }
    }
    else if (mStreamingBufferInt)   // 32-bit indices supported
    {
        const unsigned int spaceNeeded = count * sizeof(unsigned int);

        if (!mCountingBuffer || mCountingBuffer->size() < spaceNeeded)
        {
            delete mCountingBuffer;
            mCountingBuffer = new StaticIndexBufferInterface(mRenderer);
            mCountingBuffer->reserveSpace(spaceNeeded, GL_UNSIGNED_INT);

            UINT offset;
            unsigned int *data = static_cast<unsigned int*>(mCountingBuffer->map(spaceNeeded, &offset));

            if (data)
            {
                for(int i = 0; i < count; i++)
                {
                    data[i] = i;
                }

                mCountingBuffer->unmap();
            }
        }
    }
    else return NULL;

    return mCountingBuffer;
}

}
