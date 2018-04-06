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
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"

namespace rx
{
namespace
{
constexpr size_t kDynamicVertexDataSize = 1024 * 1024;
constexpr size_t kDynamicIndexDataSize  = 1024 * 8;
}  // anonymous namespace

VertexArrayVk::VertexArrayVk(const gl::VertexArrayState &state)
    : VertexArrayImpl(state),
      mCurrentArrayBufferHandles{},
      mCurrentArrayBufferOffsets{},
      mCurrentArrayBufferResources{},
      mCurrentElementArrayBufferHandle(VK_NULL_HANDLE),
      mCurrentElementArrayBufferOffset(0),
      mCurrentElementArrayBufferResource(nullptr),
      mDynamicVertexData(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, kDynamicVertexDataSize),
      mDynamicIndexData(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, kDynamicIndexDataSize),
      mDirtyLineLoopTranslation(true),
      mVertexBuffersDirty(false),
      mIndexBufferDirty(false)
{
    mCurrentArrayBufferHandles.fill(VK_NULL_HANDLE);
    mCurrentArrayBufferOffsets.fill(0);
    mCurrentArrayBufferResources.fill(nullptr);

    mPackedInputBindings.fill({0, 0});
    mPackedInputAttributes.fill({0, 0, 0});

    mDynamicVertexData.init(1);
    mDynamicIndexData.init(1);
}

VertexArrayVk::~VertexArrayVk()
{
}

void VertexArrayVk::destroy(const gl::Context *context)
{
    VkDevice device = vk::GetImpl(context)->getRenderer()->getDevice();

    mDynamicVertexData.destroy(device);
    mDynamicIndexData.destroy(device);
    mLineLoopHandler.destroy(device);
}

gl::Error VertexArrayVk::streamVertexData(RendererVk *renderer,
                                          const gl::AttributesMask &attribsToStream,
                                          const gl::DrawCallParams &drawCallParams)
{
    ASSERT(!attribsToStream.none());

    const auto &attribs          = mState.getVertexAttributes();
    const auto &bindings         = mState.getVertexBindings();

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
        uint8_t *dst = nullptr;
        uint32_t offset = 0;
        ANGLE_TRY(mDynamicVertexData.allocate(
            renderer, lastByte, &dst, &mCurrentArrayBufferHandles[attribIndex], &offset, nullptr));
        mCurrentArrayBufferOffsets[attribIndex] = static_cast<VkDeviceSize>(offset);
        memcpy(dst + firstByte, static_cast<const uint8_t *>(attrib.pointer) + firstByte,
               lastByte - firstByte);
    }

    ANGLE_TRY(mDynamicVertexData.flush(renderer->getDevice()));
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

    mCurrentElementArrayBufferOffset = offset;
    return gl::NoError();
}

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
                    mCurrentElementArrayBufferOffset   = 0;
                }
                else
                {
                    mCurrentElementArrayBufferResource = nullptr;
                    mCurrentElementArrayBufferHandle   = VK_NULL_HANDLE;
                    mCurrentElementArrayBufferOffset   = 0;
                }
                mIndexBufferDirty         = true;
                mDirtyLineLoopTranslation = true;
                break;
            }

            case gl::VertexArray::DIRTY_BIT_ELEMENT_ARRAY_BUFFER_DATA:
                mLineLoopBufferFirstIndex.reset();
                mLineLoopBufferLastIndex.reset();
                mDirtyLineLoopTranslation = true;
                break;

            default:
            {
                size_t attribIndex = gl::VertexArray::GetVertexIndexFromDirtyBit(dirtyBit);

                // Invalidate the input description for pipelines.
                mDirtyPackedInputs.set(attribIndex);

                const auto &attrib  = attribs[attribIndex];
                const auto &binding = bindings[attrib.bindingIndex];

                if (attrib.enabled)
                {
                    gl::Buffer *bufferGL = binding.getBuffer().get();

                    if (bufferGL)
                    {
                        BufferVk *bufferVk                        = vk::GetImpl(bufferGL);
                        mCurrentArrayBufferResources[attribIndex] = bufferVk;
                        mCurrentArrayBufferHandles[attribIndex] =
                            bufferVk->getVkBuffer().getHandle();
                        mClientMemoryAttribs.reset(attribIndex);
                    }
                    else
                    {
                        mCurrentArrayBufferResources[attribIndex] = nullptr;
                        mCurrentArrayBufferHandles[attribIndex]   = VK_NULL_HANDLE;
                        mClientMemoryAttribs.set(attribIndex);
                    }
                    // TODO(jmadill): Offset handling.  Assume zero for now.
                    mCurrentArrayBufferOffsets[attribIndex] = 0;
                }
                else
                {
                    mClientMemoryAttribs.reset(attribIndex);
                    UNIMPLEMENTED();
                }

                invalidatePipeline = true;
                break;
            }
        }
    }

    if (invalidatePipeline)
    {
        mVertexBuffersDirty = true;
        contextVk->invalidateCurrentPipeline();
    }

    return gl::NoError();
}

