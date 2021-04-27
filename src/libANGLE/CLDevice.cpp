//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDevice.cpp: Implements the cl::Device class.

#include "libANGLE/CLDevice.h"

namespace cl
{

Device::Device(const cl_icd_dispatch &dispatch) : _cl_device_id(dispatch) {}

}  // namespace cl
