//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation of the state class for mananging GLES 3 Vertex Array Objects.
//

#include "libANGLE/VertexArray.h"
#include "libANGLE/Buffer.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/GLImplFactory.h"
#include "libANGLE/renderer/VertexArrayImpl.h"

namespace gl
{

VertexArrayState::VertexArrayState(size_t maxAttribs, size_t maxAttribBindings)
    : mLabel(), mVertexBindings(maxAttribBindings)
{
    ASSERT(maxAttribs <= maxAttribBindings);

    for (size_t i = 0; i < maxAttribs; i++)
    {
        mVertexAttributes.emplace_back(static_cast<GLuint>(i));
    }
}

VertexArrayState::~VertexArrayState()
{
}

VertexArray::VertexArray(rx::GLImplFactory *factory,
                         GLuint id,
                         size_t maxAttribs,
                         size_t maxAttribBindings)
    : mId(id),
      mState(maxAttribs, maxAttribBindings),
      mVertexArray(factory->createVertexArray(mState))
{
}

void VertexArray::onDestroy(const Context *context)
{
    bool isBound = context->isCurrentVertexArray(this);
    for (auto &binding : mState.mVertexBindings)
    {
        binding.setBuffer(context, nullptr, isBound);
    }
    if (isBound && mState.mElementArrayBuffer.get())
        mState.mElementArrayBuffer->onBindingChanged(false, BufferBinding::ElementArray);
    mState.mElementArrayBuffer.set(context, nullptr);
    mVertexArray->destroy(context);
    SafeDelete(mVertexArray);
    delete this;
}

VertexArray::~VertexArray()
{
    ASSERT(!mVertexArray);
}

GLuint VertexArray::id() const
{
    return mId;
}

void VertexArray::setLabel(const std::string &label)
{
    mState.mLabel = label;
}

const std::string &VertexArray::getLabel() const
{
    return mState.mLabel;
}

void VertexArray::detachBuffer(const Context *context, GLuint bufferName)
{
    bool isBound = context->isCurrentVertexArray(this);
    for (auto &binding : mState.mVertexBindings)
    {
        if (binding.getBuffer().id() == bufferName)
        {
            binding.setBuffer(context, nullptr, isBound);
        }
    }

    if (mState.mElementArrayBuffer.id() == bufferName)
    {
        if (isBound && mState.mElementArrayBuffer.get())
            mState.mElementArrayBuffer->onBindingChanged(false, BufferBinding::Array);
        mState.mElementArrayBuffer.set(context, nullptr);
    }
}

const VertexAttribute &VertexArray::getVertexAttribute(size_t attribIndex) const
{
    ASSERT(attribIndex < getMaxAttribs());
    return mState.mVertexAttributes[attribIndex];
}

const VertexBinding &VertexArray::getVertexBinding(size_t bindingIndex) const
{
    ASSERT(bindingIndex < getMaxBindings());
    return mState.mVertexBindings[bindingIndex];
}

size_t VertexArray::GetVertexIndexFromDirtyBit(size_t dirtyBit)
{
    static_assert(gl::MAX_VERTEX_ATTRIBS == gl::MAX_VERTEX_ATTRIB_BINDINGS,
                  "The stride of vertex attributes should equal to that of vertex bindings.");
    ASSERT(dirtyBit > DIRTY_BIT_ELEMENT_ARRAY_BUFFER);
    return (dirtyBit - DIRTY_BIT_ATTRIB_0) % gl::MAX_VERTEX_ATTRIBS;
}

void VertexArray::setDirtyAttribBit(size_t attribIndex, DirtyAttribBitType dirtyAttribBit)
{
    mDirtyBits.set(DIRTY_BIT_ATTRIB_0 + attribIndex);
    mDirtyAttribBits[attribIndex].set(dirtyAttribBit);
}

void VertexArray::setDirtyBindingBit(size_t bindingIndex, DirtyBindingBitType dirtyBindingBit)
{
    mDirtyBits.set(DIRTY_BIT_BINDING_0 + bindingIndex);
    mDirtyBindingBits[bindingIndex].set(dirtyBindingBit);
}

void VertexArray::bindVertexBufferImpl(const Context *context,
                                       size_t bindingIndex,
                                       Buffer *boundBuffer,
                                       GLintptr offset,
                                       GLsizei stride)
{
    ASSERT(bindingIndex < getMaxBindings());
    bool isBound = context->isCurrentVertexArray(this);

    VertexBinding *binding = &mState.mVertexBindings[bindingIndex];

    binding->setBuffer(context, boundBuffer, isBound);
    binding->setOffset(offset);
    binding->setStride(stride);
}

void VertexArray::bindVertexBuffer(const Context *context,
                                   size_t bindingIndex,
                                   Buffer *boundBuffer,
                                   GLintptr offset,
                                   GLsizei stride)
{
    bindVertexBufferImpl(context, bindingIndex, boundBuffer, offset, stride);
    setDirtyBindingBit(bindingIndex, DIRTY_BINDING_BUFFER);
}

void VertexArray::setVertexAttribBinding(const Context *context,
                                         size_t attribIndex,
                                         GLuint bindingIndex)
{
    ASSERT(attribIndex < getMaxAttribs() && bindingIndex < getMaxBindings());

    if (mState.mVertexAttributes[attribIndex].bindingIndex != bindingIndex)
    {
        // In ES 3.0 contexts, the binding cannot change, hence the code below is unreachable.
        ASSERT(context->getClientVersion() >= ES_3_1);
        mState.mVertexAttributes[attribIndex].bindingIndex = bindingIndex;

        setDirtyAttribBit(attribIndex, DIRTY_ATTRIB_BINDING);
    }
    mState.mVertexAttributes[attribIndex].bindingIndex = static_cast<GLuint>(bindingIndex);
}

void VertexArray::setVertexBindingDivisor(size_t bindingIndex, GLuint divisor)
{
    ASSERT(bindingIndex < getMaxBindings());

    mState.mVertexBindings[bindingIndex].setDivisor(divisor);
    setDirtyBindingBit(bindingIndex, DIRTY_BINDING_DIVISOR);
}

void VertexArray::setVertexAttribFormatImpl(size_t attribIndex,
                                            GLint size,
                                            GLenum type,
                                            bool normalized,
                                            bool pureInteger,
                                            GLuint relativeOffset)
{
    ASSERT(attribIndex < getMaxAttribs());

    VertexAttribute *attrib = &mState.mVertexAttributes[attribIndex];

    attrib->size           = size;
    attrib->type           = type;
    attrib->normalized     = normalized;
    attrib->pureInteger    = pureInteger;
    attrib->relativeOffset = relativeOffset;
    mState.mVertexAttributesTypeMask.setIndex(GetVertexAttributeBaseType(*attrib), attribIndex);
    mState.mEnabledAttributesMask.set(attribIndex);
}

void VertexArray::setVertexAttribFormat(size_t attribIndex,
                                        GLint size,
                                        GLenum type,
                                        bool normalized,
                                        bool pureInteger,
                                        GLuint relativeOffset)
{
    setVertexAttribFormatImpl(attribIndex, size, type, normalized, pureInteger, relativeOffset);
    setDirtyAttribBit(attribIndex, DIRTY_ATTRIB_FORMAT);
}

void VertexArray::setVertexAttribDivisor(const Context *context, size_t attribIndex, GLuint divisor)
{
    ASSERT(attribIndex < getMaxAttribs());

    setVertexAttribBinding(context, attribIndex, static_cast<GLuint>(attribIndex));
    setVertexBindingDivisor(attribIndex, divisor);
}

void VertexArray::enableAttribute(size_t attribIndex, bool enabledState)
{
    ASSERT(attribIndex < getMaxAttribs());

    mState.mVertexAttributes[attribIndex].enabled = enabledState;
    mState.mVertexAttributesTypeMask.setIndex(
        GetVertexAttributeBaseType(mState.mVertexAttributes[attribIndex]), attribIndex);

    setDirtyAttribBit(attribIndex, DIRTY_ATTRIB_ENABLED);

    // Update state cache
    mState.mEnabledAttributesMask.set(attribIndex, enabledState);
}

void VertexArray::setVertexAttribPointer(const Context *context,
                                         size_t attribIndex,
                                         gl::Buffer *boundBuffer,
                                         GLint size,
                                         GLenum type,
                                         bool normalized,
                                         bool pureInteger,
                                         GLsizei stride,
                                         const void *pointer)
{
    ASSERT(attribIndex < getMaxAttribs());

    GLintptr offset = boundBuffer ? reinterpret_cast<GLintptr>(pointer) : 0;

    setVertexAttribFormatImpl(attribIndex, size, type, normalized, pureInteger, 0);
    setVertexAttribBinding(context, attribIndex, static_cast<GLuint>(attribIndex));

    VertexAttribute &attrib = mState.mVertexAttributes[attribIndex];

    GLsizei effectiveStride =
        stride != 0 ? stride : static_cast<GLsizei>(ComputeVertexAttributeTypeSize(attrib));
    attrib.pointer                 = pointer;
    attrib.vertexAttribArrayStride = stride;

    bindVertexBufferImpl(context, attribIndex, boundBuffer, offset, effectiveStride);

    setDirtyAttribBit(attribIndex, DIRTY_ATTRIB_POINTER);
}

void VertexArray::setElementArrayBuffer(const Context *context, Buffer *buffer)
{
    bool isBound = context->isCurrentVertexArray(this);
    if (isBound && mState.mElementArrayBuffer.get())
        mState.mElementArrayBuffer->onBindingChanged(false, BufferBinding::ElementArray);
    mState.mElementArrayBuffer.set(context, buffer);
    if (isBound && mState.mElementArrayBuffer.get())
        mState.mElementArrayBuffer->onBindingChanged(true, BufferBinding::ElementArray);
    mDirtyBits.set(DIRTY_BIT_ELEMENT_ARRAY_BUFFER);
}

void VertexArray::syncState(const Context *context)
{
    if (mDirtyBits.any())
    {
        mVertexArray->syncState(context, mDirtyBits, mDirtyAttribBits, mDirtyBindingBits);
        mDirtyBits.reset();

        // This is a bit of an implementation hack - but since we know the implementation
        // details of the dirty bit class it should always have the same effect as iterating
        // individual attribs. We could also look into schemes where iterating the dirty
        // bit set also resets it as you pass through it.
        memset(&mDirtyAttribBits, 0, sizeof(mDirtyAttribBits));
        memset(&mDirtyBindingBits, 0, sizeof(mDirtyBindingBits));
    }
}

void VertexArray::onBindingChanged(bool bound)
{
    if (mState.mElementArrayBuffer.get())
        mState.mElementArrayBuffer->onBindingChanged(bound, BufferBinding::ElementArray);
    for (auto &binding : mState.mVertexBindings)
    {
        binding.onContainerBindingChanged(bound);
    }
}

}  // namespace gl
