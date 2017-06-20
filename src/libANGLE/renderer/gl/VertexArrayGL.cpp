//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexArrayGL.cpp: Implements the class methods for VertexArrayGL.

#include "libANGLE/renderer/gl/VertexArrayGL.h"

#include "common/bitset_utils.h"
#include "common/debug.h"
#include "common/mathutil.h"
#include "common/utilities.h"
#include "libANGLE/Buffer.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/gl/BufferGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/StateManagerGL.h"
#include "libANGLE/renderer/gl/renderergl_utils.h"

using namespace gl;

namespace rx
{
namespace
{
// Warning: you should ensure binding really matches attrib.bindingIndex before using this function.
bool AttributeNeedsStreaming(const VertexAttribute &attrib, const VertexBinding &binding)
{
    return (attrib.enabled && binding.getBuffer().get() == nullptr);
}

bool SameVertexAttribFormat(const VertexAttribute &a, const VertexAttribute &b)
{
    return a.size == b.size && a.type == b.type && a.normalized == b.normalized &&
           a.pureInteger == b.pureInteger && a.relativeOffset == b.relativeOffset;
}

bool SameVertexBuffer(const VertexBinding &a, const VertexBinding &b)
{
    return a.getStride() == b.getStride() && a.getOffset() == b.getOffset() &&
           a.getBuffer().get() == b.getBuffer().get();
}

bool IsVertexAttribPointerSupported(size_t attribIndex, const VertexAttribute &attrib)
{
    return (attribIndex == attrib.bindingIndex && attrib.relativeOffset == 0);
}
}  // anonymous namespace

VertexArrayGL::VertexArrayGL(const VertexArrayState &state,
                             const FunctionsGL *functions,
                             StateManagerGL *stateManager)
    : VertexArrayImpl(state),
      mFunctions(functions),
      mStateManager(stateManager),
      mVertexArrayID(0),
      mAppliedElementArrayBuffer(),
      mAppliedBindings(state.getMaxBindings()),
      mStreamingElementArrayBufferSize(0),
      mStreamingElementArrayBuffer(0),
      mStreamingArrayBufferSize(0),
      mStreamingArrayBuffer(0)
{
    ASSERT(mFunctions);
    ASSERT(mStateManager);
    mFunctions->genVertexArrays(1, &mVertexArrayID);

    // Set the cached vertex attribute array and vertex attribute binding array size
    GLuint maxVertexAttribs = static_cast<GLuint>(state.getMaxAttribs());
    for (GLuint i = 0; i < maxVertexAttribs; i++)
    {
        mAppliedAttributes.emplace_back(i);
    }
}

void VertexArrayGL::destroy(const gl::Context *context)
{
    mStateManager->deleteVertexArray(mVertexArrayID);
    mVertexArrayID = 0;

    mStateManager->deleteBuffer(mStreamingElementArrayBuffer);
    mStreamingElementArrayBufferSize = 0;
    mStreamingElementArrayBuffer     = 0;

    mStateManager->deleteBuffer(mStreamingArrayBuffer);
    mStreamingArrayBufferSize = 0;
    mStreamingArrayBuffer     = 0;

    mAppliedElementArrayBuffer.set(context, nullptr);
    for (auto &binding : mAppliedBindings)
    {
        binding.setBuffer(context, nullptr);
    }
}

gl::Error VertexArrayGL::syncDrawArraysState(const gl::Context *context,
                                             const gl::AttributesMask &activeAttributesMask,
                                             GLint first,
                                             GLsizei count,
                                             GLsizei instanceCount) const
{
    return syncDrawState(context, activeAttributesMask, first, count, GL_NONE, nullptr,
                         instanceCount, false, nullptr);
}

gl::Error VertexArrayGL::syncDrawElementsState(const gl::Context *context,
                                               const gl::AttributesMask &activeAttributesMask,
                                               GLsizei count,
                                               GLenum type,
                                               const void *indices,
                                               GLsizei instanceCount,
                                               bool primitiveRestartEnabled,
                                               const void **outIndices) const
{
    return syncDrawState(context, activeAttributesMask, 0, count, type, indices, instanceCount,
                         primitiveRestartEnabled, outIndices);
}

gl::Error VertexArrayGL::syncElementArrayState(const gl::Context *context) const
{
    gl::Buffer *elementArrayBuffer = mData.getElementArrayBuffer().get();
    ASSERT(elementArrayBuffer);
    if (elementArrayBuffer != mAppliedElementArrayBuffer.get())
    {
        const BufferGL *bufferGL = GetImplAs<BufferGL>(elementArrayBuffer);
        mStateManager->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferGL->getBufferID());
        mAppliedElementArrayBuffer.set(context, elementArrayBuffer);
    }

