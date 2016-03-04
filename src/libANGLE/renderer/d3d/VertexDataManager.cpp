//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexDataManager.h: Defines the VertexDataManager, a class that
// runs the Buffer translation process.

#include "libANGLE/renderer/d3d/VertexDataManager.h"

#include "libANGLE/Buffer.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/Program.h"
#include "libANGLE/State.h"
#include "libANGLE/VertexAttribute.h"
#include "libANGLE/VertexArray.h"
#include "libANGLE/renderer/d3d/BufferD3D.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/VertexBuffer.h"

namespace rx
{
namespace
{
enum
{
    INITIAL_STREAM_BUFFER_SIZE = 1024 * 1024
};
// This has to be at least 4k or else it fails on ATI cards.
enum
{
    CONSTANT_VERTEX_BUFFER_SIZE = 4096
};

int ElementsInBuffer(const gl::VertexAttribute &attrib, unsigned int size)
{
    // Size cannot be larger than a GLsizei
    if (size > static_cast<unsigned int>(std::numeric_limits<int>::max()))
    {
        size = static_cast<unsigned int>(std::numeric_limits<int>::max());
    }

    GLsizei stride = static_cast<GLsizei>(ComputeVertexAttributeStride(attrib));
    return (size - attrib.offset % stride +
            (stride - static_cast<GLsizei>(ComputeVertexAttributeTypeSize(attrib)))) /
           stride;
}

bool DirectStoragePossible(const gl::VertexAttribute &attrib)
{
    // Current value attribs may not use direct storage.
    if (!attrib.enabled)
    {
        return false;
    }

    gl::Buffer *buffer = attrib.buffer.get();
    if (!buffer)
    {
        return false;
    }

    BufferD3D *bufferD3D = GetImplAs<BufferD3D>(buffer);
    ASSERT(bufferD3D);
    if (!bufferD3D->supportsDirectBinding())
    {
        return false;
    }

    // Alignment restrictions: In D3D, vertex data must be aligned to the format stride, or to a
    // 4-byte boundary, whichever is smaller. (Undocumented, and experimentally confirmed)
    size_t alignment = 4;

    if (attrib.type != GL_FLOAT)
    {
        gl::VertexFormatType vertexFormatType = gl::GetVertexFormatType(attrib);

        // TODO(jmadill): add VertexFormatCaps
        BufferFactoryD3D *factory = bufferD3D->getFactory();

        auto errorOrElementSize = factory->getVertexSpaceRequired(attrib, 1, 0);
        if (errorOrElementSize.isError())
        {
            ERR("Unlogged error in DirectStoragePossible.");
            return false;
        }

        alignment = std::min<size_t>(errorOrElementSize.getResult(), 4);

        // CPU-converted vertex data must be converted (naturally).
        if ((factory->getVertexConversionType(vertexFormatType) & VERTEX_CONVERT_CPU) != 0)
        {
            return false;
        }
    }

    // Final alignment check - unaligned data must be converted.
    return (static_cast<size_t>(ComputeVertexAttributeStride(attrib)) % alignment == 0) &&
           (static_cast<size_t>(attrib.offset) % alignment == 0);
}
}  // anonymous namespace

TranslatedAttribute::TranslatedAttribute()
    : active(false),
      attribute(nullptr),
      currentValueType(GL_NONE),
      offset(0),
      stride(0),
      vertexBuffer(),
      storage(nullptr),
      serial(0),
      divisor(0)
{
}

VertexDataManager::CurrentValueState::CurrentValueState()
    : buffer(nullptr),
      offset(0)
{
    data.FloatValues[0] = std::numeric_limits<float>::quiet_NaN();
    data.FloatValues[1] = std::numeric_limits<float>::quiet_NaN();
    data.FloatValues[2] = std::numeric_limits<float>::quiet_NaN();
    data.FloatValues[3] = std::numeric_limits<float>::quiet_NaN();
    data.Type = GL_FLOAT;
}

VertexDataManager::CurrentValueState::~CurrentValueState()
{
    SafeDelete(buffer);
}

VertexDataManager::VertexDataManager(BufferFactoryD3D *factory)
    : mFactory(factory),
      mStreamingBuffer(nullptr),
      // TODO(jmadill): use context caps
      mCurrentValueCache(gl::MAX_VERTEX_ATTRIBS)
{
    mStreamingBuffer = new StreamingVertexBufferInterface(factory, INITIAL_STREAM_BUFFER_SIZE);

    if (!mStreamingBuffer)
    {
        ERR("Failed to allocate the streaming vertex buffer.");
    }

    // TODO(jmadill): use context caps
    mActiveEnabledAttributes.reserve(gl::MAX_VERTEX_ATTRIBS);
    mActiveDisabledAttributes.reserve(gl::MAX_VERTEX_ATTRIBS);
}

VertexDataManager::~VertexDataManager()
{
    SafeDelete(mStreamingBuffer);
}

void VertexDataManager::unmapStreamingBuffer()
{
    mStreamingBuffer->getVertexBuffer()->hintUnmapResource();
}

gl::Error VertexDataManager::prepareVertexData(const gl::State &state,
                                               GLint start,
                                               GLsizei count,
                                               std::vector<TranslatedAttribute> *translatedAttribs,
                                               GLsizei instances)
{
    if (!mStreamingBuffer)
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal streaming vertex buffer is unexpectedly NULL.");
    }

