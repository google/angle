#include "precompiled.h"
//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#include "libGLESv2/renderer/VertexDataManager.h"
#include "libGLESv2/renderer/BufferStorage.h"

#include "libGLESv2/Buffer.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/VertexAttribute.h"
#include "libGLESv2/renderer/VertexBuffer.h"

namespace
{
    enum { INITIAL_STREAM_BUFFER_SIZE = 1024*1024 };
    // This has to be at least 4k or else it fails on ATI cards.
    enum { CONSTANT_VERTEX_BUFFER_SIZE = 4096 };
}

namespace rx
{

static int ElementsInBuffer(const gl::VertexAttribute &attribute, int size)
{
    int stride = attribute.stride();
    return (size - attribute.mOffset % stride + (stride - attribute.typeSize())) / stride;
}

static bool DirectStoragePossible(VertexBufferInterface *vb, const gl::VertexAttribute &attrib, const gl::VertexAttribCurrentValueData &currentValue)
{
    gl::Buffer *buffer = attrib.mBoundBuffer.get();
    BufferStorage *storage = buffer ? buffer->getStorage() : NULL;

    const bool isAligned = (attrib.stride() % 4 == 0) && (attrib.mOffset % 4 == 0);
    const bool requiresConversion = attrib.mArrayEnabled ?
                                    vb->getVertexBuffer()->requiresConversion(attrib) :
                                    vb->getVertexBuffer()->requiresConversion(currentValue);

    return storage && storage->supportsDirectBinding() && !requiresConversion && isAligned;
}

VertexDataManager::VertexDataManager(Renderer *renderer) : mRenderer(renderer)
{
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        mCurrentValue[i].FloatValues[0] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].FloatValues[1] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].FloatValues[2] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].FloatValues[3] = std::numeric_limits<float>::quiet_NaN();
        mCurrentValue[i].Type = GL_FLOAT;
        mCurrentValueBuffer[i] = NULL;
        mCurrentValueOffsets[i] = 0;
    }

    mStreamingBuffer = new StreamingVertexBufferInterface(renderer, INITIAL_STREAM_BUFFER_SIZE);

    if (!mStreamingBuffer)
    {
        ERR("Failed to allocate the streaming vertex buffer.");
    }
}

VertexDataManager::~VertexDataManager()
{
    delete mStreamingBuffer;

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        delete mCurrentValueBuffer[i];
    }
}

