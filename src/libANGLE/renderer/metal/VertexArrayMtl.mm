//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VertexArrayMtl.mm:
//    Implements the class methods for VertexArrayMtl.
//

#include "libANGLE/renderer/metal/VertexArrayMtl.h"

#include "common/debug.h"

namespace rx
{

// VertexArrayMtl implementation
VertexArrayMtl::VertexArrayMtl(const gl::VertexArrayState &state) : VertexArrayImpl(state)
{
    UNIMPLEMENTED();
}
VertexArrayMtl::~VertexArrayMtl() {}

void VertexArrayMtl::destroy(const gl::Context *context)
{
    UNIMPLEMENTED();
}

angle::Result VertexArrayMtl::syncState(const gl::Context *context,
                                        const gl::VertexArray::DirtyBits &dirtyBits,
                                        gl::VertexArray::DirtyAttribBitsArray *attribBits,
                                        gl::VertexArray::DirtyBindingBitsArray *bindingBits)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

}
