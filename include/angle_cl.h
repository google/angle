//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// angle_cl.h: Includes all necessary CL headers and definitions for ANGLE.

#ifndef ANGLECL_H_
#define ANGLECL_H_

#define CL_TARGET_OPENCL_VERSION 300
#define CL_USE_DEPRECATED_OPENCL_1_0_APIS
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define CL_USE_DEPRECATED_OPENCL_2_0_APIS
#define CL_USE_DEPRECATED_OPENCL_2_1_APIS
#define CL_USE_DEPRECATED_OPENCL_2_2_APIS

#include "CL/cl_icd.h"

#include <cstddef>
#include <type_traits>

namespace cl
{

using ContextErrorCB = void(CL_CALLBACK *)(const char *errinfo,
                                           const void *private_info,
                                           size_t cb,
                                           void *user_data);

template <typename CLObjectType>
struct Dispatch
{
    constexpr Dispatch(const cl_icd_dispatch &dispatch) : mDispatch(&dispatch)
    {
        static_assert(
            std::is_standard_layout<CLObjectType>::value && offsetof(CLObjectType, mDispatch) == 0u,
            "Not ICD compatible");
    }
    ~Dispatch() = default;

    constexpr const cl_icd_dispatch &getDispatch() { return *mDispatch; }

  private:
    // This has to be the first member to be OpenCL ICD compatible
    const cl_icd_dispatch *const mDispatch;
};

}  // namespace cl

struct _cl_platform_id : public cl::Dispatch<_cl_platform_id>
{
    constexpr _cl_platform_id(const cl_icd_dispatch &dispatch)
        : cl::Dispatch<_cl_platform_id>(dispatch)
    {}
    ~_cl_platform_id() = default;
};

struct _cl_device_id : public cl::Dispatch<_cl_device_id>
{
    constexpr _cl_device_id(const cl_icd_dispatch &dispatch) : cl::Dispatch<_cl_device_id>(dispatch)
    {}
    ~_cl_device_id() = default;
};

struct _cl_context : public cl::Dispatch<_cl_context>
{
    constexpr _cl_context(const cl_icd_dispatch &dispatch) : cl::Dispatch<_cl_context>(dispatch) {}
    ~_cl_context() = default;
};

struct _cl_command_queue : public cl::Dispatch<_cl_command_queue>
{
    constexpr _cl_command_queue(const cl_icd_dispatch &dispatch)
        : cl::Dispatch<_cl_command_queue>(dispatch)
    {}
    ~_cl_command_queue() = default;
};

struct _cl_mem : public cl::Dispatch<_cl_mem>
{
    constexpr _cl_mem(const cl_icd_dispatch &dispatch) : cl::Dispatch<_cl_mem>(dispatch) {}
    ~_cl_mem() = default;
};

struct _cl_program : public cl::Dispatch<_cl_program>
{
    constexpr _cl_program(const cl_icd_dispatch &dispatch) : cl::Dispatch<_cl_program>(dispatch) {}
    ~_cl_program() = default;
};

struct _cl_kernel : public cl::Dispatch<_cl_kernel>
{
    constexpr _cl_kernel(const cl_icd_dispatch &dispatch) : cl::Dispatch<_cl_kernel>(dispatch) {}
    ~_cl_kernel() = default;
};

struct _cl_event : public cl::Dispatch<_cl_event>
{
    constexpr _cl_event(const cl_icd_dispatch &dispatch) : cl::Dispatch<_cl_event>(dispatch) {}
    ~_cl_event() = default;
};

struct _cl_sampler : public cl::Dispatch<_cl_sampler>
{
    constexpr _cl_sampler(const cl_icd_dispatch &dispatch) : cl::Dispatch<_cl_sampler>(dispatch) {}
    ~_cl_sampler() = default;
};

#endif  // ANGLECL_H_
