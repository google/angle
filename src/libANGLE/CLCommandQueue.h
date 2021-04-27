//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueue.h: Defines the cl::CommandQueue class, which can be used to queue a set of OpenCL
// operations.

#ifndef LIBANGLE_CLCOMMANDQUEUE_H_
#define LIBANGLE_CLCOMMANDQUEUE_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class CommandQueue final : public _cl_command_queue, public Object
{
  public:
    CommandQueue(const cl_icd_dispatch &dispatch);
    ~CommandQueue() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLCOMMANDQUEUE_H_
