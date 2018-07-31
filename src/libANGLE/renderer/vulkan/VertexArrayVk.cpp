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

bool BindingIsAligned(const gl::VertexBinding &binding, unsigned componentSize)
{
    return (binding.getOffset() % componentSize == 0) && (binding.getStride() % componentSize == 0);
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
      mCurrentArrayBufferResources{},
      mCurrentArrayBufferFormats{},
      mCurrentArrayBufferStrides{},
      mCurrentArrayBufferConversion{{
          INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT,
          INIT,
      }},
      mCurrentArrayBufferConversionCanRelease{},
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

    for (vk::DynamicBuffer &buffer : mCurrentArrayBufferConversion)
    {
        buffer.init(1, renderer);
    }
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
    for (vk::DynamicBuffer &buffer : mCurrentArrayBufferConversion)
    {
        buffer.destroy(device);
    }
    mDynamicVertexData.destroy(device);
    mDynamicIndexData.destroy(device);
    mTranslatedByteIndexData.destroy(device);
    mLineLoopHelper.destroy(device);
}

angle::Result VertexArrayVk::streamVertexData(ContextVk *contextVk,
                                              const gl::AttributesMask &attribsToStream,
                                              const gl::DrawCallParams &drawCallParams)
{
    ASSERT(!attribsToStream.none());

    const auto &attribs  = mState.getVertexAttributes();
    const auto &bindings = mState.getVertexBindings();

    // TODO(fjhenigman): When we have a bunch of interleaved attributes, they end up
    // un-interleaved, wasting space and copying time.  Consider improving on that.
    for (size_t attribIndex : attribsToStream)
    {
        const gl::VertexAttribute &attrib = attribs[attribIndex];
        const gl::VertexBinding &binding  = bindings[attrib.bindingIndex];
        ASSERT(attrib.enabled && binding.getBuffer().get() == nullptr);

        // Only vertexCount() vertices will be used by the upcoming draw so that is
        // all we copy, but we allocate space firstVertex() + vertexCount() so indexing
        // will work.  If we don't start at zero all the indices will be off.
        // TODO(fjhenigman): See if we can account for indices being off by adjusting
        // the offset, thus avoiding wasted memory.
        const size_t bytesAllocated =
            (drawCallParams.firstVertex() + drawCallParams.vertexCount()) *
            mCurrentArrayBufferStrides[attribIndex];
        const uint8_t *src = static_cast<const uint8_t *>(attrib.pointer) +
                             drawCallParams.firstVertex() * binding.getStride();
        uint8_t *dst    = nullptr;
        uint32_t offset = 0;
        ANGLE_TRY(mDynamicVertexData.allocate(contextVk, bytesAllocated, &dst,
                                              &mCurrentArrayBufferHandles[attribIndex], &offset,
                                              nullptr));
        dst += drawCallParams.firstVertex() * mCurrentArrayBufferStrides[attribIndex];
        mCurrentArrayBufferOffsets[attribIndex] = static_cast<VkDeviceSize>(offset);
        mCurrentArrayBufferFormats[attribIndex]->vertexLoadFunction(
            src, binding.getStride(), drawCallParams.vertexCount(), dst);
    }

    ANGLE_TRY(mDynamicVertexData.flush(contextVk));
    mDynamicVertexData.releaseRetainedBuffers(contextVk->getRenderer());
    return angle::Result::Continue();
}

