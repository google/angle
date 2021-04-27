//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgram.cpp: Implements the cl::Program class.

#include "libANGLE/CLProgram.h"

namespace cl
{

Program::Program(const cl_icd_dispatch &dispatch) : _cl_program(dispatch) {}

}  // namespace cl
