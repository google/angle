//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueueImpl.h: Defines the abstract rx::CLCommandQueueImpl class.

#ifndef LIBANGLE_RENDERER_CLCOMMANDQUEUEIMPL_H_
#define LIBANGLE_RENDERER_CLCOMMANDQUEUEIMPL_H_

#include "libANGLE/renderer/CLtypes.h"

namespace rx
{

class CLCommandQueueImpl : angle::NonCopyable
{
  public:
    using Ptr = std::unique_ptr<CLCommandQueueImpl>;

    CLCommandQueueImpl(const cl::CommandQueue &commandQueue);
    virtual ~CLCommandQueueImpl();

    virtual cl_int setProperty(cl::CommandQueueProperties properties, cl_bool enable) = 0;

  protected:
    const cl::CommandQueue &mCommandQueue;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLCOMMANDQUEUEIMPL_H_
