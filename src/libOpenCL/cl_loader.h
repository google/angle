//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// cl_loader.h:
//   Simple CL function loader.

#ifndef LIBCL_CL_LOADER_H_
#define LIBCL_CL_LOADER_H_

#include <export.h>

#include <CL/cl_icd.h>

ANGLE_NO_EXPORT extern cl_icd_dispatch cl_loader;

namespace angle
{
using GenericProc = void (*)();
using LoadProc    = GenericProc(CL_API_ENTRY *)(const char *);
ANGLE_NO_EXPORT void LoadCL(LoadProc loadProc);
}  // namespace angle

#endif  // LIBCL_CL_LOADER_H_
