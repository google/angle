//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// geometry/VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#include "libGLESv2/geometry/VertexDataManager.h"

#include "common/debug.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/Program.h"

#include "libGLESv2/geometry/backend.h"
#include "libGLESv2/geometry/IndexDataManager.h"

namespace
{
    enum { INITIAL_STREAM_BUFFER_SIZE = 1024*1024 };
}

namespace gl
{

VertexDataManager::VertexDataManager(Context *context, BufferBackEnd *backend)
    : mContext(context), mBackend(backend), mDirtyCurrentValues(true), mCurrentValueOffset(0)
{
    mStreamBuffer = mBackend->createVertexBuffer(INITIAL_STREAM_BUFFER_SIZE);
    try
    {
        mCurrentValueBuffer = mBackend->createVertexBufferForStrideZero(4*sizeof(float)*MAX_VERTEX_ATTRIBS);
    }
    catch (...)
    {
        delete mStreamBuffer;
        throw;
    }
}

VertexDataManager::~VertexDataManager()
{
    delete mStreamBuffer;
    delete mCurrentValueBuffer;
}

std::bitset<MAX_VERTEX_ATTRIBS> VertexDataManager::getActiveAttribs() const
{
    std::bitset<MAX_VERTEX_ATTRIBS> active;

    Program *program = mContext->getCurrentProgram();

    for (int attributeIndex = 0; attributeIndex < MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        active[attributeIndex] = (program->getSemanticIndex(attributeIndex) != -1);
    }

    return active;
}

GLenum VertexDataManager::preRenderValidate(GLint start, GLsizei count,
                                            TranslatedAttribute *translated)
{
    const AttributeState *attribs = mContext->getVertexAttribBlock();
    const std::bitset<MAX_VERTEX_ATTRIBS> activeAttribs = getActiveAttribs();

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        if (!activeAttribs[i] && attribs[i].mEnabled && attribs[i].mBoundBuffer != 0 && !mContext->getBuffer(attribs[i].mBoundBuffer))
            return GL_INVALID_OPERATION;
    }

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        translated[i].enabled = activeAttribs[i];
    }

    processNonArrayAttributes(attribs, activeAttribs, translated);

    // Process array attributes.

    std::size_t requiredSpace = 0;

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        if (activeAttribs[i] && attribs[i].mEnabled)
        {
            requiredSpace += spaceRequired(attribs[i], count);
        }
    }

    if (requiredSpace > mStreamBuffer->size())
    {
        std::size_t newSize = std::max(requiredSpace, 3 * mStreamBuffer->size() / 2); // 1.5 x mStreamBuffer->size() is arbitrary and should be checked to see we don't have too many reallocations.

        TranslatedVertexBuffer *newStreamBuffer = mBackend->createVertexBuffer(newSize);

        delete mStreamBuffer;
        mStreamBuffer = newStreamBuffer;
    }

    mStreamBuffer->reserveSpace(requiredSpace);

    for (size_t i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        if (activeAttribs[i] && attribs[i].mEnabled)
        {
            FormatConverter formatConverter = mBackend->getFormatConverter(attribs[i].mType, attribs[i].mSize, attribs[i].mNormalized);

            translated[i].type = attribs[i].mType;
            translated[i].size = attribs[i].mSize;
            translated[i].normalized = attribs[i].mNormalized;
            translated[i].stride = formatConverter.outputVertexSize;
            translated[i].buffer = mStreamBuffer;

            void *output = mStreamBuffer->map(spaceRequired(attribs[i], count), &translated[i].offset);

            const void *input;
            if (attribs[i].mBoundBuffer)
            {
                input = mContext->getBuffer(attribs[i].mBoundBuffer)->data();
                input = static_cast<const char*>(input) + reinterpret_cast<size_t>(attribs[i].mPointer);
            }
            else
            {
                input = attribs[i].mPointer;
            }

            size_t inputStride = interpretGlStride(attribs[i]);

            input = static_cast<const char*>(input) + inputStride * start;

            if (formatConverter.identity && inputStride == typeSize(attribs[i].mType) * attribs[i].mSize)
            {
                memcpy(output, input, count * inputStride);
            }
            else
            {
                formatConverter.convertArray(input, inputStride, count, output);
            }

            mStreamBuffer->unmap();
        }
    }

    return GL_NO_ERROR;
}

void VertexDataManager::reloadCurrentValues(const AttributeState *attribs, std::size_t *offset)
{
    if (mDirtyCurrentValues)
    {
        std::size_t totalSize = 4 * sizeof(float) * MAX_VERTEX_ATTRIBS;

        mCurrentValueBuffer->reserveSpace(totalSize);

        float* p = static_cast<float*>(mCurrentValueBuffer->map(totalSize, &mCurrentValueOffset));

        for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
        {
            memcpy(&p[i*4], attribs[i].mCurrentValue, sizeof(attribs[i].mCurrentValue)); // FIXME: this should be doing a translation. This assumes that GL_FLOATx4 is supported.
        }

        mCurrentValueBuffer->unmap();

        mDirtyCurrentValues = false;
    }

    *offset = mCurrentValueOffset;
}

std::size_t VertexDataManager::typeSize(GLenum type) const
{
    switch (type)
    {
      case GL_BYTE: case GL_UNSIGNED_BYTE: return sizeof(GLbyte);
      case GL_SHORT: case GL_UNSIGNED_SHORT: return sizeof(GLshort);
      case GL_FIXED: return sizeof(GLfixed);
      case GL_FLOAT: return sizeof(GLfloat);
      default: UNREACHABLE(); return sizeof(GLfloat);
    }
}

std::size_t VertexDataManager::interpretGlStride(const AttributeState &attrib) const
{
    return attrib.mStride ? attrib.mStride : typeSize(attrib.mType) * attrib.mSize;
}

// Round up x (>=0) to the next multiple of multiple (>0).
// 0 rounds up to 0.
std::size_t VertexDataManager::roundUp(std::size_t x, std::size_t multiple) const
{
    ASSERT(x >= 0);
    ASSERT(multiple > 0);

    std::size_t remainder = x % multiple;
    if (remainder != 0)
    {
        return x + multiple - remainder;
    }
    else
    {
        return x;
    }
}

std::size_t VertexDataManager::spaceRequired(const AttributeState &attrib, std::size_t maxVertex) const
{
    std::size_t size = mBackend->getFormatConverter(attrib.mType, attrib.mSize, attrib.mNormalized).outputVertexSize;
    size *= maxVertex;

    return roundUp(size, 4 * sizeof(GLfloat));
}

void VertexDataManager::processNonArrayAttributes(const AttributeState *attribs, const std::bitset<MAX_VERTEX_ATTRIBS> &activeAttribs, TranslatedAttribute *translated)
{
    bool usesCurrentValues = false;

    for (int i = 0; i < MAX_VERTEX_ATTRIBS; i++)
    {
        if (activeAttribs[i] && !attribs[i].mEnabled)
        {
            usesCurrentValues = true;
            break;
        }
    }

    if (usesCurrentValues)
    {
        std::size_t currentValueOffset;

        reloadCurrentValues(attribs, &currentValueOffset);

        for (std::size_t i = 0; i < MAX_VERTEX_ATTRIBS; i++)
        {
            if (activeAttribs[i] && !attribs[i].mEnabled)
            {
                translated[i].buffer = mCurrentValueBuffer;

                translated[i].type = GL_FLOAT;
                translated[i].size = 4;
                translated[i].normalized = false;
                translated[i].stride = 0;
                translated[i].offset = currentValueOffset + 4 * sizeof(float) * i;
            }
        }
    }
}

}
