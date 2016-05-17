//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextImpl:
//   Implementation-specific functionality associated with a GL Context.
//

#include "libANGLE/renderer/ContextImpl.h"

namespace rx
{

ContextImpl::ContextImpl(const gl::ContextState &state) : mState(state)
{
}

ContextImpl::~ContextImpl()
{
}

}  // namespace rx
