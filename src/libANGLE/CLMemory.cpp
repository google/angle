//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLMemory.cpp: Implements the cl::Memory class.

#include "libANGLE/CLMemory.h"

namespace cl
{

Memory::Memory(const cl_icd_dispatch &dispatch) : _cl_mem(dispatch) {}

}  // namespace cl