    return gl::NoError();
}

gl::Error VertexArrayGL::syncDrawState(const gl::Context *context,
                                       const gl::AttributesMask &activeAttributesMask,
                                       GLint first,
                                       GLsizei count,
                                       GLenum type,
                                       const void *indices,
                                       GLsizei instanceCount,
                                       bool primitiveRestartEnabled,
                                       const void **outIndices) const
{
    mStateManager->bindVertexArray(mVertexArrayID, getAppliedElementArrayBufferID());

    // Check if any attributes need to be streamed, determines if the index range needs to be
    // computed
    bool attributesNeedStreaming = mAttributesNeedStreaming.any();

    // Determine if an index buffer needs to be streamed and the range of vertices that need to be
    // copied
    IndexRange indexRange;
    if (type != GL_NONE)
    {
        ANGLE_TRY(syncIndexData(context, count, type, indices, primitiveRestartEnabled,
                                attributesNeedStreaming, &indexRange, outIndices));
    }
    else
    {
        // Not an indexed call, set the range to [first, first + count - 1]
        indexRange.start = first;
        indexRange.end   = first + count - 1;
    }

    if (attributesNeedStreaming)
    {
        ANGLE_TRY(streamAttributes(activeAttributesMask, instanceCount, indexRange));
    }

    return gl::NoError();
}

gl::Error VertexArrayGL::syncIndexData(const gl::Context *context,
                                       GLsizei count,
                                       GLenum type,
                                       const void *indices,
                                       bool primitiveRestartEnabled,
                                       bool attributesNeedStreaming,
                                       IndexRange *outIndexRange,
                                       const void **outIndices) const
{
    ASSERT(outIndices);

    gl::Buffer *elementArrayBuffer = mData.getElementArrayBuffer().get();

    // Need to check the range of indices if attributes need to be streamed
    if (elementArrayBuffer != nullptr)
    {
        if (elementArrayBuffer != mAppliedElementArrayBuffer.get())
        {
            const BufferGL *bufferGL = GetImplAs<BufferGL>(elementArrayBuffer);
            mStateManager->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferGL->getBufferID());
            mAppliedElementArrayBuffer.set(context, elementArrayBuffer);
        }

        // Only compute the index range if the attributes also need to be streamed
        if (attributesNeedStreaming)
        {
            ptrdiff_t elementArrayBufferOffset = reinterpret_cast<ptrdiff_t>(indices);
            Error error                        = mData.getElementArrayBuffer()->getIndexRange(
                type, elementArrayBufferOffset, count, primitiveRestartEnabled, outIndexRange);
            if (error.isError())
            {
                return error;
            }
        }

        // Indices serves as an offset into the index buffer in this case, use the same value for
        // the draw call
        *outIndices = indices;
    }
    else
    {
        // Need to stream the index buffer
        // TODO: if GLES, nothing needs to be streamed

        // Only compute the index range if the attributes also need to be streamed
        if (attributesNeedStreaming)
        {
            *outIndexRange = ComputeIndexRange(type, indices, count, primitiveRestartEnabled);
        }

        // Allocate the streaming element array buffer
        if (mStreamingElementArrayBuffer == 0)
        {
            mFunctions->genBuffers(1, &mStreamingElementArrayBuffer);
            mStreamingElementArrayBufferSize = 0;
        }

        mStateManager->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, mStreamingElementArrayBuffer);
        mAppliedElementArrayBuffer.set(context, nullptr);

        // Make sure the element array buffer is large enough
        const Type &indexTypeInfo          = GetTypeInfo(type);
        size_t requiredStreamingBufferSize = indexTypeInfo.bytes * count;
        if (requiredStreamingBufferSize > mStreamingElementArrayBufferSize)
        {
            // Copy the indices in while resizing the buffer
            mFunctions->bufferData(GL_ELEMENT_ARRAY_BUFFER, requiredStreamingBufferSize, indices,
                                   GL_DYNAMIC_DRAW);
            mStreamingElementArrayBufferSize = requiredStreamingBufferSize;
        }
        else
        {
            // Put the indices at the beginning of the buffer
            mFunctions->bufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, requiredStreamingBufferSize,
                                      indices);
        }

        // Set the index offset for the draw call to zero since the supplied index pointer is to
        // client data
        *outIndices = nullptr;
    }

    return gl::NoError();
}

