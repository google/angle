//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// geometry/IndexDataManager.cpp: Defines the IndexDataManager, a class that
// runs the Buffer translation process for index buffers.

#include "geometry/IndexDataManager.h"

#include "common/debug.h"
#include "Buffer.h"
#include "geometry/backend.h"

namespace
{
    enum { INITIAL_INDEX_BUFFER_SIZE = sizeof(gl::Index) * 8192 };
}

namespace gl
{

IndexDataManager::IndexDataManager(Context *context, BufferBackEnd *backend)
    : mContext(context), mBackend(backend)
{
    mStreamBuffer = mBackend->createIndexBuffer(INITIAL_INDEX_BUFFER_SIZE);
}

IndexDataManager::~IndexDataManager()
{
    delete mStreamBuffer;
}

namespace
{

template <class InputIndexType>
void copyIndices(const InputIndexType *in, GLsizei count, Index *out, GLuint *minIndex, GLuint *maxIndex)
{
    GLuint minIndexSoFar = *in;
    GLuint maxIndexSoFar = *in;

    for (GLsizei i = 0; i < count; i++)
    {
        if (minIndexSoFar > *in) minIndexSoFar = *in;
        if (maxIndexSoFar < *in) maxIndexSoFar = *in;

        *out++ = *in++;
    }

    *minIndex = minIndexSoFar;
    *maxIndex = maxIndexSoFar;
}

}

TranslatedIndexData IndexDataManager::preRenderValidate(GLenum mode, GLenum type, GLsizei count, Buffer *arrayElementBuffer, const void *indices)
{
    ASSERT(type == GL_UNSIGNED_SHORT || type == GL_UNSIGNED_BYTE);
    ASSERT(count > 0);

    TranslatedIndexData translated;

    translated.count = count;

    std::size_t requiredSpace = spaceRequired(mode, type, count);

    if (requiredSpace > mStreamBuffer->size())
    {
        std::size_t newSize = std::max(requiredSpace, 2 * mStreamBuffer->size());

        TranslatedIndexBuffer *newStreamBuffer = mBackend->createIndexBuffer(newSize);

        delete mStreamBuffer;
        mStreamBuffer = newStreamBuffer;
    }

    mStreamBuffer->reserveSpace(requiredSpace);

    size_t offset;
    void *output = mStreamBuffer->map(requiredSpace, &offset);

    translated.buffer = mStreamBuffer;
    translated.offset = offset;

    translated.indices = static_cast<const Index*>(output);

    if (arrayElementBuffer != NULL)
    {
        indices = static_cast<const GLubyte*>(arrayElementBuffer->data()) + reinterpret_cast<GLsizei>(indices);
    }

    Index *out = static_cast<Index*>(output);

    if (type == GL_UNSIGNED_SHORT)
    {
        const GLushort *in = static_cast<const GLushort*>(indices);

        copyIndices(in, count, out, &translated.minIndex, &translated.maxIndex);
    }
    else
    {
        const GLubyte *in = static_cast<const GLubyte*>(indices);

        copyIndices(in, count, out, &translated.minIndex, &translated.maxIndex);
    }

    mStreamBuffer->unmap();

    return translated;
}

std::size_t IndexDataManager::spaceRequired(GLenum mode, GLenum type, GLsizei count)
{
    return count * sizeof(Index);
}

}
