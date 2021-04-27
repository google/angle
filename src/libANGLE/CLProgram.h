//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgram.h: Defines the cl::Program class, which consists of a set of OpenCL kernels.

#ifndef LIBANGLE_CLPROGRAM_H_
#define LIBANGLE_CLPROGRAM_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class Program final : public _cl_program, public Object
{
  public:
    Program(const cl_icd_dispatch &dispatch);
    ~Program() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLPROGRAM_H_