    // Compute active enabled and active disable attributes, for speed.
    // TODO(jmadill): don't recompute if there was no state change
    const gl::VertexArray *vertexArray = state.getVertexArray();
    const gl::Program *program         = state.getProgram();
    const auto &vertexAttributes       = vertexArray->getVertexAttributes();

    mActiveEnabledAttributes.clear();
    mActiveDisabledAttributes.clear();
    translatedAttribs->clear();

    for (size_t attribIndex = 0; attribIndex < vertexAttributes.size(); ++attribIndex)
    {
        if (program->isAttribLocationActive(attribIndex))
        {
            // Resize automatically puts in empty attribs
            translatedAttribs->resize(attribIndex + 1);

            TranslatedAttribute *translated = &(*translatedAttribs)[attribIndex];

            // Record the attribute now
            translated->active = true;
            translated->attribute = &vertexAttributes[attribIndex];
            translated->currentValueType =
                state.getVertexAttribCurrentValue(static_cast<unsigned int>(attribIndex)).Type;
            translated->divisor = vertexAttributes[attribIndex].divisor;

            if (vertexAttributes[attribIndex].enabled)
            {
                mActiveEnabledAttributes.push_back(translated);
            }
            else
            {
                mActiveDisabledAttributes.push_back(attribIndex);
            }
        }
    }

    // Reserve the required space in the buffers
    for (const TranslatedAttribute *activeAttrib : mActiveEnabledAttributes)
    {
        gl::Error error = reserveSpaceForAttrib(*activeAttrib, count, instances);
        if (error.isError())
        {
            return error;
        }
    }

    // Perform the vertex data translations
    for (TranslatedAttribute *activeAttrib : mActiveEnabledAttributes)
    {
        gl::Error error = storeAttribute(activeAttrib, start, count, instances);

        if (error.isError())
        {
            unmapStreamingBuffer();
            return error;
        }
    }

    unmapStreamingBuffer();

    for (size_t attribIndex : mActiveDisabledAttributes)
    {
        if (mCurrentValueCache[attribIndex].buffer == nullptr)
        {
            mCurrentValueCache[attribIndex].buffer = new StreamingVertexBufferInterface(mFactory, CONSTANT_VERTEX_BUFFER_SIZE);
        }

        gl::Error error = storeCurrentValue(
            state.getVertexAttribCurrentValue(static_cast<unsigned int>(attribIndex)),
            &(*translatedAttribs)[attribIndex], &mCurrentValueCache[attribIndex]);
        if (error.isError())
        {
            return error;
        }
    }

