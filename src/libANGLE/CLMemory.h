//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLMemory.h: Defines the cl::Memory class, which is a memory object and represents OpenCL objects
// such as buffers, images and pipes.

#ifndef LIBANGLE_CLMEMORY_H_
#define LIBANGLE_CLMEMORY_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class Memory final : public _cl_mem, public Object
{
  public:
    Memory(const cl_icd_dispatch &dispatch);
    ~Memory() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLMEMORY_H_