void VertexArrayGL::computeStreamingAttributeSizes(const gl::AttributesMask &activeAttributesMask,
                                                   GLsizei instanceCount,
                                                   const gl::IndexRange &indexRange,
                                                   size_t *outStreamingDataSize,
                                                   size_t *outMaxAttributeDataSize) const
{
    *outStreamingDataSize    = 0;
    *outMaxAttributeDataSize = 0;

    ASSERT(mAttributesNeedStreaming.any());

    const auto &attribs  = mData.getVertexAttributes();
    const auto &bindings = mData.getVertexBindings();

    gl::AttributesMask attribsToStream = (mAttributesNeedStreaming & activeAttributesMask);

    for (auto idx : attribsToStream)
    {
        const auto &attrib  = attribs[idx];
        const auto &binding = bindings[attrib.bindingIndex];
        ASSERT(AttributeNeedsStreaming(attrib, binding));

        // If streaming is going to be required, compute the size of the required buffer
        // and how much slack space at the beginning of the buffer will be required by determining
        // the attribute with the largest data size.
        size_t typeSize = ComputeVertexAttributeTypeSize(attrib);
        *outStreamingDataSize += typeSize * ComputeVertexBindingElementCount(
                                                binding, indexRange.vertexCount(), instanceCount);
        *outMaxAttributeDataSize = std::max(*outMaxAttributeDataSize, typeSize);
    }
}

gl::Error VertexArrayGL::streamAttributes(const gl::AttributesMask &activeAttributesMask,
                                          GLsizei instanceCount,
                                          const gl::IndexRange &indexRange) const
{
    // Sync the vertex attribute state and track what data needs to be streamed
    size_t streamingDataSize    = 0;
    size_t maxAttributeDataSize = 0;

    computeStreamingAttributeSizes(activeAttributesMask, instanceCount, indexRange,
                                   &streamingDataSize, &maxAttributeDataSize);

    if (streamingDataSize == 0)
    {
        return gl::NoError();
    }

    if (mStreamingArrayBuffer == 0)
    {
        mFunctions->genBuffers(1, &mStreamingArrayBuffer);
        mStreamingArrayBufferSize = 0;
    }

    // If first is greater than zero, a slack space needs to be left at the beginning of the buffer
    // so that the same 'first' argument can be passed into the draw call.
    const size_t bufferEmptySpace   = maxAttributeDataSize * indexRange.start;
    const size_t requiredBufferSize = streamingDataSize + bufferEmptySpace;

    mStateManager->bindBuffer(GL_ARRAY_BUFFER, mStreamingArrayBuffer);
    if (requiredBufferSize > mStreamingArrayBufferSize)
    {
        mFunctions->bufferData(GL_ARRAY_BUFFER, requiredBufferSize, nullptr, GL_DYNAMIC_DRAW);
        mStreamingArrayBufferSize = requiredBufferSize;
    }

    // Unmapping a buffer can return GL_FALSE to indicate that the system has corrupted the data
    // somehow (such as by a screen change), retry writing the data a few times and return
    // OUT_OF_MEMORY if that fails.
    GLboolean unmapResult     = GL_FALSE;
    size_t unmapRetryAttempts = 5;
    while (unmapResult != GL_TRUE && --unmapRetryAttempts > 0)
    {
        uint8_t *bufferPointer = MapBufferRangeWithFallback(mFunctions, GL_ARRAY_BUFFER, 0,
                                                            requiredBufferSize, GL_MAP_WRITE_BIT);
        size_t curBufferOffset = bufferEmptySpace;

        const auto &attribs  = mData.getVertexAttributes();
        const auto &bindings = mData.getVertexBindings();

        gl::AttributesMask attribsToStream = (mAttributesNeedStreaming & activeAttributesMask);

        for (auto idx : attribsToStream)
        {
            const auto &attrib  = attribs[idx];
            const auto &binding = bindings[attrib.bindingIndex];
            ASSERT(AttributeNeedsStreaming(attrib, binding));

            const size_t streamedVertexCount =
                ComputeVertexBindingElementCount(binding, indexRange.vertexCount(), instanceCount);

            const size_t sourceStride = ComputeVertexAttributeStride(attrib, binding);
            const size_t destStride   = ComputeVertexAttributeTypeSize(attrib);

            // Vertices do not apply the 'start' offset when the divisor is non-zero even when doing
            // a non-instanced draw call
            const size_t firstIndex = binding.getDivisor() == 0 ? indexRange.start : 0;

            // Attributes using client memory ignore the VERTEX_ATTRIB_BINDING state.
            // https://www.opengl.org/registry/specs/ARB/vertex_attrib_binding.txt
            const uint8_t *inputPointer = reinterpret_cast<const uint8_t *>(attrib.pointer);

            // Pack the data when copying it, user could have supplied a very large stride that
            // would cause the buffer to be much larger than needed.
            if (destStride == sourceStride)
            {
                // Can copy in one go, the data is packed
                memcpy(bufferPointer + curBufferOffset, inputPointer + (sourceStride * firstIndex),
                       destStride * streamedVertexCount);
            }
            else
            {
                // Copy each vertex individually
                for (size_t vertexIdx = 0; vertexIdx < streamedVertexCount; vertexIdx++)
                {
                    uint8_t *out      = bufferPointer + curBufferOffset + (destStride * vertexIdx);
                    const uint8_t *in = inputPointer + sourceStride * (vertexIdx + firstIndex);
                    memcpy(out, in, destStride);
                }
            }

            // Compute where the 0-index vertex would be.
            const size_t vertexStartOffset = curBufferOffset - (firstIndex * destStride);

            callVertexAttribPointer(static_cast<GLuint>(idx), attrib,
                                    static_cast<GLsizei>(destStride),
                                    static_cast<GLintptr>(vertexStartOffset));

            curBufferOffset += destStride * streamedVertexCount;
        }

        unmapResult = mFunctions->unmapBuffer(GL_ARRAY_BUFFER);
    }

    if (unmapResult != GL_TRUE)
    {
        return gl::OutOfMemory() << "Failed to unmap the client data streaming buffer.";
    }

    return gl::NoError();
}

