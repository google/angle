//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TransformFeedbackMtl.mm:
//    Defines the class interface for TransformFeedbackMtl, implementing TransformFeedbackImpl.
//

#include "libANGLE/renderer/metal/TransformFeedbackMtl.h"

#include "libANGLE/Context.h"
#include "libANGLE/Query.h"

#include "common/debug.h"

namespace rx
{

TransformFeedbackMtl::TransformFeedbackMtl(const gl::TransformFeedbackState &state)
    : TransformFeedbackImpl(state)
{}

TransformFeedbackMtl::~TransformFeedbackMtl() {}

angle::Result TransformFeedbackMtl::begin(const gl::Context *context,
                                          gl::PrimitiveMode primitiveMode)
{
    UNIMPLEMENTED();

    return angle::Result::Continue;
}

angle::Result TransformFeedbackMtl::end(const gl::Context *context)
{
    UNIMPLEMENTED();

    return angle::Result::Continue;
}

angle::Result TransformFeedbackMtl::pause(const gl::Context *context)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TransformFeedbackMtl::resume(const gl::Context *context)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TransformFeedbackMtl::bindIndexedBuffer(
    const gl::Context *context,
    size_t index,
    const gl::OffsetBindingPointer<gl::Buffer> &binding)
{
    UNIMPLEMENTED();

    return angle::Result::Continue;
}

}  // namespace rx