const gl::AttribArray<VkBuffer> &VertexArrayVk::getCurrentArrayBufferHandles() const
{
    return mCurrentArrayBufferHandles;
}

const gl::AttribArray<VkDeviceSize> &VertexArrayVk::getCurrentArrayBufferOffsets() const
{
    return mCurrentArrayBufferOffsets;
}

void VertexArrayVk::updateArrayBufferReadDependencies(vk::CommandGraphNode *readingNode,
                                                      const gl::AttributesMask &activeAttribsMask,
                                                      Serial serial)
{
    // Handle the bound array buffers.
    for (size_t attribIndex : activeAttribsMask)
    {
        if (mCurrentArrayBufferResources[attribIndex])
            mCurrentArrayBufferResources[attribIndex]->onReadResource(readingNode, serial);
    }
}

void VertexArrayVk::updateElementArrayBufferReadDependency(vk::CommandGraphNode *readingNode,
                                                           Serial serial)
{
    // Handle the bound element array buffer.
    if (mCurrentElementArrayBufferResource)
    {
        mCurrentElementArrayBufferResource->onReadResource(readingNode, serial);
    }
}

void VertexArrayVk::getPackedInputDescriptions(vk::PipelineDesc *pipelineDesc)
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
            UNIMPLEMENTED();
        }
    }

    mDirtyPackedInputs.reset();
}

void VertexArrayVk::updatePackedInputInfo(uint32_t attribIndex,
                                          const gl::VertexBinding &binding,
                                          const gl::VertexAttribute &attrib)
{
    vk::PackedVertexInputBindingDesc &bindingDesc = mPackedInputBindings[attribIndex];

    size_t attribSize = gl::ComputeVertexAttributeTypeSize(attrib);
    ASSERT(attribSize <= std::numeric_limits<uint16_t>::max());

    bindingDesc.stride    = static_cast<uint16_t>(binding.getStride());
    bindingDesc.inputRate = static_cast<uint16_t>(
        binding.getDivisor() > 0 ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX);

    gl::VertexFormatType vertexFormatType = gl::GetVertexFormatType(attrib);
    VkFormat vkFormat                     = vk::GetNativeVertexFormat(vertexFormatType);
    ASSERT(vkFormat <= std::numeric_limits<uint16_t>::max());

    vk::PackedVertexInputAttributeDesc &attribDesc = mPackedInputAttributes[attribIndex];
    attribDesc.format                              = static_cast<uint16_t>(vkFormat);
    attribDesc.location                            = static_cast<uint16_t>(attribIndex);
    attribDesc.offset = static_cast<uint32_t>(ComputeVertexAttributeOffset(attrib, binding));
}

gl::Error VertexArrayVk::drawArrays(const gl::Context *context,
                                    RendererVk *renderer,
                                    const gl::DrawCallParams &drawCallParams,
                                    vk::CommandGraphNode *drawNode,
                                    bool newCommandBuffer)
{
    vk::CommandBuffer *commandBuffer = drawNode->getInsideRenderPassCommands();
    ASSERT(commandBuffer->valid());

    ANGLE_TRY(onDraw(context, renderer, drawCallParams, drawNode, newCommandBuffer));

    if (drawCallParams.mode() != GL_LINE_LOOP)
    {
        commandBuffer->draw(drawCallParams.vertexCount(), 1, drawCallParams.firstVertex(), 0);
        return gl::NoError();
    }

    // Handle GL_LINE_LOOP drawArrays.
    // This test may be incorrect if the draw call switches from DrawArrays/DrawElements.
    int lastVertex = drawCallParams.firstVertex() + drawCallParams.vertexCount();
    if (!mLineLoopBufferFirstIndex.valid() || !mLineLoopBufferLastIndex.valid() ||
        mLineLoopBufferFirstIndex != drawCallParams.firstVertex() ||
        mLineLoopBufferLastIndex != lastVertex)
    {
        ANGLE_TRY(mLineLoopHandler.createIndexBuffer(renderer, drawCallParams,
                                                     &mCurrentElementArrayBufferHandle,
                                                     &mCurrentElementArrayBufferOffset));

        mLineLoopBufferFirstIndex = drawCallParams.firstVertex();
        mLineLoopBufferLastIndex  = lastVertex;
    }

    commandBuffer->bindIndexBuffer(mCurrentElementArrayBufferHandle,
                                   mCurrentElementArrayBufferOffset, VK_INDEX_TYPE_UINT32);

    vk::LineLoopHandler::Draw(drawCallParams.vertexCount(), commandBuffer);

    return gl::NoError();
}

