//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDevice.h: Defines the cl::Device class, which provides information about OpenCL device
// configurations.

#ifndef LIBANGLE_CLDEVICE_H_
#define LIBANGLE_CLDEVICE_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class Device final : public _cl_device_id, public Object
{
  public:
    Device(const cl_icd_dispatch &dispatch);
    ~Device() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLDEVICE_H_
