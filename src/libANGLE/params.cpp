//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// params:
//   Parameter wrapper structs for OpenGL ES. These helpers cache re-used values
//   in entry point routines.

#include "libANGLE/params.h"

#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/VertexArray.h"

namespace gl
{

// static
constexpr ParamTypeInfo ParamsBase::TypeInfo;
constexpr ParamTypeInfo HasIndexRange::TypeInfo;

// HasIndexRange implementation.
HasIndexRange::HasIndexRange()
    : ParamsBase(nullptr), mContext(nullptr), mCount(0), mType(GL_NONE), mIndices(nullptr)
{
}

HasIndexRange::HasIndexRange(Context *context, GLsizei count, GLenum type, const void *indices)
    : ParamsBase(context), mContext(context), mCount(count), mType(type), mIndices(indices)
{
}

const Optional<IndexRange> &HasIndexRange::getIndexRange() const
{
    if (mIndexRange.valid() || !mContext)
    {
        return mIndexRange;
    }

    const State &state = mContext->getGLState();

    const gl::VertexArray *vao     = state.getVertexArray();
    gl::Buffer *elementArrayBuffer = vao->getElementArrayBuffer().get();

    if (elementArrayBuffer)
    {
        uintptr_t offset = reinterpret_cast<uintptr_t>(mIndices);
        IndexRange indexRange;
        Error error =
            elementArrayBuffer->getIndexRange(mContext, mType, static_cast<size_t>(offset), mCount,
                                              state.isPrimitiveRestartEnabled(), &indexRange);
        if (error.isError())
        {
            mContext->handleError(error);
            return mIndexRange;
        }

        mIndexRange = indexRange;
    }
    else
    {
        mIndexRange = ComputeIndexRange(mType, mIndices, mCount, state.isPrimitiveRestartEnabled());
    }

    return mIndexRange;
}

// DrawCallParams implementation.
// Called by DrawArrays.
DrawCallParams::DrawCallParams(GLenum mode,
                               GLint firstVertex,
                               GLsizei vertexCount,
                               GLsizei instances)
    : mMode(mode),
      mHasIndexRange(nullptr),
      mFirstVertex(firstVertex),
      mVertexCount(vertexCount),
      mIndexCount(0),
      mBaseVertex(0),
      mType(GL_NONE),
      mIndices(nullptr),
      mInstances(instances),
      mIndirect(nullptr)
{
}

// Called by DrawElements.
DrawCallParams::DrawCallParams(GLenum mode,
                               const HasIndexRange &hasIndexRange,
                               GLint indexCount,
                               GLenum type,
                               const void *indices,
                               GLint baseVertex,
                               GLsizei instances)
    : mMode(mode),
      mHasIndexRange(&hasIndexRange),
      mFirstVertex(0),
      mVertexCount(0),
      mIndexCount(indexCount),
      mBaseVertex(baseVertex),
      mType(type),
      mIndices(indices),
      mInstances(instances),
      mIndirect(nullptr)
{
}

// Called by DrawArraysIndirect.
DrawCallParams::DrawCallParams(GLenum mode, const void *indirect)
    : mMode(mode),
      mHasIndexRange(nullptr),
      mFirstVertex(0),
      mVertexCount(0),
      mIndexCount(0),
      mBaseVertex(0),
      mType(GL_NONE),
      mIndices(nullptr),
      mInstances(0),
      mIndirect(indirect)
{
}

// Called by DrawElementsIndirect.
DrawCallParams::DrawCallParams(GLenum mode, GLenum type, const void *indirect)
    : mMode(mode),
      mHasIndexRange(nullptr),
      mFirstVertex(0),
      mVertexCount(0),
      mIndexCount(0),
      mBaseVertex(0),
      mType(type),
      mIndices(nullptr),
      mInstances(0),
      mIndirect(indirect)
{
}

GLenum DrawCallParams::mode() const
{
    return mMode;
}

GLint DrawCallParams::firstVertex() const
{
    // In some cases we can know the first vertex will be fixed at zero, if we're on the "fast
    // path". In these cases the index range is not resolved. If the first vertex is not zero,
    // however, then it must be because the index range is resolved. This only applies to the
    // D3D11 back-end currently.
    ASSERT(mFirstVertex == 0 || mHasIndexRange == nullptr);
    return mFirstVertex;
}

GLsizei DrawCallParams::vertexCount() const
{
    ASSERT(!mHasIndexRange);
    return mVertexCount;
}

GLsizei DrawCallParams::indexCount() const
{
    ASSERT(isDrawElements());
    return mIndexCount;
}

GLint DrawCallParams::baseVertex() const
{
    return mBaseVertex;
}

GLenum DrawCallParams::type() const
{
    ASSERT(isDrawElements());
    return mType;
}

const void *DrawCallParams::indices() const
{
    return mIndices;
}

GLsizei DrawCallParams::instances() const
{
    return mInstances;
}

const void *DrawCallParams::indirect() const
{
    return mIndirect;
}

bool DrawCallParams::isDrawElements() const
{
    return (mType != GL_NONE);
}

void DrawCallParams::ensureIndexRangeResolved() const
{
    if (mHasIndexRange == nullptr)
    {
        return;
    }

    // This call will resolve the index range.
    const gl::IndexRange &indexRange = mHasIndexRange->getIndexRange().value();

    mFirstVertex   = mBaseVertex + static_cast<GLint>(indexRange.start);
    mVertexCount   = static_cast<GLsizei>(indexRange.vertexCount());
    mHasIndexRange = nullptr;
}

}  // namespace gl