gl::Error VertexArrayVk::drawElements(const gl::Context *context,
                                      RendererVk *renderer,
                                      const gl::DrawCallParams &drawCallParams,
                                      vk::CommandGraphNode *drawNode,
                                      bool newCommandBuffer)
{
    vk::CommandBuffer *commandBuffer = drawNode->getInsideRenderPassCommands();
    ASSERT(commandBuffer->valid());

    if (drawCallParams.mode() != GL_LINE_LOOP)
    {
        ANGLE_TRY(onIndexedDraw(context, renderer, drawCallParams, drawNode, newCommandBuffer));
        commandBuffer->drawIndexed(drawCallParams.indexCount(), 1, 0, 0, 0);
        return gl::NoError();
    }

    // Handle GL_LINE_LOOP drawElements.
    gl::Buffer *elementArrayBuffer = mState.getElementArrayBuffer().get();
    if (!elementArrayBuffer)
    {
        UNIMPLEMENTED();
        return gl::InternalError() << "Line loop indices in client memory not supported";
    }

    BufferVk *elementArrayBufferVk = vk::GetImpl(elementArrayBuffer);

    VkIndexType indexType = gl_vk::GetIndexType(drawCallParams.type());

    // This also doesn't check if the element type changed, which should trigger translation.
    if (mDirtyLineLoopTranslation)
    {
        ANGLE_TRY(mLineLoopHandler.createIndexBufferFromElementArrayBuffer(
            renderer, elementArrayBufferVk, indexType, drawCallParams.indexCount(),
            &mCurrentElementArrayBufferHandle, &mCurrentElementArrayBufferOffset));
    }

    ANGLE_TRY(onIndexedDraw(context, renderer, drawCallParams, drawNode, newCommandBuffer));
    vk::LineLoopHandler::Draw(drawCallParams.indexCount(), commandBuffer);

    return gl::NoError();
}

gl::Error VertexArrayVk::onDraw(const gl::Context *context,
                                RendererVk *renderer,
                                const gl::DrawCallParams &drawCallParams,
                                vk::CommandGraphNode *drawNode,
                                bool newCommandBuffer)
{
    const gl::State &state                  = context->getGLState();
    const gl::Program *programGL            = state.getProgram();
    const gl::AttributesMask &activeAttribs = programGL->getActiveAttribLocationsMask();
    uint32_t maxAttrib                      = programGL->getState().getMaxActiveAttribLocation();

    if (mClientMemoryAttribs.any())
    {
        const gl::AttributesMask &attribsToStream = (mClientMemoryAttribs & activeAttribs);
        if (attribsToStream.any())
        {
            ANGLE_TRY(drawCallParams.ensureIndexRangeResolved(context));
            ANGLE_TRY(streamVertexData(renderer, attribsToStream, drawCallParams));
            vk::CommandBuffer *commandBuffer = drawNode->getInsideRenderPassCommands();
            commandBuffer->bindVertexBuffers(0, maxAttrib, mCurrentArrayBufferHandles.data(),
                                             mCurrentArrayBufferOffsets.data());
        }
    }
    else if (mVertexBuffersDirty || newCommandBuffer)
    {
        if (maxAttrib > 0)
        {
            vk::CommandBuffer *commandBuffer = drawNode->getInsideRenderPassCommands();
            commandBuffer->bindVertexBuffers(0, maxAttrib, mCurrentArrayBufferHandles.data(),
                                             mCurrentArrayBufferOffsets.data());
            updateArrayBufferReadDependencies(drawNode, activeAttribs,
                                              renderer->getCurrentQueueSerial());
        }
        mVertexBuffersDirty = false;
    }

    return gl::NoError();
}

gl::Error VertexArrayVk::onIndexedDraw(const gl::Context *context,
                                       RendererVk *renderer,
                                       const gl::DrawCallParams &drawCallParams,
                                       vk::CommandGraphNode *drawNode,
                                       bool newCommandBuffer)
{
    ANGLE_TRY(onDraw(context, renderer, drawCallParams, drawNode, newCommandBuffer));

    if (!mState.getElementArrayBuffer().get())
    {
        ANGLE_TRY(drawCallParams.ensureIndexRangeResolved(context));
        ANGLE_TRY(streamIndexData(renderer, drawCallParams));
        vk::CommandBuffer *commandBuffer = drawNode->getInsideRenderPassCommands();
        commandBuffer->bindIndexBuffer(mCurrentElementArrayBufferHandle,
                                       mCurrentElementArrayBufferOffset,
                                       gl_vk::GetIndexType(drawCallParams.type()));
    }
    else if (mIndexBufferDirty || newCommandBuffer)
    {
        if (drawCallParams.type() == GL_UNSIGNED_BYTE)
        {
            // TODO(fjhenigman): Index format translation.
            UNIMPLEMENTED();
            return gl::InternalError()
                   << "Unsigned byte translation is not implemented for indices in a buffer object";
        }

        vk::CommandBuffer *commandBuffer = drawNode->getInsideRenderPassCommands();
        commandBuffer->bindIndexBuffer(mCurrentElementArrayBufferHandle,
                                       mCurrentElementArrayBufferOffset,
                                       gl_vk::GetIndexType(drawCallParams.type()));
        updateElementArrayBufferReadDependency(drawNode, renderer->getCurrentQueueSerial());
        mIndexBufferDirty = false;
    }

    return gl::NoError();
}
}  // namespace rx
