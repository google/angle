//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// geometry/IndexDataManager.cpp: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#include "libGLESv2/geometry/IndexDataManager.h"

#include "common/debug.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/geometry/backend.h"

namespace
{
    enum { INITIAL_INDEX_BUFFER_SIZE = 4096 * sizeof(GLuint) };
}

namespace gl
{

IndexDataManager::IndexDataManager(Context *context, BufferBackEnd *backend)
  : mContext(context), mBackend(backend), mIntIndicesSupported(backend->supportIntIndices())
{
    mStreamBufferShort = mBackend->createIndexBuffer(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_SHORT);

    if (mIntIndicesSupported)
    {
        mStreamBufferInt = mBackend->createIndexBuffer(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_INT);
    }
    else
    {
        mStreamBufferInt = NULL;
    }
}

IndexDataManager::~IndexDataManager()
{
    delete mStreamBufferShort;
    delete mStreamBufferInt;
}

namespace
{

template <class InputIndexType, class OutputIndexType>
void copyIndices(const InputIndexType *in, GLsizei count, OutputIndexType *out, GLuint *minIndex, GLuint *maxIndex)
{
    InputIndexType first = *in;
    GLuint minIndexSoFar = first;
    GLuint maxIndexSoFar = first;

    for (GLsizei i = 0; i < count; i++)
    {
        if (minIndexSoFar > *in) minIndexSoFar = *in;
        if (maxIndexSoFar < *in) maxIndexSoFar = *in;

        *out++ = *in++;
    }

    // It might be a line loop, so copy the loop index.
    *out = first;

    *minIndex = minIndexSoFar;
    *maxIndex = maxIndexSoFar;
}

}

TranslatedIndexData IndexDataManager::preRenderValidate(GLenum mode, GLenum type, GLsizei count, Buffer *arrayElementBuffer, const void *indices)
{
    ASSERT(type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_INT);
    ASSERT(count > 0);

    TranslatedIndexData translated;

    translated.count = count;

    std::size_t requiredSpace = spaceRequired(type, count);

    TranslatedIndexBuffer *streamIb = prepareIndexBuffer(type, requiredSpace);

    size_t offset;
    void *output = streamIb->map(requiredSpace, &offset);

    translated.buffer = streamIb;
    translated.offset = offset;
    translated.indexSize = indexSize(type);

    translated.indices = output;

    if (arrayElementBuffer != NULL)
    {
        indices = static_cast<const GLubyte*>(arrayElementBuffer->data()) + reinterpret_cast<GLsizei>(indices);
    }

    if (type == GL_UNSIGNED_BYTE)
    {
        const GLubyte *in = static_cast<const GLubyte*>(indices);
        GLushort *out = static_cast<GLushort*>(output);

        copyIndices(in, count, out, &translated.minIndex, &translated.maxIndex);
    }
    else if (type == GL_UNSIGNED_INT)
    {
        const GLuint *in = static_cast<const GLuint*>(indices);

        if (mIntIndicesSupported)
        {
            GLuint *out = static_cast<GLuint*>(output);

            copyIndices(in, count, out, &translated.minIndex, &translated.maxIndex);
        }
        else
        {
            // When 32-bit indices are unsupported, fake them by truncating to 16-bit.

            GLushort *out = static_cast<GLushort*>(output);

            copyIndices(in, count, out, &translated.minIndex, &translated.maxIndex);
        }
    }
    else
    {
        const GLushort *in = static_cast<const GLushort*>(indices);
        GLushort *out = static_cast<GLushort*>(output);

        copyIndices(in, count, out, &translated.minIndex, &translated.maxIndex);
    }

    streamIb->unmap();

    return translated;
}

std::size_t IndexDataManager::indexSize(GLenum type) const
{
    return (type == GL_UNSIGNED_INT && mIntIndicesSupported) ? sizeof(GLuint) : sizeof(GLushort);
}

std::size_t IndexDataManager::spaceRequired(GLenum type, GLsizei count) const
{
    return (count + 1) * indexSize(type); // +1 because we always leave an extra for line loops
}

TranslatedIndexBuffer *IndexDataManager::prepareIndexBuffer(GLenum type, std::size_t requiredSpace)
{
    bool use32 = (type == GL_UNSIGNED_INT && mIntIndicesSupported);

    TranslatedIndexBuffer *streamIb = use32 ? mStreamBufferInt : mStreamBufferShort;

    if (requiredSpace > streamIb->size())
    {
        std::size_t newSize = std::max(requiredSpace, 2 * streamIb->size());

        TranslatedIndexBuffer *newStreamBuffer = mBackend->createIndexBuffer(newSize, use32 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT);

        delete streamIb;

        streamIb = newStreamBuffer;

        if (use32)
        {
            mStreamBufferInt = streamIb;
        }
        else
        {
            mStreamBufferShort = streamIb;
        }
    }

    streamIb->reserveSpace(requiredSpace);

    return streamIb;
}

}
