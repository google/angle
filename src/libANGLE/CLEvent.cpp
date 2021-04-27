//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLEvent.cpp: Implements the cl::Event class.

#include "libANGLE/CLEvent.h"

namespace cl
{

Event::Event(const cl_icd_dispatch &dispatch) : _cl_event(dispatch) {}

}  // namespace cl
