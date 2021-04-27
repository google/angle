//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.h: Defines the cl::Platform class, which provides information about platform-specific
// OpenCL features.

#ifndef LIBANGLE_CLPLATFORM_H_
#define LIBANGLE_CLPLATFORM_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class Platform final : public _cl_platform_id, public Object
{
  public:
    Platform(const cl_icd_dispatch &dispatch);
    ~Platform() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLPLATFORM_H_
