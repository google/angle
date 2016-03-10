//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer.cpp: Defines the abstract VertexBuffer class and VertexBufferInterface
// class with derivations, classes that perform graphics API agnostic vertex buffer operations.

#include "libANGLE/renderer/d3d/VertexBuffer.h"

#include "common/mathutil.h"
#include "libANGLE/renderer/d3d/BufferD3D.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/VertexAttribute.h"

namespace rx
{

// VertexBuffer Implementation
unsigned int VertexBuffer::mNextSerial = 1;

VertexBuffer::VertexBuffer() : mRefCount(1)
{
    updateSerial();
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::updateSerial()
{
    mSerial = mNextSerial++;
}

unsigned int VertexBuffer::getSerial() const
{
    return mSerial;
}

void VertexBuffer::addRef()
{
    mRefCount++;
}

void VertexBuffer::release()
{
    ASSERT(mRefCount > 0);
    mRefCount--;

    if (mRefCount == 0)
    {
        delete this;
    }
}

// VertexBufferInterface Implementation
VertexBufferInterface::VertexBufferInterface(BufferFactoryD3D *factory, bool dynamic)
    : mFactory(factory), mVertexBuffer(factory->createVertexBuffer()), mDynamic(dynamic)
{
}

VertexBufferInterface::~VertexBufferInterface()
{
    if (mVertexBuffer)
    {
        mVertexBuffer->release();
    }
}

unsigned int VertexBufferInterface::getSerial() const
{
    return mVertexBuffer->getSerial();
}

unsigned int VertexBufferInterface::getBufferSize() const
{
    return mVertexBuffer->getBufferSize();
}

gl::Error VertexBufferInterface::setBufferSize(unsigned int size)
{
    if (mVertexBuffer->getBufferSize() == 0)
    {
        return mVertexBuffer->initialize(size, mDynamic);
    }

    return mVertexBuffer->setBufferSize(size);
}

gl::ErrorOrResult<unsigned int> VertexBufferInterface::getSpaceRequired(
    const gl::VertexAttribute &attrib,
    GLsizei count,
    GLsizei instances) const
{
    auto errorOrSpaceRequired = mFactory->getVertexSpaceRequired(attrib, count, instances);
    if (errorOrSpaceRequired.isError())
    {
        return errorOrSpaceRequired.getError();
    }

    unsigned int spaceRequired = errorOrSpaceRequired.getResult();

    // Align to 16-byte boundary
    unsigned int alignedSpaceRequired = roundUp(spaceRequired, 16u);

    if (alignedSpaceRequired < spaceRequired)
    {
        return gl::Error(GL_OUT_OF_MEMORY,
                         "Vertex buffer overflow in VertexBufferInterface::getSpaceRequired.");
    }

    return std::move(alignedSpaceRequired);
}

gl::Error VertexBufferInterface::discard()
{
    return mVertexBuffer->discard();
}

VertexBuffer *VertexBufferInterface::getVertexBuffer() const
{
    return mVertexBuffer;
}

// StreamingVertexBufferInterface Implementation
StreamingVertexBufferInterface::StreamingVertexBufferInterface(BufferFactoryD3D *factory,
                                                               std::size_t initialSize)
    : VertexBufferInterface(factory, true), mWritePosition(0), mReservedSpace(0)
{
    setBufferSize(static_cast<unsigned int>(initialSize));
}

StreamingVertexBufferInterface::~StreamingVertexBufferInterface()
{
}

gl::Error StreamingVertexBufferInterface::reserveSpace(unsigned int size)
{
    unsigned int curBufferSize = getBufferSize();
    if (size > curBufferSize)
    {
        gl::Error error = setBufferSize(std::max(size, 3 * curBufferSize / 2));
        if (error.isError())
        {
            return error;
        }
        mWritePosition = 0;
    }
    else if (mWritePosition + size > curBufferSize)
    {
        gl::Error error = discard();
        if (error.isError())
        {
            return error;
        }
        mWritePosition = 0;
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error StreamingVertexBufferInterface::storeDynamicAttribute(const gl::VertexAttribute &attrib,
                                                                GLenum currentValueType,
                                                                GLint start,
                                                                GLsizei count,
                                                                GLsizei instances,
                                                                unsigned int *outStreamOffset,
                                                                const uint8_t *sourceData)
{
    auto spaceRequiredOrError = getSpaceRequired(attrib, count, instances);
    if (spaceRequiredOrError.isError())
    {
        return spaceRequiredOrError.getError();
    }

    unsigned int alignedSpaceRequired = spaceRequiredOrError.getResult();

    // Protect against integer overflow
    if (!IsUnsignedAdditionSafe(mWritePosition, alignedSpaceRequired))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Internal error, new vertex buffer write position would overflow.");
    }

    gl::Error error = reserveSpace(mReservedSpace);
    if (error.isError())
    {
        return error;
    }
    mReservedSpace = 0;

    error = mVertexBuffer->storeVertexAttributes(attrib, currentValueType, start, count, instances,
                                                 mWritePosition, sourceData);
    if (error.isError())
    {
        return error;
    }

    if (outStreamOffset)
    {
        *outStreamOffset = mWritePosition;
    }

    mWritePosition += alignedSpaceRequired;

    return gl::Error(GL_NO_ERROR);
}

gl::Error StreamingVertexBufferInterface::reserveVertexSpace(const gl::VertexAttribute &attrib,
                                                             GLsizei count,
                                                             GLsizei instances)
{
    auto errorOrRequiredSpace = mFactory->getVertexSpaceRequired(attrib, count, instances);
    if (errorOrRequiredSpace.isError())
    {
        return errorOrRequiredSpace.getError();
    }

    unsigned int requiredSpace = errorOrRequiredSpace.getResult();

    // Align to 16-byte boundary
    unsigned int alignedRequiredSpace = roundUp(requiredSpace, 16u);

    // Protect against integer overflow
    if (!IsUnsignedAdditionSafe(mReservedSpace, alignedRequiredSpace) ||
        alignedRequiredSpace < requiredSpace)
    {
        return gl::Error(GL_OUT_OF_MEMORY,
                         "Unable to reserve %u extra bytes in internal vertex buffer, "
                         "it would result in an overflow.",
                         requiredSpace);
    }

    mReservedSpace += alignedRequiredSpace;

    return gl::Error(GL_NO_ERROR);
}

// StaticVertexBufferInterface Implementation
StaticVertexBufferInterface::AttributeSignature::AttributeSignature()
    : type(GL_NONE), size(0), stride(0), normalized(false), pureInteger(false), offset(0)
{
}

bool StaticVertexBufferInterface::AttributeSignature::matchesAttribute(
    const gl::VertexAttribute &attrib) const
{
    size_t attribStride = ComputeVertexAttributeStride(attrib);

    if (type != attrib.type || size != attrib.size || static_cast<GLuint>(stride) != attribStride ||
        normalized != attrib.normalized || pureInteger != attrib.pureInteger)
    {
        return false;
    }

    size_t attribOffset = (static_cast<size_t>(attrib.offset) % attribStride);
    return (offset == attribOffset);
}

void StaticVertexBufferInterface::AttributeSignature::set(const gl::VertexAttribute &attrib)
{
    type        = attrib.type;
    size        = attrib.size;
    normalized  = attrib.normalized;
    pureInteger = attrib.pureInteger;
    offset = stride = static_cast<GLuint>(ComputeVertexAttributeStride(attrib));
    offset = static_cast<size_t>(attrib.offset) % ComputeVertexAttributeStride(attrib);
}

StaticVertexBufferInterface::StaticVertexBufferInterface(BufferFactoryD3D *factory)
    : VertexBufferInterface(factory, false)
{
}

StaticVertexBufferInterface::~StaticVertexBufferInterface()
{
}

bool StaticVertexBufferInterface::matchesAttribute(const gl::VertexAttribute &attrib) const
{
    return mSignature.matchesAttribute(attrib);
}

void StaticVertexBufferInterface::setAttribute(const gl::VertexAttribute &attrib)
{
    return mSignature.set(attrib);
}

gl::Error StaticVertexBufferInterface::storeStaticAttribute(const gl::VertexAttribute &attrib,
                                                            GLint start,
                                                            GLsizei count,
                                                            GLsizei instances,
                                                            const uint8_t *sourceData)
{
    auto spaceRequiredOrError = getSpaceRequired(attrib, count, instances);
    if (spaceRequiredOrError.isError())
    {
        return spaceRequiredOrError.getError();
    }

    setBufferSize(spaceRequiredOrError.getResult());

    ASSERT(attrib.enabled);
    gl::Error error = mVertexBuffer->storeVertexAttributes(attrib, GL_NONE, start, count, instances,
                                                           0, sourceData);
    if (!error.isError())
    {
        mSignature.set(attrib);
        mVertexBuffer->hintUnmapResource();
    }

    return error;
}

}  // namespace rx
