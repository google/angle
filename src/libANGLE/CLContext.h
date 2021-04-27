//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContext.h: Defines the cl::Context class, which manages OpenCL objects such as command-queues,
// memory, program and kernel objects and for executing kernels on one or more devices.

#ifndef LIBANGLE_CLCONTEXT_H_
#define LIBANGLE_CLCONTEXT_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class Context final : public _cl_context, public Object
{
  public:
    Context(const cl_icd_dispatch &dispatch);
    ~Context() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLCONTEXT_H_
