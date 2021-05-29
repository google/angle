//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueueCL.h: Defines the class interface for CLCommandQueueCL,
// implementing CLCommandQueueImpl.

#ifndef LIBANGLE_RENDERER_CL_CLCOMMANDQUEUECL_H_
#define LIBANGLE_RENDERER_CL_CLCOMMANDQUEUECL_H_

#include "libANGLE/renderer/CLCommandQueueImpl.h"

namespace rx
{

class CLCommandQueueCL : public CLCommandQueueImpl
{
  public:
    CLCommandQueueCL(const cl::CommandQueue &commandQueue, cl_command_queue native);
    ~CLCommandQueueCL() override;

    cl_int setProperty(cl::CommandQueueProperties properties, cl_bool enable) override;

  private:
    const cl_command_queue mNative;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLCOMMANDQUEUECL_H_
