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
constexpr size_t kDynamicVertexDataSize = 1024 * 1024;
constexpr size_t kDynamicIndexDataSize  = 1024 * 8;
}  // anonymous namespace

VertexArrayVk::VertexArrayVk(const gl::VertexArrayState &state, RendererVk *renderer)
    : VertexArrayImpl(state),
      mCurrentArrayBufferHandles{},
      mCurrentArrayBufferOffsets{},
      mCurrentArrayBufferResources{},
      mCurrentElementArrayBufferHandle(VK_NULL_HANDLE),
      mCurrentElementArrayBufferOffset(0),
      mCurrentElementArrayBufferResource(nullptr),
      mDynamicVertexData(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, kDynamicVertexDataSize),
      mDynamicIndexData(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, kDynamicIndexDataSize),
      mTranslatedByteIndexData(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, kDynamicIndexDataSize),
      mLineLoopHelper(renderer),
      mDirtyLineLoopTranslation(true),
      mVertexBuffersDirty(false),
      mIndexBufferDirty(false),
      mLastIndexBufferOffset(0)
{
    mCurrentArrayBufferHandles.fill(VK_NULL_HANDLE);
    mCurrentArrayBufferOffsets.fill(0);
    mCurrentArrayBufferResources.fill(nullptr);

    mPackedInputBindings.fill({0, 0});
    mPackedInputAttributes.fill({0, 0, 0});

    mDynamicVertexData.init(1, renderer);
    mDynamicIndexData.init(1, renderer);
    mTranslatedByteIndexData.init(1, renderer);
}

VertexArrayVk::~VertexArrayVk()
{
}

void VertexArrayVk::destroy(const gl::Context *context)
{
    VkDevice device = vk::GetImpl(context)->getRenderer()->getDevice();
    mDynamicVertexData.destroy(device);
    mDynamicIndexData.destroy(device);
    mTranslatedByteIndexData.destroy(device);
    mLineLoopHelper.destroy(device);
}

gl::Error VertexArrayVk::streamVertexData(RendererVk *renderer,
                                          const gl::AttributesMask &attribsToStream,
                                          const gl::DrawCallParams &drawCallParams)
{
    ASSERT(!attribsToStream.none());

    const auto &attribs  = mState.getVertexAttributes();
    const auto &bindings = mState.getVertexBindings();

    const size_t lastVertex = drawCallParams.firstVertex() + drawCallParams.vertexCount();

    // TODO(fjhenigman): When we have a bunch of interleaved attributes, they end up
    // un-interleaved, wasting space and copying time.  Consider improving on that.
    for (size_t attribIndex : attribsToStream)
    {
        const gl::VertexAttribute &attrib = attribs[attribIndex];
        const gl::VertexBinding &binding  = bindings[attrib.bindingIndex];
        ASSERT(attrib.enabled && binding.getBuffer().get() == nullptr);

        // TODO(fjhenigman): Work with more formats than just GL_FLOAT.
        if (attrib.type != GL_FLOAT)
        {
            UNIMPLEMENTED();
            return gl::InternalError();
        }

        // Only [firstVertex, lastVertex] is needed by the upcoming draw so that
        // is all we copy, but we allocate space for [0, lastVertex] so indexing
        // will work.  If we don't start at zero all the indices will be off.
        // TODO(fjhenigman): See if we can account for indices being off by adjusting
        // the offset, thus avoiding wasted memory.
        const size_t firstByte = drawCallParams.firstVertex() * binding.getStride();
        const size_t lastByte =
            lastVertex * binding.getStride() + gl::ComputeVertexAttributeTypeSize(attrib);
        uint8_t *dst    = nullptr;
        uint32_t offset = 0;
        ANGLE_TRY(mDynamicVertexData.allocate(
            renderer, lastByte, &dst, &mCurrentArrayBufferHandles[attribIndex], &offset, nullptr));
        mCurrentArrayBufferOffsets[attribIndex] = static_cast<VkDeviceSize>(offset);
        memcpy(dst + firstByte, static_cast<const uint8_t *>(attrib.pointer) + firstByte,
               lastByte - firstByte);
    }

    ANGLE_TRY(mDynamicVertexData.flush(renderer->getDevice()));
    mDynamicVertexData.releaseRetainedBuffers(renderer);
    return gl::NoError();
}

