//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueue.cpp: Implements the cl::CommandQueue class.

#include "libANGLE/CLCommandQueue.h"

namespace cl
{

CommandQueue::CommandQueue(const cl_icd_dispatch &dispatch) : _cl_command_queue(dispatch) {}

}  // namespace cl