GLuint VertexArrayGL::getVertexArrayID() const
{
    return mVertexArrayID;
}

GLuint VertexArrayGL::getAppliedElementArrayBufferID() const
{
    if (mAppliedElementArrayBuffer.get() == nullptr)
    {
        return mStreamingElementArrayBuffer;
    }

    return GetImplAs<BufferGL>(mAppliedElementArrayBuffer.get())->getBufferID();
}

void VertexArrayGL::updateNeedsStreaming(size_t attribIndex)
{
    const auto &attrib  = mData.getVertexAttribute(attribIndex);
    const auto &binding = mData.getBindingFromAttribIndex(attribIndex);
    mAttributesNeedStreaming.set(attribIndex, AttributeNeedsStreaming(attrib, binding));
}

void VertexArrayGL::updateAttribEnabled(size_t attribIndex)
{
    const bool enabled = mData.getVertexAttribute(attribIndex).enabled;
    if (mAppliedAttributes[attribIndex].enabled == enabled)
    {
        return;
    }

    updateNeedsStreaming(attribIndex);

    if (enabled)
    {
        mFunctions->enableVertexAttribArray(static_cast<GLuint>(attribIndex));
    }
    else
    {
        mFunctions->disableVertexAttribArray(static_cast<GLuint>(attribIndex));
    }

    mAppliedAttributes[attribIndex].enabled = enabled;
}

void VertexArrayGL::updateAttribPointer(const gl::Context *context, size_t attribIndex)
{
    const VertexAttribute &attrib = mData.getVertexAttribute(attribIndex);

    // TODO(jiawei.shao@intel.com): Vertex Attrib Binding
    ASSERT(IsVertexAttribPointerSupported(attribIndex, attrib));

    const GLuint bindingIndex    = attrib.bindingIndex;
    const VertexBinding &binding = mData.getVertexBinding(bindingIndex);

    // We do not need to compare attrib.pointer because when we use a different client memory
    // pointer, we don't need to update mAttributesNeedStreaming by binding.buffer and we won't
    // update attribPointer in this function.
    if (SameVertexAttribFormat(mAppliedAttributes[attribIndex], attrib) &&
        mAppliedAttributes[attribIndex].bindingIndex == bindingIndex &&
        SameVertexBuffer(mAppliedBindings[attribIndex], binding))
    {
        return;
    }

    updateNeedsStreaming(attribIndex);

    // If we need to stream, defer the attribPointer to the draw call.
    // Skip the attribute that is disabled and uses a client memory pointer.
    const Buffer *arrayBuffer = binding.getBuffer().get();
    if (arrayBuffer == nullptr)
    {
        // Mark the applied binding is using a client memory pointer by setting its buffer to
        // nullptr so that if it doesn't use a client memory pointer later, there is no chance that
        // the caching will skip it.
        mAppliedBindings[bindingIndex].setBuffer(context, nullptr);
        return;
    }

    // Since ANGLE always uses a non-zero VAO, we cannot use a client memory pointer on it:
    // [OpenGL ES 3.0.2] Section 2.8 page 24:
    // An INVALID_OPERATION error is generated when a non-zero vertex array object is bound,
    // zero is bound to the ARRAY_BUFFER buffer object binding point, and the pointer argument
    // is not NULL.

    const BufferGL *arrayBufferGL = GetImplAs<BufferGL>(arrayBuffer);
    mStateManager->bindBuffer(GL_ARRAY_BUFFER, arrayBufferGL->getBufferID());

    callVertexAttribPointer(static_cast<GLuint>(attribIndex), attrib, binding.getStride(),
                            binding.getOffset());

    mAppliedAttributes[attribIndex].size                    = attrib.size;
    mAppliedAttributes[attribIndex].type                    = attrib.type;
    mAppliedAttributes[attribIndex].normalized              = attrib.normalized;
    mAppliedAttributes[attribIndex].pureInteger             = attrib.pureInteger;
    mAppliedAttributes[attribIndex].relativeOffset          = attrib.relativeOffset;

    mAppliedAttributes[attribIndex].bindingIndex = bindingIndex;

    mAppliedBindings[bindingIndex].setStride(binding.getStride());
    mAppliedBindings[bindingIndex].setOffset(binding.getOffset());
    mAppliedBindings[bindingIndex].setBuffer(context, binding.getBuffer().get());
}