gl::Error VertexArrayVk::streamIndexData(RendererVk *renderer,
                                         const gl::DrawCallParams &drawCallParams)
{
    ASSERT(!mState.getElementArrayBuffer().get());

    uint32_t offset = 0;

    const GLsizei amount = sizeof(GLushort) * drawCallParams.indexCount();
    GLubyte *dst         = nullptr;

    ANGLE_TRY(mDynamicIndexData.allocate(renderer, amount, &dst, &mCurrentElementArrayBufferHandle,
                                         &offset, nullptr));
    if (drawCallParams.type() == GL_UNSIGNED_BYTE)
    {
        // Unsigned bytes don't have direct support in Vulkan so we have to expand the
        // memory to a GLushort.
        const GLubyte *in     = static_cast<const GLubyte *>(drawCallParams.indices());
        GLushort *expandedDst = reinterpret_cast<GLushort *>(dst);
        for (GLsizei index = 0; index < drawCallParams.indexCount(); index++)
        {
            expandedDst[index] = static_cast<GLushort>(in[index]);
        }
    }
    else
    {
        memcpy(dst, drawCallParams.indices(), amount);
    }
    ANGLE_TRY(mDynamicIndexData.flush(renderer->getDevice()));
    mDynamicIndexData.releaseRetainedBuffers(renderer);
    mCurrentElementArrayBufferOffset = offset;
    return gl::NoError();
}

#define ANGLE_VERTEX_DIRTY_ATTRIB_FUNC(INDEX)                                          \
    case gl::VertexArray::DIRTY_BIT_ATTRIB_0 + INDEX:                                  \
        syncDirtyAttrib(attribs[INDEX], bindings[attribs[INDEX].bindingIndex], INDEX); \
        invalidatePipeline = true;                                                     \
        break;

#define ANGLE_VERTEX_DIRTY_BINDING_FUNC(INDEX)                                         \
    case gl::VertexArray::DIRTY_BIT_BINDING_0 + INDEX:                                 \
        syncDirtyAttrib(attribs[INDEX], bindings[attribs[INDEX].bindingIndex], INDEX); \
        invalidatePipeline = true;                                                     \
        break;

#define ANGLE_VERTEX_DIRTY_BUFFER_DATA_FUNC(INDEX)         \
    case gl::VertexArray::DIRTY_BIT_BUFFER_DATA_0 + INDEX: \
        break;

gl::Error VertexArrayVk::syncState(const gl::Context *context,
                                   const gl::VertexArray::DirtyBits &dirtyBits,
                                   const gl::VertexArray::DirtyAttribBitsArray &attribBits,
                                   const gl::VertexArray::DirtyBindingBitsArray &bindingBits)
{
    ASSERT(dirtyBits.any());

    bool invalidatePipeline = false;

    // Invalidate current pipeline.
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
                gl::Buffer *bufferGL = mState.getElementArrayBuffer().get();
                if (bufferGL)
                {
                    BufferVk *bufferVk                 = vk::GetImpl(bufferGL);
                    mCurrentElementArrayBufferResource = bufferVk;
                    mCurrentElementArrayBufferHandle   = bufferVk->getVkBuffer().getHandle();
                }
                else
                {
                    mCurrentElementArrayBufferResource = nullptr;
                    mCurrentElementArrayBufferHandle   = VK_NULL_HANDLE;
                }

                mCurrentElementArrayBufferOffset = 0;
                mLineLoopBufferFirstIndex.reset();
                mLineLoopBufferLastIndex.reset();
                mIndexBufferDirty         = true;
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

    if (invalidatePipeline)
    {
        mVertexBuffersDirty = true;
        contextVk->invalidateCurrentPipeline();
    }

    return gl::NoError();
}

void VertexArrayVk::syncDirtyAttrib(const gl::VertexAttribute &attrib,
                                    const gl::VertexBinding &binding,
                                    size_t attribIndex)
{
    // Invalidate the input description for pipelines.
    mDirtyPackedInputs.set(attribIndex);

    if (attrib.enabled)
    {
        gl::Buffer *bufferGL = binding.getBuffer().get();

        if (bufferGL)
        {
            BufferVk *bufferVk                        = vk::GetImpl(bufferGL);
            mCurrentArrayBufferResources[attribIndex] = bufferVk;
            mCurrentArrayBufferHandles[attribIndex]   = bufferVk->getVkBuffer().getHandle();
        }
        else
        {
            mCurrentArrayBufferResources[attribIndex] = nullptr;
            mCurrentArrayBufferHandles[attribIndex]   = VK_NULL_HANDLE;
        }
        // TODO(jmadill): Offset handling.  Assume zero for now.
        mCurrentArrayBufferOffsets[attribIndex] = 0;
    }
    else
    {
        UNIMPLEMENTED();
    }
}

