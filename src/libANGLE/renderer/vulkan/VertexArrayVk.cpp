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
#include "libANGLE/renderer/vulkan/CommandBufferNode.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/formatutilsvk.h"

namespace rx
{

VertexArrayVk::VertexArrayVk(const gl::VertexArrayState &state)
    : VertexArrayImpl(state),
      mCurrentArrayBufferHandles{},
      mCurrentArrayBufferResources{},
      mCurrentElementArrayBufferResource(nullptr),
      mCurrentVertexDescsValid(false)
{
    mCurrentArrayBufferHandles.fill(VK_NULL_HANDLE);
    mCurrentArrayBufferResources.fill(nullptr);
}

VertexArrayVk::~VertexArrayVk()
{
}

void VertexArrayVk::destroy(const gl::Context *context)
{
}

void VertexArrayVk::syncState(const gl::Context *context,
                              const gl::VertexArray::DirtyBits &dirtyBits)
{
    ASSERT(dirtyBits.any());

    // Invalidate current pipeline.
    // TODO(jmadill): Use pipeline cache.
    ContextVk *contextVk = vk::GetImpl(context);
    contextVk->onVertexArrayChange();

    // Invalidate the vertex descriptions.
    invalidateVertexDescriptions();

    // Rebuild current attribute buffers cache. This will fail horribly if the buffer changes.
    // TODO(jmadill): Handle buffer storage changes.
    const auto &attribs  = mState.getVertexAttributes();
    const auto &bindings = mState.getVertexBindings();

    for (auto dirtyBit : dirtyBits)
    {
        if (dirtyBit == gl::VertexArray::DIRTY_BIT_ELEMENT_ARRAY_BUFFER)
        {
            gl::Buffer *bufferGL = mState.getElementArrayBuffer().get();
            if (bufferGL)
            {
                mCurrentElementArrayBufferResource = vk::GetImpl(bufferGL);
            }
            else
            {
                mCurrentElementArrayBufferResource = nullptr;
            }
            continue;
        }

        size_t attribIndex = gl::VertexArray::GetVertexIndexFromDirtyBit(dirtyBit);

        const auto &attrib  = attribs[attribIndex];
        const auto &binding = bindings[attrib.bindingIndex];

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
        }
        else
        {
            UNIMPLEMENTED();
        }
    }
}

const gl::AttribArray<VkBuffer> &VertexArrayVk::getCurrentArrayBufferHandles() const
{
    return mCurrentArrayBufferHandles;
}

void VertexArrayVk::updateDrawDependencies(vk::CommandBufferNode *readNode,
                                           const gl::AttributesMask &activeAttribsMask,
                                           Serial serial,
                                           DrawType drawType)
{
    // Handle the bound array buffers.
    for (auto attribIndex : activeAttribsMask)
    {
        ASSERT(mCurrentArrayBufferResources[attribIndex]);
        mCurrentArrayBufferResources[attribIndex]->updateDependencies(readNode, serial);
    }

    // Handle the bound element array buffer.
    if (drawType == DrawType::Elements)
    {
        ASSERT(mCurrentElementArrayBufferResource);
        mCurrentElementArrayBufferResource->updateDependencies(readNode, serial);
    }
}

void VertexArrayVk::invalidateVertexDescriptions()
{
    mCurrentVertexDescsValid = false;
}

void VertexArrayVk::updateVertexDescriptions(const gl::Context *context,
                                             vk::PipelineDesc *pipelineDesc)
{
    if (mCurrentVertexDescsValid)
    {
        return;
    }

    const auto &attribs  = mState.getVertexAttributes();
    const auto &bindings = mState.getVertexBindings();

    const gl::Program *programGL = context->getGLState().getProgram();

    pipelineDesc->resetVertexInputState();

    for (auto attribIndex : programGL->getActiveAttribLocationsMask())
    {
        const auto &attrib  = attribs[attribIndex];
        const auto &binding = bindings[attrib.bindingIndex];
        if (attrib.enabled)
        {
            pipelineDesc->updateVertexInputInfo(static_cast<uint32_t>(attribIndex), binding,
                                                attrib);
        }
        else
        {
            UNIMPLEMENTED();
        }
    }

    mCurrentVertexDescsValid = true;
}

}  // namespace rx
