//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// libOpenCL_ICD.cpp: Implements the CL entry point required for extension cl_khr_icd.

#include "libOpenCL/dispatch.h"

extern "C" {

void *CL_API_CALL clGetExtensionFunctionAddress(const char *func_name)
{
    return cl::GetDispatch().clGetExtensionFunctionAddress(func_name);
}

}  // extern "C"