const gl::AttribArray<VkBuffer> &VertexArrayVk::getCurrentArrayBufferHandles() const
{
    return mCurrentArrayBufferHandles;
}

const gl::AttribArray<VkDeviceSize> &VertexArrayVk::getCurrentArrayBufferOffsets() const
{
    return mCurrentArrayBufferOffsets;
}

void VertexArrayVk::updateArrayBufferReadDependencies(vk::CommandGraphResource *drawFramebuffer,
                                                      const gl::AttributesMask &activeAttribsMask,
                                                      Serial serial)
{
    // Handle the bound array buffers.
    for (size_t attribIndex : activeAttribsMask)
    {
        if (mCurrentArrayBufferResources[attribIndex])
            mCurrentArrayBufferResources[attribIndex]->addReadDependency(drawFramebuffer);
    }
}

void VertexArrayVk::updateElementArrayBufferReadDependency(
    vk::CommandGraphResource *drawFramebuffer,
    Serial serial)
{
    // Handle the bound element array buffer.
    if (mCurrentElementArrayBufferResource)
    {
        mCurrentElementArrayBufferResource->addReadDependency(drawFramebuffer);
    }
}

void VertexArrayVk::getPackedInputDescriptions(const RendererVk *rendererVk,
                                               vk::PipelineDesc *pipelineDesc)
{
    updatePackedInputDescriptions(rendererVk);
    pipelineDesc->updateVertexInputInfo(mPackedInputBindings, mPackedInputAttributes);
}

void VertexArrayVk::updatePackedInputDescriptions(const RendererVk *rendererVk)
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
            updatePackedInputInfo(rendererVk, static_cast<uint32_t>(attribIndex), binding, attrib);
        }
        else
        {
            UNIMPLEMENTED();
        }
    }

    mDirtyPackedInputs.reset();
}

void VertexArrayVk::updatePackedInputInfo(const RendererVk *rendererVk,
                                          uint32_t attribIndex,
                                          const gl::VertexBinding &binding,
                                          const gl::VertexAttribute &attrib)
{
    vk::PackedVertexInputBindingDesc &bindingDesc = mPackedInputBindings[attribIndex];

    size_t attribSize = gl::ComputeVertexAttributeTypeSize(attrib);
    ASSERT(attribSize <= std::numeric_limits<uint16_t>::max());

    bindingDesc.stride    = static_cast<uint16_t>(binding.getStride());
    bindingDesc.inputRate = static_cast<uint16_t>(
        binding.getDivisor() > 0 ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX);

    VkFormat vkFormat = rendererVk->getFormat(GetVertexFormatID(attrib)).vkBufferFormat;
    ASSERT(vkFormat <= std::numeric_limits<uint16_t>::max());

    vk::PackedVertexInputAttributeDesc &attribDesc = mPackedInputAttributes[attribIndex];
    attribDesc.format                              = static_cast<uint16_t>(vkFormat);
    attribDesc.location                            = static_cast<uint16_t>(attribIndex);
    attribDesc.offset = static_cast<uint32_t>(ComputeVertexAttributeOffset(attrib, binding));
}

gl::Error VertexArrayVk::drawArrays(const gl::Context *context,
                                    RendererVk *renderer,
                                    const gl::DrawCallParams &drawCallParams,
                                    vk::CommandBuffer *commandBuffer,
                                    bool newCommandBuffer)
{
    ASSERT(commandBuffer->valid());

    ANGLE_TRY(onDraw(context, renderer, drawCallParams, commandBuffer, newCommandBuffer));

    // Note: Vertex indexes can be arbitrarily large.
    uint32_t clampedVertexCount = drawCallParams.getClampedVertexCount<uint32_t>();

    if (drawCallParams.mode() != gl::PrimitiveMode::LineLoop)
    {
        commandBuffer->draw(clampedVertexCount, 1, drawCallParams.firstVertex(), 0);
        return gl::NoError();
    }

    // Handle GL_LINE_LOOP drawArrays.
    size_t lastVertex = static_cast<size_t>(drawCallParams.firstVertex() + clampedVertexCount);
    if (!mLineLoopBufferFirstIndex.valid() || !mLineLoopBufferLastIndex.valid() ||
        mLineLoopBufferFirstIndex != drawCallParams.firstVertex() ||
        mLineLoopBufferLastIndex != lastVertex)
    {
        ANGLE_TRY(mLineLoopHelper.getIndexBufferForDrawArrays(renderer, drawCallParams,
                                                              &mCurrentElementArrayBufferHandle,
                                                              &mCurrentElementArrayBufferOffset));

        mLineLoopBufferFirstIndex = drawCallParams.firstVertex();
        mLineLoopBufferLastIndex  = lastVertex;
    }

    commandBuffer->bindIndexBuffer(mCurrentElementArrayBufferHandle,
                                   mCurrentElementArrayBufferOffset, VK_INDEX_TYPE_UINT32);

    vk::LineLoopHelper::Draw(clampedVertexCount, commandBuffer);

    return gl::NoError();
}