    for (const TranslatedAttribute *activeAttrib : mActiveEnabledAttributes)
    {
        gl::Buffer *buffer = activeAttrib->attribute->buffer.get();

        if (buffer)
        {
            BufferD3D *bufferD3D = GetImplAs<BufferD3D>(buffer);
            size_t typeSize = ComputeVertexAttributeTypeSize(*activeAttrib->attribute);
            bufferD3D->promoteStaticUsage(count * static_cast<int>(typeSize));
        }
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error VertexDataManager::reserveSpaceForAttrib(const TranslatedAttribute &translatedAttrib,
                                                   GLsizei count,
                                                   GLsizei instances) const
{
    const gl::VertexAttribute &attrib = *translatedAttrib.attribute;
    if (DirectStoragePossible(attrib))
    {
        return gl::Error(GL_NO_ERROR);
    }

    gl::Buffer *buffer = attrib.buffer.get();
    BufferD3D *bufferD3D = buffer ? GetImplAs<BufferD3D>(buffer) : nullptr;
    StaticVertexBufferInterface *staticBuffer =
        bufferD3D ? bufferD3D->getStaticVertexBuffer(attrib) : nullptr;

    if (staticBuffer)
    {
        return gl::Error(GL_NO_ERROR);
    }

    size_t totalCount = ComputeVertexAttributeElementCount(attrib, count, instances);
    ASSERT(!bufferD3D ||
           ElementsInBuffer(attrib, static_cast<unsigned int>(bufferD3D->getSize())) >=
               static_cast<int>(totalCount));

    return mStreamingBuffer->reserveVertexSpace(attrib, static_cast<GLsizei>(totalCount),
                                                instances);
}

gl::Error VertexDataManager::storeAttribute(TranslatedAttribute *translated,
                                            GLint start,
                                            GLsizei count,
                                            GLsizei instances)
{
    const gl::VertexAttribute &attrib = *translated->attribute;

    gl::Buffer *buffer = attrib.buffer.get();
    ASSERT(buffer || attrib.pointer);
    ASSERT(attrib.enabled);

    BufferD3D *storage = buffer ? GetImplAs<BufferD3D>(buffer) : nullptr;

    // Instanced vertices do not apply the 'start' offset
    GLint firstVertexIndex = (attrib.divisor > 0 ? 0 : start);

    if (DirectStoragePossible(attrib))
    {
        translated->vertexBuffer.set(nullptr);
        translated->storage = storage;
        translated->serial = storage->getSerial();
        translated->stride  = static_cast<unsigned int>(ComputeVertexAttributeStride(attrib));
        translated->offset = static_cast<unsigned int>(attrib.offset + translated->stride * firstVertexIndex);
        return gl::Error(GL_NO_ERROR);
    }

    // Compute source data pointer
    const uint8_t *sourceData = nullptr;

    if (buffer)
    {
        gl::Error error = storage->getData(&sourceData);
        if (error.isError())
        {
            return error;
        }
        sourceData += static_cast<int>(attrib.offset);
    }
    else
    {
        sourceData = static_cast<const uint8_t*>(attrib.pointer);
    }

    unsigned int streamOffset = 0;

    auto errorOrOutputElementSize = mFactory->getVertexSpaceRequired(attrib, 1, 0);
    if (errorOrOutputElementSize.isError())
    {
        return errorOrOutputElementSize.getError();
    }

    translated->storage = nullptr;
    translated->stride  = errorOrOutputElementSize.getResult();

    gl::Error error(GL_NO_ERROR);

    auto *staticBuffer = storage ? storage->getStaticVertexBuffer(attrib) : nullptr;

    if (staticBuffer)
    {
        if (staticBuffer->empty())
        {
            // Convert the entire buffer
            int totalCount =
                ElementsInBuffer(attrib, static_cast<unsigned int>(storage->getSize()));
            int startIndex = static_cast<int>(attrib.offset) /
                             static_cast<int>(ComputeVertexAttributeStride(attrib));

            error =
                staticBuffer->storeStaticAttribute(attrib, -startIndex, totalCount, 0, sourceData);
            if (error.isError())
            {
                return error;
            }
        }

        unsigned int firstElementOffset =
            (static_cast<unsigned int>(attrib.offset) /
             static_cast<unsigned int>(ComputeVertexAttributeStride(attrib))) *
            translated->stride;
        ASSERT(attrib.divisor == 0 || firstVertexIndex == 0);
        unsigned int startOffset = firstVertexIndex * translated->stride;
        if (streamOffset + firstElementOffset + startOffset < streamOffset)
        {
            return gl::Error(GL_OUT_OF_MEMORY);
        }

        streamOffset += firstElementOffset + startOffset;
        translated->vertexBuffer.set(staticBuffer->getVertexBuffer());
    }
    else
    {
        size_t totalCount = ComputeVertexAttributeElementCount(attrib, count, instances);

        error = mStreamingBuffer->storeDynamicAttribute(
            attrib, translated->currentValueType, firstVertexIndex,
            static_cast<GLsizei>(totalCount), instances, &streamOffset, sourceData);
        if (error.isError())
        {
            return error;
        }
        translated->vertexBuffer.set(mStreamingBuffer->getVertexBuffer());
    }

    ASSERT(translated->vertexBuffer.get());
    translated->serial = translated->vertexBuffer.get()->getSerial();
    translated->offset = streamOffset;

    return gl::Error(GL_NO_ERROR);
}

gl::Error VertexDataManager::storeCurrentValue(const gl::VertexAttribCurrentValueData &currentValue,
                                               TranslatedAttribute *translated,
                                               CurrentValueState *cachedState)
{
    auto *buffer = cachedState->buffer;

    if (cachedState->data != currentValue)
    {
        const gl::VertexAttribute &attrib = *translated->attribute;

        gl::Error error = buffer->reserveVertexSpace(attrib, 1, 0);
        if (error.isError())
        {
            return error;
        }

        const uint8_t *sourceData = reinterpret_cast<const uint8_t*>(currentValue.FloatValues);
        unsigned int streamOffset;
        error = buffer->storeDynamicAttribute(attrib, currentValue.Type, 0, 1, 0, &streamOffset,
                                              sourceData);
        if (error.isError())
        {
            return error;
        }

        buffer->getVertexBuffer()->hintUnmapResource();

        cachedState->data = currentValue;
        cachedState->offset = streamOffset;
    }

    translated->vertexBuffer.set(buffer->getVertexBuffer());

    translated->storage = nullptr;
    translated->serial  = buffer->getSerial();
    translated->divisor = 0;
    translated->stride  = 0;
    translated->offset  = static_cast<unsigned int>(cachedState->offset);

    return gl::Error(GL_NO_ERROR);
}

// VertexBufferBinding implementation
VertexBufferBinding::VertexBufferBinding() : mBoundVertexBuffer(nullptr)
{
}

VertexBufferBinding::VertexBufferBinding(const VertexBufferBinding &other)
    : mBoundVertexBuffer(other.mBoundVertexBuffer)
{
    if (mBoundVertexBuffer)
    {
        mBoundVertexBuffer->addRef();
    }
}

VertexBufferBinding::~VertexBufferBinding()
{
    if (mBoundVertexBuffer)
    {
        mBoundVertexBuffer->release();
    }
}

VertexBufferBinding &VertexBufferBinding::operator=(const VertexBufferBinding &other)
{
    mBoundVertexBuffer = other.mBoundVertexBuffer;
    if (mBoundVertexBuffer)
    {
        mBoundVertexBuffer->addRef();
    }
    return *this;
}

void VertexBufferBinding::set(VertexBuffer *vertexBuffer)
{
    if (mBoundVertexBuffer == vertexBuffer)
        return;

    if (mBoundVertexBuffer)
    {
        mBoundVertexBuffer->release();
    }
    if (vertexBuffer)
    {
        vertexBuffer->addRef();
    }

    mBoundVertexBuffer = vertexBuffer;
}

VertexBuffer *VertexBufferBinding::get() const
{
    return mBoundVertexBuffer;
}

}  // namespace rx