angle::Result VertexArrayVk::streamIndexData(ContextVk *contextVk,
                                             const gl::DrawCallParams &drawCallParams)
{
    ASSERT(!mState.getElementArrayBuffer().get());

    uint32_t offset = 0;

    const GLsizei amount = sizeof(GLushort) * drawCallParams.indexCount();
    GLubyte *dst         = nullptr;

    ANGLE_TRY(mDynamicIndexData.allocate(contextVk, amount, &dst, &mCurrentElementArrayBufferHandle,
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
    ANGLE_TRY(mDynamicIndexData.flush(contextVk));
    mDynamicIndexData.releaseRetainedBuffers(contextVk->getRenderer());
    mCurrentElementArrayBufferOffset = offset;
    return angle::Result::Continue();
}

// We assume the buffer is completely full of the same kind of data and convert
// and/or align it as we copy it to a DynamicBuffer. The assumption could be wrong
// but the alternative of copying it piecemeal on each draw would have a lot more
// overhead.
angle::Result VertexArrayVk::convertVertexBuffer(ContextVk *context,
                                                 BufferVk *srcBuffer,
                                                 const gl::VertexBinding &binding,
                                                 size_t attribIndex)
{

    // Preparation for mapping source buffer.
    ANGLE_TRY(context->getRenderer()->finish(context));

    unsigned srcFormatSize = mCurrentArrayBufferFormats[attribIndex]->angleFormat().pixelBytes;
    unsigned dstFormatSize = mCurrentArrayBufferFormats[attribIndex]->bufferFormat().pixelBytes;

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

    void *src       = nullptr;
    uint8_t *dst    = nullptr;
    uint32_t offset = 0;

    ANGLE_TRY(mCurrentArrayBufferConversion[attribIndex].allocate(
        context, numVertices * dstFormatSize, &dst, &mCurrentArrayBufferHandles[attribIndex],
        &offset, nullptr));
    ANGLE_TRY(srcBuffer->mapImpl(context, &src));
    mCurrentArrayBufferFormats[attribIndex]->vertexLoadFunction(
        static_cast<const uint8_t *>(src) + binding.getOffset(), binding.getStride(), numVertices,
        dst);
    ANGLE_TRY(srcBuffer->unmapImpl(context));
    ANGLE_TRY(mCurrentArrayBufferConversion[attribIndex].flush(context));

    mCurrentArrayBufferConversionCanRelease[attribIndex] = true;
    mCurrentArrayBufferOffsets[attribIndex]              = offset;
    mCurrentArrayBufferStrides[attribIndex]              = dstFormatSize;

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
        invalidatePipeline = true;                                                \
        break;

#define ANGLE_VERTEX_DIRTY_BINDING_FUNC(INDEX)                                    \
    case gl::VertexArray::DIRTY_BIT_BINDING_0 + INDEX:                            \
        ANGLE_TRY(syncDirtyAttrib(contextVk, attribs[INDEX],                      \
                                  bindings[attribs[INDEX].bindingIndex], INDEX)); \
        invalidatePipeline = true;                                                \
        break;

#define ANGLE_VERTEX_DIRTY_BUFFER_DATA_FUNC(INDEX)                                \
    case gl::VertexArray::DIRTY_BIT_BUFFER_DATA_0 + INDEX:                        \
        ANGLE_TRY(syncDirtyAttrib(contextVk, attribs[INDEX],                      \
                                  bindings[attribs[INDEX].bindingIndex], INDEX)); \
        break;

gl::Error VertexArrayVk::syncState(const gl::Context *context,
                                   const gl::VertexArray::DirtyBits &dirtyBits,
                                   const gl::VertexArray::DirtyAttribBitsArray &attribBits,
                                   const gl::VertexArray::DirtyBindingBitsArray &bindingBits)
{
    ASSERT(dirtyBits.any());

    bool invalidatePipeline = false;

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
                ANGLE_TRY(convertVertexBuffer(contextVk, bufferVk, binding, attribIndex));

                mCurrentArrayBufferConversion[attribIndex].releaseRetainedBuffers(renderer);
                mCurrentArrayBufferResources[attribIndex] = nullptr;
                releaseConversion                         = false;
            }
            else
            {
                mCurrentArrayBufferResources[attribIndex] = bufferVk;
                mCurrentArrayBufferHandles[attribIndex]   = bufferVk->getVkBuffer().getHandle();
                mCurrentArrayBufferOffsets[attribIndex]   = binding.getOffset();
                mCurrentArrayBufferStrides[attribIndex]   = binding.getStride();
            }
        }
        else
        {
            mCurrentArrayBufferResources[attribIndex] = nullptr;
            mCurrentArrayBufferHandles[attribIndex]   = VK_NULL_HANDLE;
            mCurrentArrayBufferOffsets[attribIndex]   = 0;
            mCurrentArrayBufferStrides[attribIndex] =
                mCurrentArrayBufferFormats[attribIndex]->bufferFormat().pixelBytes;
        }
    }
    else
    {
        contextVk->invalidateDefaultAttribute(attribIndex);

        // These will be filled out by the ContextVk.
        mCurrentArrayBufferResources[attribIndex] = nullptr;
        mCurrentArrayBufferHandles[attribIndex]   = VK_NULL_HANDLE;
        mCurrentArrayBufferOffsets[attribIndex]   = 0;
        mCurrentArrayBufferStrides[attribIndex]   = 0;
        mCurrentArrayBufferFormats[attribIndex] =
            &renderer->getFormat(angle::FormatID::R32G32B32A32_FLOAT);
    }

    if (releaseConversion)
        ensureConversionReleased(renderer, attribIndex);

    return angle::Result::Continue();
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
            vk::PackedVertexInputBindingDesc &bindingDesc = mPackedInputBindings[attribIndex];
            bindingDesc.stride                            = 0;
            bindingDesc.inputRate                         = VK_VERTEX_INPUT_RATE_VERTEX;

            vk::PackedVertexInputAttributeDesc &attribDesc = mPackedInputAttributes[attribIndex];
            attribDesc.format   = static_cast<uint16_t>(VK_FORMAT_R32G32B32A32_SFLOAT);
            attribDesc.location = static_cast<uint16_t>(attribIndex);
            attribDesc.offset   = 0;
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
        // TODO(fjhenigman): Add support for vertex data format.  anglebug.com/2405
        UNIMPLEMENTED();
    }

    vk::PackedVertexInputAttributeDesc &attribDesc = mPackedInputAttributes[attribIndex];
    attribDesc.format                              = static_cast<uint16_t>(vkFormat);
    attribDesc.location                            = static_cast<uint16_t>(attribIndex);
    attribDesc.offset                              = static_cast<uint32_t>(attrib.relativeOffset);
}

