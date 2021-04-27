//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// libOpenCL_ICD.cpp: Implements the CL entry points required for extension cl_khr_icd.

#include "libOpenCL/dispatch.h"

extern "C" {

cl_int CL_API_CALL clIcdGetPlatformIDsKHR(cl_uint num_entries,
                                          cl_platform_id *platforms,
                                          cl_uint *num_platforms)
{
    return cl::GetDispatch().clGetPlatformIDs(num_entries, platforms, num_platforms);
}

cl_int CL_API_CALL clGetPlatformInfo(cl_platform_id platform,
                                     cl_platform_info param_name,
                                     size_t param_value_size,
                                     void *param_value,
                                     size_t *param_value_size_ret)
{
    return cl::GetDispatch().clGetPlatformInfo(platform, param_name, param_value_size, param_value,
                                               param_value_size_ret);
}

void *CL_API_CALL clGetExtensionFunctionAddress(const char *func_name)
{
    return cl::GetDispatch().clGetExtensionFunctionAddress(func_name);
}

}  // extern "C"
