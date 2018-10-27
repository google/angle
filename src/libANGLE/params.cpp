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
constexpr ParamTypeInfo DrawCallParams::TypeInfo;

// Called by DrawArraysIndirect.
DrawCallParams::DrawCallParams(PrimitiveMode mode, const void *indirect)
    : mMode(mode),
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
DrawCallParams::DrawCallParams(PrimitiveMode mode, GLenum type, const void *indirect)
    : mMode(mode),
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

bool DrawCallParams::isDrawIndirect() const
{
    // This is a bit of a hack - it's quite possible for a direct call to have a zero count, but we
    // assume these calls are filtered out before they make it to this code.
    return (mIndexCount == 0 && mVertexCount == 0);
}
}  // namespace gl
