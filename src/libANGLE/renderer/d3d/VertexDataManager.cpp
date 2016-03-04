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

VertexStorageType ClassifyAttributeStorage(const gl::VertexAttribute &attrib)
{
    // If attribute is disabled, we use the current value.
    if (!attrib.enabled)
    {
        return VertexStorageType::CURRENT_VALUE;
    }

    // If specified with immediate data, we must use dynamic storage.
    auto *buffer = attrib.buffer.get();
    if (!buffer)
    {
        return VertexStorageType::DYNAMIC;
    }

    // Check if the buffer supports direct storage.
    if (DirectStoragePossible(attrib))
    {
        return VertexStorageType::DIRECT;
    }

    // Otherwise the storage is static or dynamic.
    BufferD3D *bufferD3D = GetImplAs<BufferD3D>(buffer);
    ASSERT(bufferD3D);
    switch (bufferD3D->getUsage())
    {
        case D3DBufferUsage::DYNAMIC:
            return VertexStorageType::DYNAMIC;
        case D3DBufferUsage::STATIC:
            return VertexStorageType::STATIC;
        default:
            UNREACHABLE();
            return VertexStorageType::UNKNOWN;
    }
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
    ASSERT(mStreamingBuffer);

    const gl::VertexArray *vertexArray = state.getVertexArray();
    const auto &vertexAttributes       = vertexArray->getVertexAttributes();

    mDynamicAttributeIndexesCache.clear();
    const gl::Program *program = state.getProgram();

    translatedAttribs->clear();

    for (size_t attribIndex = 0; attribIndex < vertexAttributes.size(); ++attribIndex)
    {
        // Skip attrib locations the program doesn't use.
        if (!program->isAttribLocationActive(attribIndex))
            continue;

        const auto &attrib = vertexAttributes[attribIndex];

        // Resize automatically puts in empty attribs
        translatedAttribs->resize(attribIndex + 1);

        TranslatedAttribute *translated = &(*translatedAttribs)[attribIndex];
        auto currentValueData =
            state.getVertexAttribCurrentValue(static_cast<unsigned int>(attribIndex));

        // Record the attribute now
        translated->active           = true;
        translated->attribute        = &attrib;
        translated->currentValueType = currentValueData.Type;
        translated->divisor          = attrib.divisor;

        switch (ClassifyAttributeStorage(attrib))
        {
            case VertexStorageType::STATIC:
            {
                // Store static attribute.
                gl::Error error = StoreStaticAttrib(translated, start, count, instances);
                if (error.isError())
                {
                    return error;
                }
                break;
            }
            case VertexStorageType::DYNAMIC:
                // Dynamic attributes must be handled together.
                mDynamicAttributeIndexesCache.push_back(attribIndex);
                break;
            case VertexStorageType::DIRECT:
                // Update translated data for direct attributes.
                StoreDirectAttrib(translated, start);
                break;
            case VertexStorageType::CURRENT_VALUE:
            {
                gl::Error error = storeCurrentValue(currentValueData, translated, attribIndex);
                if (error.isError())
                {
                    return error;
                }
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
    }

    if (mDynamicAttributeIndexesCache.empty())
    {
        gl::Error(GL_NO_ERROR);
    }

    return storeDynamicAttribs(translatedAttribs, mDynamicAttributeIndexesCache, start, count,
                               instances);
}

// static
void VertexDataManager::StoreDirectAttrib(TranslatedAttribute *directAttrib, GLint start)
{
    const auto &attrib   = *directAttrib->attribute;
    gl::Buffer *buffer   = attrib.buffer.get();
    BufferD3D *bufferD3D = buffer ? GetImplAs<BufferD3D>(buffer) : nullptr;

    // Instanced vertices do not apply the 'start' offset
    GLint firstVertexIndex = (attrib.divisor > 0 ? 0 : start);

    ASSERT(DirectStoragePossible(attrib));
    directAttrib->vertexBuffer.set(nullptr);
    directAttrib->storage = bufferD3D;
    directAttrib->serial  = bufferD3D->getSerial();
    directAttrib->stride = static_cast<unsigned int>(ComputeVertexAttributeStride(attrib));
    directAttrib->offset =
        static_cast<unsigned int>(attrib.offset + directAttrib->stride * firstVertexIndex);
}

// static
gl::Error VertexDataManager::StoreStaticAttrib(TranslatedAttribute *translated,
                                               GLint start,
                                               GLsizei count,
                                               GLsizei instances)
{
    const gl::VertexAttribute &attrib = *translated->attribute;

    gl::Buffer *buffer = attrib.buffer.get();
    ASSERT(buffer && attrib.enabled && !DirectStoragePossible(attrib));
    BufferD3D *bufferD3D = GetImplAs<BufferD3D>(buffer);

    // Instanced vertices do not apply the 'start' offset
    GLint firstVertexIndex = (attrib.divisor > 0 ? 0 : start);

    // Compute source data pointer
    const uint8_t *sourceData = nullptr;

    gl::Error error = bufferD3D->getData(&sourceData);
    if (error.isError())
    {
        return error;
    }
    sourceData += static_cast<int>(attrib.offset);

    unsigned int streamOffset = 0;

    auto errorOrOutputElementSize = bufferD3D->getFactory()->getVertexSpaceRequired(attrib, 1, 0);
    if (errorOrOutputElementSize.isError())
    {
        return errorOrOutputElementSize.getError();
    }

    translated->storage = nullptr;
    translated->stride  = errorOrOutputElementSize.getResult();

    auto *staticBuffer = bufferD3D->getStaticVertexBuffer(attrib);
    ASSERT(staticBuffer);

    if (staticBuffer->empty())
    {
        // Convert the entire buffer
        int totalCount = ElementsInBuffer(attrib, static_cast<unsigned int>(bufferD3D->getSize()));
        int startIndex = static_cast<int>(attrib.offset) /
                         static_cast<int>(ComputeVertexAttributeStride(attrib));

        error = staticBuffer->storeStaticAttribute(attrib, -startIndex, totalCount, 0, sourceData);
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

    VertexBuffer *vertexBuffer = staticBuffer->getVertexBuffer();

    translated->vertexBuffer.set(vertexBuffer);
    translated->serial = vertexBuffer->getSerial();
    translated->offset = streamOffset + firstElementOffset + startOffset;

    return gl::Error(GL_NO_ERROR);
}

gl::Error VertexDataManager::storeDynamicAttribs(
    std::vector<TranslatedAttribute> *translatedAttribs,
    const std::vector<size_t> &dynamicAttribIndexes,
    GLint start,
    GLsizei count,
    GLsizei instances)
{
    // Reserve the required space for the dynamic buffers.
    for (size_t attribIndex : dynamicAttribIndexes)
    {
        const auto &dynamicAttrib = (*translatedAttribs)[attribIndex];
        gl::Error error = reserveSpaceForAttrib(dynamicAttrib, count, instances);
        if (error.isError())
        {
            return error;
        }
    }

    // Store dynamic attributes
    for (size_t attribIndex : dynamicAttribIndexes)
    {
        auto *dynamicAttrib = &(*translatedAttribs)[attribIndex];
        gl::Error error = storeDynamicAttrib(dynamicAttrib, start, count, instances);
        if (error.isError())
        {
            unmapStreamingBuffer();
            return error;
        }

        // Promote static usage of dynamic buffers.
        gl::Buffer *buffer = dynamicAttrib->attribute->buffer.get();
        if (buffer)
        {
            BufferD3D *bufferD3D = GetImplAs<BufferD3D>(buffer);
            size_t typeSize = ComputeVertexAttributeTypeSize(*dynamicAttrib->attribute);
            bufferD3D->promoteStaticUsage(count * static_cast<int>(typeSize));
        }
    }

    unmapStreamingBuffer();
    return gl::Error(GL_NO_ERROR);
}

gl::Error VertexDataManager::reserveSpaceForAttrib(const TranslatedAttribute &translatedAttrib,
                                                   GLsizei count,
                                                   GLsizei instances) const
{
    const gl::VertexAttribute &attrib = *translatedAttrib.attribute;
    ASSERT(!DirectStoragePossible(attrib));

    gl::Buffer *buffer   = attrib.buffer.get();
    BufferD3D *bufferD3D = buffer ? GetImplAs<BufferD3D>(buffer) : nullptr;
    ASSERT(!bufferD3D || bufferD3D->getStaticVertexBuffer(attrib) == nullptr);
    UNUSED_ASSERTION_VARIABLE(bufferD3D);

    size_t totalCount = ComputeVertexAttributeElementCount(attrib, count, instances);
    ASSERT(!bufferD3D ||
           ElementsInBuffer(attrib, static_cast<unsigned int>(bufferD3D->getSize())) >=
               static_cast<int>(totalCount));

    return mStreamingBuffer->reserveVertexSpace(attrib, static_cast<GLsizei>(totalCount),
                                                instances);
}

gl::Error VertexDataManager::storeDynamicAttrib(TranslatedAttribute *translated,
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

    size_t totalCount = ComputeVertexAttributeElementCount(attrib, count, instances);

    gl::Error error = mStreamingBuffer->storeDynamicAttribute(
        attrib, translated->currentValueType, firstVertexIndex, static_cast<GLsizei>(totalCount),
        instances, &streamOffset, sourceData);
    if (error.isError())
    {
        return error;
    }

    VertexBuffer *vertexBuffer = mStreamingBuffer->getVertexBuffer();

    translated->vertexBuffer.set(vertexBuffer);
    translated->serial = vertexBuffer->getSerial();
    translated->offset = streamOffset;

    return gl::Error(GL_NO_ERROR);
}

gl::Error VertexDataManager::storeCurrentValue(const gl::VertexAttribCurrentValueData &currentValue,
                                               TranslatedAttribute *translated,
                                               size_t attribIndex)
{
    CurrentValueState *cachedState = &mCurrentValueCache[attribIndex];
    auto *&buffer                  = cachedState->buffer;

    if (!buffer)
    {
        buffer = new StreamingVertexBufferInterface(mFactory, CONSTANT_VERTEX_BUFFER_SIZE);
    }

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