gl::Error VertexArrayVk::drawElements(const gl::Context *context,
                                      RendererVk *renderer,
                                      const gl::DrawCallParams &drawCallParams,
                                      vk::CommandBuffer *commandBuffer,
                                      bool newCommandBuffer)
{
    ASSERT(commandBuffer->valid());

    if (drawCallParams.mode() != gl::PrimitiveMode::LineLoop)
    {
        ANGLE_TRY(
            onIndexedDraw(context, renderer, drawCallParams, commandBuffer, newCommandBuffer));
        commandBuffer->drawIndexed(drawCallParams.indexCount(), 1, 0, 0, 0);
        return gl::NoError();
    }

    // Handle GL_LINE_LOOP drawElements.
    if (mDirtyLineLoopTranslation)
    {
        gl::Buffer *elementArrayBuffer = mState.getElementArrayBuffer().get();
        VkIndexType indexType          = gl_vk::GetIndexType(drawCallParams.type());

        if (!elementArrayBuffer)
        {
            ANGLE_TRY(mLineLoopHelper.getIndexBufferForClientElementArray(
                renderer, drawCallParams, &mCurrentElementArrayBufferHandle,
                &mCurrentElementArrayBufferOffset));
        }
        else
        {
            // When using an element array buffer, 'indices' is an offset to the first element.
            intptr_t offset                = reinterpret_cast<intptr_t>(drawCallParams.indices());
            BufferVk *elementArrayBufferVk = vk::GetImpl(elementArrayBuffer);
            ANGLE_TRY(mLineLoopHelper.getIndexBufferForElementArrayBuffer(
                renderer, elementArrayBufferVk, indexType, drawCallParams.indexCount(), offset,
                &mCurrentElementArrayBufferHandle, &mCurrentElementArrayBufferOffset));
        }
    }

    ANGLE_TRY(onIndexedDraw(context, renderer, drawCallParams, commandBuffer, newCommandBuffer));
    vk::LineLoopHelper::Draw(drawCallParams.indexCount(), commandBuffer);

    return gl::NoError();
}

gl::Error VertexArrayVk::onDraw(const gl::Context *context,
                                RendererVk *renderer,
                                const gl::DrawCallParams &drawCallParams,
                                vk::CommandBuffer *commandBuffer,
                                bool newCommandBuffer)
{
    const gl::State &state                  = context->getGLState();
    const gl::Program *programGL            = state.getProgram();
    const gl::AttributesMask &clientAttribs = mState.getEnabledClientMemoryAttribsMask();
    const gl::AttributesMask &activeAttribs = programGL->getActiveAttribLocationsMask();
    uint32_t maxAttrib                      = programGL->getState().getMaxActiveAttribLocation();

    if (clientAttribs.any())
    {
        const gl::AttributesMask &attribsToStream = (clientAttribs & activeAttribs);
        if (attribsToStream.any())
        {
            ANGLE_TRY(drawCallParams.ensureIndexRangeResolved(context));
            ANGLE_TRY(streamVertexData(renderer, attribsToStream, drawCallParams));
            commandBuffer->bindVertexBuffers(0, maxAttrib, mCurrentArrayBufferHandles.data(),
                                             mCurrentArrayBufferOffsets.data());
        }
    }
    else if (mVertexBuffersDirty || newCommandBuffer)
    {
        if (maxAttrib > 0)
        {
            commandBuffer->bindVertexBuffers(0, maxAttrib, mCurrentArrayBufferHandles.data(),
                                             mCurrentArrayBufferOffsets.data());

            vk::CommandGraphResource *drawFramebuffer = vk::GetImpl(state.getDrawFramebuffer());
            updateArrayBufferReadDependencies(drawFramebuffer, activeAttribs,
                                              renderer->getCurrentQueueSerial());
        }

        mVertexBuffersDirty = false;

        // This forces the binding to happen if we follow a drawElement call from a drawArrays call.
        mIndexBufferDirty = true;

        // If we've had a drawElements call with a line loop before, we want to make sure this is
        // invalidated the next time drawElements is called since we use the same index buffer for
        // both calls.
        mDirtyLineLoopTranslation = true;
    }

    return gl::NoError();
}

