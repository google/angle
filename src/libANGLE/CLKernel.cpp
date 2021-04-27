//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLKernel.cpp: Implements the cl::Kernel class.

#include "libANGLE/CLKernel.h"

namespace cl
{

Kernel::Kernel(const cl_icd_dispatch &dispatch) : _cl_kernel(dispatch) {}

}  // namespace cl
