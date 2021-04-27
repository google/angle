//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// dispatch.h: Declares a function to fetch the ANGLE OpenCL dispatch table.

#ifndef LIBOPENCL_DISPATCH_H_
#define LIBOPENCL_DISPATCH_H_

#include "export.h"

#ifndef CL_API_ENTRY
#    define CL_API_ENTRY ANGLE_EXPORT
#endif
#include "angle_cl.h"

namespace cl
{

cl_icd_dispatch &GetDispatch();

}  // namespace cl

#endif  // LIBOPENCL_DISPATCH_H_