void VertexArrayGL::callVertexAttribPointer(GLuint attribIndex,
                                            const VertexAttribute &attrib,
                                            GLsizei stride,
                                            GLintptr offset) const
{
    const GLvoid *pointer = reinterpret_cast<const GLvoid *>(offset);
    if (attrib.pureInteger)
    {
        ASSERT(!attrib.normalized);
        mFunctions->vertexAttribIPointer(attribIndex, attrib.size, attrib.type, stride, pointer);
    }
    else
    {
        mFunctions->vertexAttribPointer(attribIndex, attrib.size, attrib.type, attrib.normalized,
                                        stride, pointer);
    }
}

void VertexArrayGL::updateAttribDivisor(size_t attribIndex)
{
    const GLuint bindingIndex = mData.getVertexAttribute(attribIndex).bindingIndex;
    ASSERT(attribIndex == bindingIndex);

    const GLuint divisor = mData.getVertexBinding(bindingIndex).getDivisor();
    if (mAppliedAttributes[attribIndex].bindingIndex == bindingIndex &&
        mAppliedBindings[bindingIndex].getDivisor() == divisor)
    {
        return;
    }

    mFunctions->vertexAttribDivisor(static_cast<GLuint>(attribIndex), divisor);

    mAppliedAttributes[attribIndex].bindingIndex = bindingIndex;
    mAppliedBindings[bindingIndex].setDivisor(divisor);
}

void VertexArrayGL::syncState(const gl::Context *context, const VertexArray::DirtyBits &dirtyBits)
{
    mStateManager->bindVertexArray(mVertexArrayID, getAppliedElementArrayBufferID());

    for (size_t dirtyBit : dirtyBits)
    {
        if (dirtyBit == VertexArray::DIRTY_BIT_ELEMENT_ARRAY_BUFFER)
        {
            // TODO(jmadill): Element array buffer bindings
            continue;
        }

        size_t index = VertexArray::GetAttribIndex(dirtyBit);
        if (dirtyBit >= VertexArray::DIRTY_BIT_ATTRIB_0_ENABLED &&
            dirtyBit < VertexArray::DIRTY_BIT_ATTRIB_MAX_ENABLED)
        {
            updateAttribEnabled(index);
        }
        else if (dirtyBit >= VertexArray::DIRTY_BIT_ATTRIB_0_POINTER &&
                 dirtyBit < VertexArray::DIRTY_BIT_ATTRIB_MAX_POINTER)
        {
            updateAttribPointer(context, index);
        }
        else if (dirtyBit >= VertexArray::DIRTY_BIT_ATTRIB_0_FORMAT &&
                 dirtyBit < VertexArray::DIRTY_BIT_BINDING_MAX_BUFFER)
        {
            // TODO(jiawei.shao@intel.com): Vertex Attrib Bindings
            ASSERT(index == mData.getBindingIndexFromAttribIndex(index));
        }
        else if (dirtyBit >= VertexArray::DIRTY_BIT_BINDING_0_DIVISOR &&
                 dirtyBit < VertexArray::DIRTY_BIT_BINDING_MAX_DIVISOR)
        {
            updateAttribDivisor(index);
        }
        else
            UNREACHABLE();
    }
}

}  // rx
