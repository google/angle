//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexArrayGL.cpp: Implements the class methods for VertexArrayGL.

#include "libANGLE/renderer/gl/VertexArrayGL.h"

#include "common/debug.h"

namespace rx
{

VertexArrayGL::VertexArrayGL()
    : VertexArrayImpl()
{}

VertexArrayGL::~VertexArrayGL()
{}

void VertexArrayGL::setElementArrayBuffer(const gl::Buffer *buffer)
{
    UNIMPLEMENTED();
}

void VertexArrayGL::setAttribute(size_t idx, const gl::VertexAttribute &attr)
{
    UNIMPLEMENTED();
}

void VertexArrayGL::setAttributeDivisor(size_t idx, GLuint divisor)
{
    UNIMPLEMENTED();
}

void VertexArrayGL::enableAttribute(size_t idx, bool enabledState)
{
    UNIMPLEMENTED();
}

}
