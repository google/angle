//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContext.cpp: Implements the cl::Context class.

#include "libANGLE/CLContext.h"

namespace cl
{

Context::Context(const cl_icd_dispatch &dispatch) : _cl_context(dispatch) {}

}  // namespace cl
