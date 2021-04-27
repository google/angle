//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLKernel.h: Defines the cl::Kernel class, which is a function declared in an OpenCL program.

#ifndef LIBANGLE_CLKERNEL_H_
#define LIBANGLE_CLKERNEL_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class Kernel final : public _cl_kernel, public Object
{
  public:
    Kernel(const cl_icd_dispatch &dispatch);
    ~Kernel() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLKERNEL_H_