gl::Error VertexArrayVk::onIndexedDraw(const gl::Context *context,
                                       RendererVk *renderer,
                                       const gl::DrawCallParams &drawCallParams,
                                       vk::CommandBuffer *commandBuffer,
                                       bool newCommandBuffer)
{
    ANGLE_TRY(onDraw(context, renderer, drawCallParams, commandBuffer, newCommandBuffer));
    bool isLineLoop  = drawCallParams.mode() == gl::PrimitiveMode::LineLoop;
    gl::Buffer *glBuffer = mState.getElementArrayBuffer().get();
    uintptr_t offset =
        glBuffer && !isLineLoop ? reinterpret_cast<uintptr_t>(drawCallParams.indices()) : 0;

    if (!glBuffer && !isLineLoop)
    {
        ANGLE_TRY(drawCallParams.ensureIndexRangeResolved(context));
        ANGLE_TRY(streamIndexData(renderer, drawCallParams));
        commandBuffer->bindIndexBuffer(mCurrentElementArrayBufferHandle,
                                       mCurrentElementArrayBufferOffset,
                                       gl_vk::GetIndexType(drawCallParams.type()));
    }
    else if (mIndexBufferDirty || newCommandBuffer || offset != mLastIndexBufferOffset)
    {
        if (drawCallParams.type() == GL_UNSIGNED_BYTE &&
            drawCallParams.mode() != gl::PrimitiveMode::LineLoop)
        {
            // Unsigned bytes don't have direct support in Vulkan so we have to expand the
            // memory to a GLushort.
            BufferVk *bufferVk   = vk::GetImpl(glBuffer);
            void *srcDataMapping = nullptr;
            ASSERT(!glBuffer->isMapped());
            ANGLE_TRY(bufferVk->map(context, 0, &srcDataMapping));
            uint8_t *srcData           = reinterpret_cast<uint8_t *>(srcDataMapping);
            intptr_t offsetIntoSrcData = reinterpret_cast<intptr_t>(drawCallParams.indices());
            srcData += offsetIntoSrcData;

            // Allocate a new buffer that's double the size of the buffer provided by the user to
            // go from unsigned byte to unsigned short.
            uint8_t *allocatedData      = nullptr;
            bool newBufferAllocated     = false;
            uint32_t expandedDataOffset = 0;
            mTranslatedByteIndexData.allocate(
                renderer, static_cast<size_t>(bufferVk->getSize()) * 2, &allocatedData,
                &mCurrentElementArrayBufferHandle, &expandedDataOffset, &newBufferAllocated);
            mCurrentElementArrayBufferOffset = static_cast<VkDeviceSize>(expandedDataOffset);

            // Expand the source into the destination
            ASSERT(!context->getGLState().isPrimitiveRestartEnabled());
            uint16_t *expandedDst = reinterpret_cast<uint16_t *>(allocatedData);
            for (GLsizei index = 0; index < bufferVk->getSize() - offsetIntoSrcData; index++)
            {
                expandedDst[index] = static_cast<GLushort>(srcData[index]);
            }

            // Make sure our writes are available.
            mTranslatedByteIndexData.flush(renderer->getDevice());
            GLboolean result = false;
            ANGLE_TRY(bufferVk->unmap(context, &result));

            // We do not add the offset from the drawCallParams here because we've already copied
            // the source starting at the offset requested.
            commandBuffer->bindIndexBuffer(mCurrentElementArrayBufferHandle,
                                           mCurrentElementArrayBufferOffset,
                                           gl_vk::GetIndexType(drawCallParams.type()));
        }
        else
        {
            commandBuffer->bindIndexBuffer(mCurrentElementArrayBufferHandle,
                                           mCurrentElementArrayBufferOffset + offset,
                                           gl_vk::GetIndexType(drawCallParams.type()));
        }

        mLastIndexBufferOffset = offset;

        const gl::State &glState                  = context->getGLState();
        vk::CommandGraphResource *drawFramebuffer = vk::GetImpl(glState.getDrawFramebuffer());
        updateElementArrayBufferReadDependency(drawFramebuffer, renderer->getCurrentQueueSerial());
        mIndexBufferDirty = false;

        // If we've had a drawArrays call with a line loop before, we want to make sure this is
        // invalidated the next time drawArrays is called since we use the same index buffer for
        // both calls.
        mLineLoopBufferFirstIndex.reset();
        mLineLoopBufferLastIndex.reset();
    }

    return gl::NoError();
}
}  // namespace rx
