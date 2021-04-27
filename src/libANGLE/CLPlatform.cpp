//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.cpp: Implements the cl::Platform class.

#include "libANGLE/CLPlatform.h"

namespace cl
{

Platform::Platform(const cl_icd_dispatch &dispatch) : _cl_platform_id(dispatch) {}

}  // namespace cl