gl::Error VertexArrayVk::drawArrays(const gl::Context *context,
                                    const gl::DrawCallParams &drawCallParams,
                                    vk::CommandBuffer *commandBuffer,
                                    bool newCommandBuffer)
{
    ASSERT(commandBuffer->valid());

    ContextVk *contextVk = vk::GetImpl(context);

    ANGLE_TRY(onDraw(context, drawCallParams, commandBuffer, newCommandBuffer));

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
        ANGLE_TRY(mLineLoopHelper.getIndexBufferForDrawArrays(contextVk, drawCallParams,
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
                                      const gl::DrawCallParams &drawCallParams,
                                      vk::CommandBuffer *commandBuffer,
                                      bool newCommandBuffer)
{
    ASSERT(commandBuffer->valid());

    ContextVk *contextVk = vk::GetImpl(context);

    if (drawCallParams.mode() != gl::PrimitiveMode::LineLoop)
    {
        ANGLE_TRY(onIndexedDraw(context, drawCallParams, commandBuffer, newCommandBuffer));
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
                contextVk, drawCallParams, &mCurrentElementArrayBufferHandle,
                &mCurrentElementArrayBufferOffset));
        }
        else
        {
            // When using an element array buffer, 'indices' is an offset to the first element.
            intptr_t offset                = reinterpret_cast<intptr_t>(drawCallParams.indices());
            BufferVk *elementArrayBufferVk = vk::GetImpl(elementArrayBuffer);
            ANGLE_TRY(mLineLoopHelper.getIndexBufferForElementArrayBuffer(
                contextVk, elementArrayBufferVk, indexType, drawCallParams.indexCount(), offset,
                &mCurrentElementArrayBufferHandle, &mCurrentElementArrayBufferOffset));
        }
    }

    ANGLE_TRY(onIndexedDraw(context, drawCallParams, commandBuffer, newCommandBuffer));
    vk::LineLoopHelper::Draw(drawCallParams.indexCount(), commandBuffer);

    return gl::NoError();
}