GLenum VertexDataManager::prepareVertexData(const gl::VertexAttribute attribs[], const gl::VertexAttribCurrentValueData currentValues[],
                                            gl::ProgramBinary *programBinary, GLint start, GLsizei count, TranslatedAttribute *translated, GLsizei instances)
{
    if (!mStreamingBuffer)
    {
        return GL_OUT_OF_MEMORY;
    }

    for (int attributeIndex = 0; attributeIndex < gl::MAX_VERTEX_ATTRIBS; attributeIndex++)
    {
        translated[attributeIndex].active = (programBinary->getSemanticIndex(attributeIndex) != -1);
    }

    // Invalidate static buffers that don't contain matching attributes
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].mArrayEnabled)
        {
            gl::Buffer *buffer = attribs[i].mBoundBuffer.get();
            StaticVertexBufferInterface *staticBuffer = buffer ? buffer->getStaticVertexBuffer() : NULL;

            if (staticBuffer && staticBuffer->getBufferSize() > 0 && staticBuffer->lookupAttribute(attribs[i]) == -1 &&
                !DirectStoragePossible(staticBuffer, attribs[i], currentValues[i]))
            {
                buffer->invalidateStaticData();
            }
        }
    }

    // Reserve the required space in the buffers
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].mArrayEnabled)
        {
            gl::Buffer *buffer = attribs[i].mBoundBuffer.get();
            StaticVertexBufferInterface *staticBuffer = buffer ? buffer->getStaticVertexBuffer() : NULL;
            VertexBufferInterface *vertexBuffer = staticBuffer ? staticBuffer : static_cast<VertexBufferInterface*>(mStreamingBuffer);

            if (!DirectStoragePossible(vertexBuffer, attribs[i], currentValues[i]))
            {
                if (staticBuffer)
                {
                    if (staticBuffer->getBufferSize() == 0)
                    {
                        int totalCount = ElementsInBuffer(attribs[i], buffer->size());
                        if (!staticBuffer->reserveVertexSpace(attribs[i], totalCount, 0))
                        {
                            return GL_OUT_OF_MEMORY;
                        }
                    }
                }
                else
                {
                    if (!mStreamingBuffer->reserveVertexSpace(attribs[i], count, instances))
                    {
                        return GL_OUT_OF_MEMORY;
                    }
                }
            }
        }
    }

    // Perform the vertex data translations
    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active)
        {
            if (attribs[i].mArrayEnabled)
            {
                gl::Buffer *buffer = attribs[i].mBoundBuffer.get();

                if (!buffer && attribs[i].mPointer == NULL)
                {
                    // This is an application error that would normally result in a crash, but we catch it and return an error
                    ERR("An enabled vertex array has no buffer and no pointer.");
                    return GL_INVALID_OPERATION;
                }

                StaticVertexBufferInterface *staticBuffer = buffer ? buffer->getStaticVertexBuffer() : NULL;
                VertexBufferInterface *vertexBuffer = staticBuffer ? staticBuffer : static_cast<VertexBufferInterface*>(mStreamingBuffer);

                BufferStorage *storage = buffer ? buffer->getStorage() : NULL;
                bool directStorage = DirectStoragePossible(vertexBuffer, attribs[i], currentValues[i]);

                std::size_t streamOffset = -1;
                unsigned int outputElementSize = 0;

                if (directStorage)
                {
                    outputElementSize = attribs[i].stride();
                    streamOffset = attribs[i].mOffset + outputElementSize * start;
                }
                else if (staticBuffer)
                {
                    streamOffset = staticBuffer->lookupAttribute(attribs[i]);
                    outputElementSize = staticBuffer->getVertexBuffer()->getSpaceRequired(attribs[i], 1, 0);

                    if (streamOffset == -1)
                    {
                        // Convert the entire buffer
                        int totalCount = ElementsInBuffer(attribs[i], storage->getSize());
                        int startIndex = attribs[i].mOffset / attribs[i].stride();

                        streamOffset = staticBuffer->storeVertexAttributes(attribs[i], currentValues[i], -startIndex, totalCount, 0);
                    }

                    if (streamOffset != -1)
                    {
                        streamOffset += (attribs[i].mOffset / attribs[i].stride()) * outputElementSize;

                        if (instances == 0 || attribs[i].mDivisor == 0)
                        {
                            streamOffset += start * outputElementSize;
                        }
                    }
                }
                else
                {
                    outputElementSize = mStreamingBuffer->getVertexBuffer()->getSpaceRequired(attribs[i], 1, 0);
                    streamOffset = mStreamingBuffer->storeVertexAttributes(attribs[i], currentValues[i], start, count, instances);
                }

                if (streamOffset == -1)
                {
                    return GL_OUT_OF_MEMORY;
                }

                translated[i].storage = directStorage ? storage : NULL;
                translated[i].vertexBuffer = vertexBuffer->getVertexBuffer();
                translated[i].serial = directStorage ? storage->getSerial() : vertexBuffer->getSerial();
                translated[i].divisor = attribs[i].mDivisor;

                translated[i].attribute = &attribs[i];
                translated[i].currentValueType = currentValues[i].Type;
                translated[i].stride = outputElementSize;
                translated[i].offset = streamOffset;
            }
            else
            {
                if (!mCurrentValueBuffer[i])
                {
                    mCurrentValueBuffer[i] = new StreamingVertexBufferInterface(mRenderer, CONSTANT_VERTEX_BUFFER_SIZE);
                }

                StreamingVertexBufferInterface *buffer = mCurrentValueBuffer[i];

                if (memcmp(&mCurrentValue[i], &currentValues[i], sizeof(gl::VertexAttribCurrentValueData)) != 0)
                {
                    if (!buffer->reserveVertexSpace(attribs[i], 1, 0))
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    int streamOffset = buffer->storeVertexAttributes(attribs[i], currentValues[i], 0, 1, 0);
                    if (streamOffset == -1)
                    {
                        return GL_OUT_OF_MEMORY;
                    }

                    mCurrentValueOffsets[i] = streamOffset;
                }

                translated[i].storage = NULL;
                translated[i].vertexBuffer = mCurrentValueBuffer[i]->getVertexBuffer();
                translated[i].serial = mCurrentValueBuffer[i]->getSerial();
                translated[i].divisor = 0;

                translated[i].attribute = &attribs[i];
                translated[i].currentValueType = currentValues[i].Type;
                translated[i].stride = 0;
                translated[i].offset = mCurrentValueOffsets[i];
            }
        }
    }

    for (int i = 0; i < gl::MAX_VERTEX_ATTRIBS; i++)
    {
        if (translated[i].active && attribs[i].mArrayEnabled)
        {
            gl::Buffer *buffer = attribs[i].mBoundBuffer.get();

            if (buffer)
            {
                buffer->promoteStaticUsage(count * attribs[i].typeSize());
            }
        }
    }

    return GL_NO_ERROR;
}

}
