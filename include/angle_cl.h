//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// angle_cl.h:
//   Includes all necessary CL headers and definitions for ANGLE.
//

#ifndef ANGLECL_H_
#define ANGLECL_H_

// Workaround: The OpenCL headers use 'intptr_t' but don't include 'stdint.h' on Windows.
// TODO(jplate): Remove after the CL headers are fixed.
#include <stdint.h>

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS

#include "CL/opencl.h"

#endif  // ANGLECL_H_