gl::Error VertexArrayVk::onDraw(const gl::Context *context,
                                const gl::DrawCallParams &drawCallParams,
                                vk::CommandBuffer *commandBuffer,
                                bool newCommandBuffer)
{
    ContextVk *contextVk                    = vk::GetImpl(context);
    const gl::State &state                  = context->getGLState();
    const gl::Program *programGL            = state.getProgram();
    const gl::AttributesMask &clientAttribs = context->getStateCache().getActiveClientAttribsMask();
    uint32_t maxAttrib                      = programGL->getState().getMaxActiveAttribLocation();

    if (clientAttribs.any())
    {
        ANGLE_TRY(drawCallParams.ensureIndexRangeResolved(context));
        ANGLE_TRY(streamVertexData(contextVk, clientAttribs, drawCallParams));
        commandBuffer->bindVertexBuffers(0, maxAttrib, mCurrentArrayBufferHandles.data(),
                                         mCurrentArrayBufferOffsets.data());
    }
    else if (mVertexBuffersDirty || newCommandBuffer)
    {
        if (maxAttrib > 0)
        {
            commandBuffer->bindVertexBuffers(0, maxAttrib, mCurrentArrayBufferHandles.data(),
                                             mCurrentArrayBufferOffsets.data());

            const gl::AttributesMask &bufferedAttribs =
                context->getStateCache().getActiveBufferedAttribsMask();

            vk::CommandGraphResource *drawFramebuffer = vk::GetImpl(state.getDrawFramebuffer());
            updateArrayBufferReadDependencies(drawFramebuffer, bufferedAttribs,
                                              contextVk->getRenderer()->getCurrentQueueSerial());
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
                                       const gl::DrawCallParams &drawCallParams,
                                       vk::CommandBuffer *commandBuffer,
                                       bool newCommandBuffer)
{
    ContextVk *contextVk = vk::GetImpl(context);
    ANGLE_TRY(onDraw(context, drawCallParams, commandBuffer, newCommandBuffer));
    bool isLineLoop      = drawCallParams.mode() == gl::PrimitiveMode::LineLoop;
    gl::Buffer *glBuffer = mState.getElementArrayBuffer().get();
    uintptr_t offset =
        glBuffer && !isLineLoop ? reinterpret_cast<uintptr_t>(drawCallParams.indices()) : 0;

    if (!glBuffer && !isLineLoop)
    {
        ANGLE_TRY(drawCallParams.ensureIndexRangeResolved(context));
        ANGLE_TRY(streamIndexData(contextVk, drawCallParams));
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
            ANGLE_TRY(bufferVk->mapImpl(contextVk, &srcDataMapping));
            uint8_t *srcData           = static_cast<uint8_t *>(srcDataMapping);
            intptr_t offsetIntoSrcData = reinterpret_cast<intptr_t>(drawCallParams.indices());
            srcData += offsetIntoSrcData;

            // Allocate a new buffer that's double the size of the buffer provided by the user to
            // go from unsigned byte to unsigned short.
            uint8_t *allocatedData      = nullptr;
            bool newBufferAllocated     = false;
            uint32_t expandedDataOffset = 0;
            ANGLE_TRY(mTranslatedByteIndexData.allocate(
                contextVk, static_cast<size_t>(bufferVk->getSize()) * 2, &allocatedData,
                &mCurrentElementArrayBufferHandle, &expandedDataOffset, &newBufferAllocated));
            mCurrentElementArrayBufferOffset = static_cast<VkDeviceSize>(expandedDataOffset);

            // Expand the source into the destination
            ASSERT(!context->getGLState().isPrimitiveRestartEnabled());
            uint16_t *expandedDst = reinterpret_cast<uint16_t *>(allocatedData);
            for (GLsizei index = 0; index < bufferVk->getSize() - offsetIntoSrcData; index++)
            {
                expandedDst[index] = static_cast<GLushort>(srcData[index]);
            }

            // Make sure our writes are available.
            ANGLE_TRY(mTranslatedByteIndexData.flush(contextVk));
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
        updateElementArrayBufferReadDependency(drawFramebuffer,
                                               contextVk->getRenderer()->getCurrentQueueSerial());
        mIndexBufferDirty = false;

        // If we've had a drawArrays call with a line loop before, we want to make sure this is
        // invalidated the next time drawArrays is called since we use the same index buffer for
        // both calls.
        mLineLoopBufferFirstIndex.reset();
        mLineLoopBufferLastIndex.reset();
    }

    return gl::NoError();
}

void VertexArrayVk::updateDefaultAttrib(RendererVk *renderer,
                                        size_t attribIndex,
                                        VkBuffer bufferHandle,
                                        uint32_t offset)
{
    if (!mState.getEnabledAttributesMask().test(attribIndex))
    {
        mCurrentArrayBufferHandles[attribIndex]   = bufferHandle;
        mCurrentArrayBufferOffsets[attribIndex]   = offset;
        mCurrentArrayBufferResources[attribIndex] = nullptr;
        mCurrentArrayBufferStrides[attribIndex]   = 0;
        mCurrentArrayBufferFormats[attribIndex] =
            &renderer->getFormat(angle::FormatID::R32G32B32A32_FIXED);
        mDirtyPackedInputs.set(attribIndex);
    }
}
}  // namespace rx
