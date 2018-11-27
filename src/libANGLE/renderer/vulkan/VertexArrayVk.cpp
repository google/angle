//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VertexArrayVk.cpp:
//    Implements the class methods for VertexArrayVk.
//

#include "libANGLE/renderer/vulkan/VertexArrayVk.h"

#include "common/debug.h"
#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"

namespace rx
{
namespace
{
constexpr size_t kDynamicVertexDataSize    = 1024 * 1024;
constexpr size_t kDynamicIndexDataSize     = 1024 * 8;
constexpr size_t kMaxVertexFormatAlignment = 4;

bool BindingIsAligned(const gl::VertexBinding &binding, unsigned componentSize)
{
    return (binding.getOffset() % componentSize == 0) && (binding.getStride() % componentSize == 0);
}

angle::Result StreamVertexData(ContextVk *contextVk,
                               vk::DynamicBuffer *dynamicBuffer,
                               const uint8_t *sourceData,
                               size_t bytesToAllocate,
                               size_t destOffset,
                               size_t vertexCount,
                               size_t stride,
                               VertexCopyFunction vertexLoadFunction,
                               VkBuffer *bufferHandleOut,
                               VkDeviceSize *bufferOffsetOut)
{
    uint8_t *dst = nullptr;
    ANGLE_TRY(dynamicBuffer->allocate(contextVk, bytesToAllocate, &dst, bufferHandleOut,
                                      bufferOffsetOut, nullptr));
    dst += destOffset;
    vertexLoadFunction(sourceData, stride, vertexCount, dst);

    ANGLE_TRY(dynamicBuffer->flush(contextVk));
    return angle::Result::Continue();
}

}  // anonymous namespace

#define INIT                                        \
    {                                               \
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 1024 * 8 \
    }

VertexArrayVk::VertexArrayVk(const gl::VertexArrayState &state, RendererVk *renderer)
    : VertexArrayImpl(state),
      mCurrentArrayBufferHandles{},
      mCurrentArrayBufferOffsets{},
      mCurrentArrayBuffers{},
      mCurrentArrayBufferFormats{},
      mCurrentArrayBufferStrides{},
      mCurrentArrayBufferConversion{{
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
          INIT,
      }},
      mCurrentArrayBufferConversionCanRelease{},
      mCurrentElementArrayBufferHandle(VK_NULL_HANDLE),
      mCurrentElementArrayBufferOffset(0),
      mCurrentElementArrayBuffer(nullptr),
      mPackedInputBindings{},
      mPackedInputAttributes{},
      mDynamicVertexData(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, kDynamicVertexDataSize),
      mDynamicIndexData(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, kDynamicIndexDataSize),
      mTranslatedByteIndexData(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, kDynamicIndexDataSize),
      mLineLoopHelper(renderer),
      mDirtyLineLoopTranslation(true)
{
    mCurrentArrayBufferHandles.fill(VK_NULL_HANDLE);
    mCurrentArrayBufferOffsets.fill(0);
    mCurrentArrayBuffers.fill(nullptr);

    for (vk::DynamicBuffer &buffer : mCurrentArrayBufferConversion)
    {
        buffer.init(kMaxVertexFormatAlignment, renderer);
    }
    mDynamicVertexData.init(kMaxVertexFormatAlignment, renderer);
    mDynamicIndexData.init(1, renderer);
    mTranslatedByteIndexData.init(1, renderer);

    // Initially consider all inputs dirty.
    mDirtyPackedInputs.set();
}

VertexArrayVk::~VertexArrayVk() {}

void VertexArrayVk::destroy(const gl::Context *context)
{
    RendererVk *renderer = vk::GetImpl(context)->getRenderer();

    for (vk::DynamicBuffer &buffer : mCurrentArrayBufferConversion)
    {
        buffer.release(renderer);
    }
    mDynamicVertexData.release(renderer);
    mDynamicIndexData.release(renderer);
    mTranslatedByteIndexData.release(renderer);
    mLineLoopHelper.release(renderer);
}

angle::Result VertexArrayVk::streamIndexData(ContextVk *contextVk,
                                             GLenum indexType,
                                             size_t indexCount,
                                             const void *sourcePointer,
                                             vk::DynamicBuffer *dynamicBuffer)
{
    ASSERT(!mState.getElementArrayBuffer() || indexType == GL_UNSIGNED_BYTE);

    dynamicBuffer->releaseRetainedBuffers(contextVk->getRenderer());

    const size_t amount = sizeof(GLushort) * indexCount;
    GLubyte *dst        = nullptr;

    ANGLE_TRY(dynamicBuffer->allocate(contextVk, amount, &dst, &mCurrentElementArrayBufferHandle,
                                      &mCurrentElementArrayBufferOffset, nullptr));
    if (indexType == GL_UNSIGNED_BYTE)
    {
        // Unsigned bytes don't have direct support in Vulkan so we have to expand the
        // memory to a GLushort.
        const GLubyte *in     = static_cast<const GLubyte *>(sourcePointer);
        GLushort *expandedDst = reinterpret_cast<GLushort *>(dst);
        for (size_t index = 0; index < indexCount; index++)
        {
            expandedDst[index] = static_cast<GLushort>(in[index]);
        }
    }
    else
    {
        memcpy(dst, sourcePointer, amount);
    }
    ANGLE_TRY(dynamicBuffer->flush(contextVk));
    return angle::Result::Continue();
}

// We assume the buffer is completely full of the same kind of data and convert
// and/or align it as we copy it to a DynamicBuffer. The assumption could be wrong
// but the alternative of copying it piecemeal on each draw would have a lot more
// overhead.
angle::Result VertexArrayVk::convertVertexBuffer(ContextVk *contextVk,
                                                 BufferVk *srcBuffer,
                                                 const gl::VertexBinding &binding,
                                                 size_t attribIndex)
{

    // Needed before reading buffer or we could get stale data.
    ANGLE_TRY(contextVk->getRenderer()->finish(contextVk));

    unsigned srcFormatSize = mCurrentArrayBufferFormats[attribIndex]->angleFormat().pixelBytes;
    unsigned dstFormatSize = mCurrentArrayBufferStrides[attribIndex];

    mCurrentArrayBufferConversion[attribIndex].releaseRetainedBuffers(contextVk->getRenderer());

    // Bytes usable for vertex data.
    GLint64 bytes = srcBuffer->getSize() - binding.getOffset();
    if (bytes < srcFormatSize)
        return angle::Result::Continue();

    // Count the last vertex.  It may occupy less than a full stride.
    size_t numVertices = 1;
    bytes -= srcFormatSize;

    // Count how many strides fit remaining space.
    if (bytes > 0)
        numVertices += static_cast<size_t>(bytes) / binding.getStride();

    void *src = nullptr;
    ANGLE_TRY(srcBuffer->mapImpl(contextVk, &src));
    const uint8_t *srcBytes = reinterpret_cast<const uint8_t *>(src);
    srcBytes += binding.getOffset();
    ASSERT(GetVertexInputAlignment(*mCurrentArrayBufferFormats[attribIndex]) <=
           kMaxVertexFormatAlignment);
    ANGLE_TRY(StreamVertexData(contextVk, &mCurrentArrayBufferConversion[attribIndex], srcBytes,
                               numVertices * dstFormatSize, 0, numVertices, binding.getStride(),
                               mCurrentArrayBufferFormats[attribIndex]->vertexLoadFunction,
                               &mCurrentArrayBufferHandles[attribIndex],
                               &mCurrentArrayBufferOffsets[attribIndex]));
    ANGLE_TRY(srcBuffer->unmapImpl(contextVk));

    mCurrentArrayBufferConversionCanRelease[attribIndex] = true;

    return angle::Result::Continue();
}

void VertexArrayVk::ensureConversionReleased(RendererVk *renderer, size_t attribIndex)
{
    if (mCurrentArrayBufferConversionCanRelease[attribIndex])
    {
        mCurrentArrayBufferConversion[attribIndex].release(renderer);
        mCurrentArrayBufferConversionCanRelease[attribIndex] = false;
    }
}

#define ANGLE_VERTEX_DIRTY_ATTRIB_FUNC(INDEX)                                     \
    case gl::VertexArray::DIRTY_BIT_ATTRIB_0 + INDEX:                             \
        ANGLE_TRY(syncDirtyAttrib(contextVk, attribs[INDEX],                      \
                                  bindings[attribs[INDEX].bindingIndex], INDEX)); \
        invalidateContext = true;                                                 \
        break;

#define ANGLE_VERTEX_DIRTY_BINDING_FUNC(INDEX)                                    \
    case gl::VertexArray::DIRTY_BIT_BINDING_0 + INDEX:                            \
        ANGLE_TRY(syncDirtyAttrib(contextVk, attribs[INDEX],                      \
                                  bindings[attribs[INDEX].bindingIndex], INDEX)); \
        invalidateContext = true;                                                 \
        break;

#define ANGLE_VERTEX_DIRTY_BUFFER_DATA_FUNC(INDEX)                                \
    case gl::VertexArray::DIRTY_BIT_BUFFER_DATA_0 + INDEX:                        \
        ANGLE_TRY(syncDirtyAttrib(contextVk, attribs[INDEX],                      \
                                  bindings[attribs[INDEX].bindingIndex], INDEX)); \
        break;

angle::Result VertexArrayVk::syncState(const gl::Context *context,
                                       const gl::VertexArray::DirtyBits &dirtyBits,
                                       const gl::VertexArray::DirtyAttribBitsArray &attribBits,
                                       const gl::VertexArray::DirtyBindingBitsArray &bindingBits)
{
    ASSERT(dirtyBits.any());

    bool invalidateContext = false;

    ContextVk *contextVk = vk::GetImpl(context);

    // Rebuild current attribute buffers cache. This will fail horribly if the buffer changes.
    // TODO(jmadill): Handle buffer storage changes.
    const auto &attribs  = mState.getVertexAttributes();
    const auto &bindings = mState.getVertexBindings();

    for (size_t dirtyBit : dirtyBits)
    {
        switch (dirtyBit)
        {
            case gl::VertexArray::DIRTY_BIT_ELEMENT_ARRAY_BUFFER:
            {
                gl::Buffer *bufferGL = mState.getElementArrayBuffer();
                if (bufferGL)
                {
                    BufferVk *bufferVk         = vk::GetImpl(bufferGL);
                    mCurrentElementArrayBuffer = &bufferVk->getBuffer();
                    mCurrentElementArrayBufferHandle =
                        bufferVk->getBuffer().getBuffer().getHandle();
                }
                else
                {
                    mCurrentElementArrayBuffer       = nullptr;
                    mCurrentElementArrayBufferHandle = VK_NULL_HANDLE;
                }

                mCurrentElementArrayBufferOffset = 0;
                mLineLoopBufferFirstIndex.reset();
                mLineLoopBufferLastIndex.reset();
                contextVk->setIndexBufferDirty();
                mDirtyLineLoopTranslation = true;
                break;
            }

            case gl::VertexArray::DIRTY_BIT_ELEMENT_ARRAY_BUFFER_DATA:
                mLineLoopBufferFirstIndex.reset();
                mLineLoopBufferLastIndex.reset();
                mDirtyLineLoopTranslation = true;
                break;

                ANGLE_VERTEX_INDEX_CASES(ANGLE_VERTEX_DIRTY_ATTRIB_FUNC);
                ANGLE_VERTEX_INDEX_CASES(ANGLE_VERTEX_DIRTY_BINDING_FUNC);
                ANGLE_VERTEX_INDEX_CASES(ANGLE_VERTEX_DIRTY_BUFFER_DATA_FUNC);

            default:
                UNREACHABLE();
                break;
        }
    }

    if (invalidateContext)
    {
        contextVk->invalidateVertexAndIndexBuffers();
    }

    return angle::Result::Continue();
}

angle::Result VertexArrayVk::syncDirtyAttrib(ContextVk *contextVk,
                                             const gl::VertexAttribute &attrib,
                                             const gl::VertexBinding &binding,
                                             size_t attribIndex)
{
    // Invalidate the input description for pipelines.
    mDirtyPackedInputs.set(attribIndex);

    RendererVk *renderer   = contextVk->getRenderer();
    bool releaseConversion = true;

    if (attrib.enabled)
    {
        gl::Buffer *bufferGL                    = binding.getBuffer().get();
        mCurrentArrayBufferFormats[attribIndex] = &renderer->getFormat(GetVertexFormatID(attrib));

        if (bufferGL)
        {
            BufferVk *bufferVk = vk::GetImpl(bufferGL);
            unsigned componentSize =
                mCurrentArrayBufferFormats[attribIndex]->angleFormat().pixelBytes / attrib.size;

            if (mCurrentArrayBufferFormats[attribIndex]->vertexLoadRequiresConversion ||
                !BindingIsAligned(binding, componentSize))
            {
                mCurrentArrayBufferStrides[attribIndex] =
                    mCurrentArrayBufferFormats[attribIndex]->bufferFormat().pixelBytes;

                ANGLE_TRY(convertVertexBuffer(contextVk, bufferVk, binding, attribIndex));

                mCurrentArrayBuffers[attribIndex] = nullptr;
                releaseConversion                 = false;
            }
            else
            {
                mCurrentArrayBuffers[attribIndex] = &bufferVk->getBuffer();
                mCurrentArrayBufferHandles[attribIndex] =
                    bufferVk->getBuffer().getBuffer().getHandle();
                mCurrentArrayBufferOffsets[attribIndex] = binding.getOffset();
                mCurrentArrayBufferStrides[attribIndex] = binding.getStride();
            }
        }
        else
        {
            mCurrentArrayBuffers[attribIndex]       = nullptr;
            mCurrentArrayBufferHandles[attribIndex] = VK_NULL_HANDLE;
            mCurrentArrayBufferOffsets[attribIndex] = 0;
            mCurrentArrayBufferStrides[attribIndex] =
                mCurrentArrayBufferFormats[attribIndex]->bufferFormat().pixelBytes;
        }
    }
    else
    {
        contextVk->invalidateDefaultAttribute(attribIndex);

        // These will be filled out by the ContextVk.
        mCurrentArrayBuffers[attribIndex]       = nullptr;
        mCurrentArrayBufferHandles[attribIndex] = VK_NULL_HANDLE;
        mCurrentArrayBufferOffsets[attribIndex] = 0;
        mCurrentArrayBufferStrides[attribIndex] = 0;
        mCurrentArrayBufferFormats[attribIndex] =
            &renderer->getFormat(angle::FormatID::R32G32B32A32_FLOAT);
    }

    if (releaseConversion)
        ensureConversionReleased(renderer, attribIndex);

    return angle::Result::Continue();
}

void VertexArrayVk::getPackedInputDescriptions(vk::GraphicsPipelineDesc *pipelineDesc)
{
    updatePackedInputDescriptions();
    pipelineDesc->updateVertexInputInfo(mPackedInputBindings, mPackedInputAttributes);
}

void VertexArrayVk::updatePackedInputDescriptions()
{
    if (!mDirtyPackedInputs.any())
    {
        return;
    }

    const auto &attribs  = mState.getVertexAttributes();
    const auto &bindings = mState.getVertexBindings();

    for (auto attribIndex : mDirtyPackedInputs)
    {
        const auto &attrib  = attribs[attribIndex];
        const auto &binding = bindings[attrib.bindingIndex];
        if (attrib.enabled)
        {
            updatePackedInputInfo(static_cast<uint32_t>(attribIndex), binding, attrib);
        }
        else
        {
            vk::PackedVertexInputBindingDesc &bindingDesc = mPackedInputBindings[attribIndex];
            bindingDesc.stride                            = 0;
            bindingDesc.inputRate                         = VK_VERTEX_INPUT_RATE_VERTEX;

            mPackedInputAttributes.formats[attribIndex] =
                static_cast<uint16_t>(VK_FORMAT_R32G32B32A32_SFLOAT);
            mPackedInputAttributes.offsets[attribIndex] = 0;
        }
    }

    mDirtyPackedInputs.reset();
}

void VertexArrayVk::updatePackedInputInfo(uint32_t attribIndex,
                                          const gl::VertexBinding &binding,
                                          const gl::VertexAttribute &attrib)
{
    vk::PackedVertexInputBindingDesc &bindingDesc = mPackedInputBindings[attribIndex];

    bindingDesc.stride    = static_cast<uint16_t>(mCurrentArrayBufferStrides[attribIndex]);
    bindingDesc.inputRate = static_cast<uint16_t>(
        binding.getDivisor() > 0 ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX);

    VkFormat vkFormat = mCurrentArrayBufferFormats[attribIndex]->vkBufferFormat;
    ASSERT(vkFormat <= std::numeric_limits<uint16_t>::max());
    if (vkFormat == VK_FORMAT_UNDEFINED)
    {
        UNIMPLEMENTED();
    }

    mPackedInputAttributes.formats[attribIndex] = static_cast<uint8_t>(vkFormat);
    mPackedInputAttributes.offsets[attribIndex] = static_cast<uint16_t>(attrib.relativeOffset);
}

angle::Result VertexArrayVk::updateClientAttribs(const gl::Context *context,
                                                 GLint firstVertex,
                                                 GLsizei vertexOrIndexCount,
                                                 GLenum indexTypeOrNone,
                                                 const void *indices)
{
    ContextVk *contextVk                    = vk::GetImpl(context);
    const gl::AttributesMask &clientAttribs = context->getStateCache().getActiveClientAttribsMask();

    ASSERT(clientAttribs.any());

    GLint startVertex;
    size_t vertexCount;
    ANGLE_TRY(GetVertexRangeInfo(context, firstVertex, vertexOrIndexCount, indexTypeOrNone, indices,
                                 0, &startVertex, &vertexCount));

    mDynamicVertexData.releaseRetainedBuffers(contextVk->getRenderer());

    const auto &attribs  = mState.getVertexAttributes();
    const auto &bindings = mState.getVertexBindings();

    // TODO(fjhenigman): When we have a bunch of interleaved attributes, they end up
    // un-interleaved, wasting space and copying time.  Consider improving on that.
    for (size_t attribIndex : clientAttribs)
    {
        const gl::VertexAttribute &attrib = attribs[attribIndex];
        const gl::VertexBinding &binding  = bindings[attrib.bindingIndex];
        ASSERT(attrib.enabled && binding.getBuffer().get() == nullptr);

        const size_t bytesToAllocate =
            (startVertex + vertexCount) * mCurrentArrayBufferStrides[attribIndex];
        const uint8_t *src =
            static_cast<const uint8_t *>(attrib.pointer) + startVertex * binding.getStride();

        size_t destOffset = startVertex * mCurrentArrayBufferStrides[attribIndex];
        ASSERT(GetVertexInputAlignment(*mCurrentArrayBufferFormats[attribIndex]) <=
               kMaxVertexFormatAlignment);

        // Only vertexCount() vertices will be used by the upcoming draw. so that is all we copy.
        // We allocate space for startVertex + vertexCount so indexing will work.  If we
        // don't start at zero all the indices will be off.
        // TODO(fjhenigman): See if we can account for indices being off by adjusting the
        // offset, thus avoiding wasted memory.
        ANGLE_TRY(StreamVertexData(
            contextVk, &mDynamicVertexData, src, bytesToAllocate, destOffset, vertexCount,
            binding.getStride(), mCurrentArrayBufferFormats[attribIndex]->vertexLoadFunction,
            &mCurrentArrayBufferHandles[attribIndex], &mCurrentArrayBufferOffsets[attribIndex]));
    }

    return angle::Result::Continue();
}

angle::Result VertexArrayVk::handleLineLoop(ContextVk *contextVk,
                                            GLint firstVertex,
                                            GLsizei vertexOrIndexCount,
                                            GLenum indexTypeOrNone,
                                            const void *indices)
{
    if (indexTypeOrNone != GL_NONE)
    {
        // Handle GL_LINE_LOOP drawElements.
        if (mDirtyLineLoopTranslation)
        {
            gl::Buffer *elementArrayBuffer = mState.getElementArrayBuffer();

            if (!elementArrayBuffer)
            {
                ANGLE_TRY(mLineLoopHelper.streamIndices(
                    contextVk, indexTypeOrNone, vertexOrIndexCount,
                    reinterpret_cast<const uint8_t *>(indices), &mCurrentElementArrayBufferHandle,
                    &mCurrentElementArrayBufferOffset));
            }
            else
            {
                // When using an element array buffer, 'indices' is an offset to the first element.
                intptr_t offset                = reinterpret_cast<intptr_t>(indices);
                BufferVk *elementArrayBufferVk = vk::GetImpl(elementArrayBuffer);
                ANGLE_TRY(mLineLoopHelper.getIndexBufferForElementArrayBuffer(
                    contextVk, elementArrayBufferVk, indexTypeOrNone, vertexOrIndexCount, offset,
                    &mCurrentElementArrayBufferHandle, &mCurrentElementArrayBufferOffset));
            }
        }

        // If we've had a drawArrays call with a line loop before, we want to make sure this is
        // invalidated the next time drawArrays is called since we use the same index buffer for
        // both calls.
        mLineLoopBufferFirstIndex.reset();
        mLineLoopBufferLastIndex.reset();
        return angle::Result::Continue();
    }

    // Note: Vertex indexes can be arbitrarily large.
    uint32_t clampedVertexCount = gl::clampCast<uint32_t>(vertexOrIndexCount);

    // Handle GL_LINE_LOOP drawArrays.
    size_t lastVertex = static_cast<size_t>(firstVertex + clampedVertexCount);
    if (!mLineLoopBufferFirstIndex.valid() || !mLineLoopBufferLastIndex.valid() ||
        mLineLoopBufferFirstIndex != firstVertex || mLineLoopBufferLastIndex != lastVertex)
    {
        ANGLE_TRY(mLineLoopHelper.getIndexBufferForDrawArrays(
            contextVk, clampedVertexCount, firstVertex, &mCurrentElementArrayBufferHandle,
            &mCurrentElementArrayBufferOffset));

        mLineLoopBufferFirstIndex = firstVertex;
        mLineLoopBufferLastIndex  = lastVertex;
    }

    return angle::Result::Continue();
}

angle::Result VertexArrayVk::updateIndexTranslation(ContextVk *contextVk,
                                                    GLsizei indexCount,
                                                    GLenum type,
                                                    const void *indices)
{
    ASSERT(type != GL_NONE);

    gl::Buffer *glBuffer = mState.getElementArrayBuffer();

    if (!glBuffer)
    {
        ANGLE_TRY(streamIndexData(contextVk, type, indexCount, indices, &mDynamicIndexData));
    }
    else
    {
        // Needed before reading buffer or we could get stale data.
        ANGLE_TRY(contextVk->getRenderer()->finish(contextVk));

        ASSERT(type == GL_UNSIGNED_BYTE);
        // Unsigned bytes don't have direct support in Vulkan so we have to expand the
        // memory to a GLushort.
        BufferVk *bufferVk   = vk::GetImpl(glBuffer);
        void *srcDataMapping = nullptr;
        ASSERT(!glBuffer->isMapped());
        ANGLE_TRY(bufferVk->mapImpl(contextVk, &srcDataMapping));
        uint8_t *srcData           = static_cast<uint8_t *>(srcDataMapping);
        intptr_t offsetIntoSrcData = reinterpret_cast<intptr_t>(indices);
        srcData += offsetIntoSrcData;

        ANGLE_TRY(streamIndexData(contextVk, type,
                                  static_cast<size_t>(bufferVk->getSize()) - offsetIntoSrcData,
                                  srcData, &mTranslatedByteIndexData));

        ANGLE_TRY(bufferVk->unmapImpl(contextVk));
    }

    return angle::Result::Continue();
}

void VertexArrayVk::updateDefaultAttrib(RendererVk *renderer,
                                        size_t attribIndex,
                                        VkBuffer bufferHandle,
                                        uint32_t offset)
{
    if (!mState.getEnabledAttributesMask().test(attribIndex))
    {
        mCurrentArrayBufferHandles[attribIndex] = bufferHandle;
        mCurrentArrayBufferOffsets[attribIndex] = offset;
        mCurrentArrayBuffers[attribIndex]       = nullptr;
        mCurrentArrayBufferStrides[attribIndex] = 0;
        mCurrentArrayBufferFormats[attribIndex] =
            &renderer->getFormat(angle::FormatID::R32G32B32A32_FIXED);
        mDirtyPackedInputs.set(attribIndex);
    }
}
}  // namespace rx
