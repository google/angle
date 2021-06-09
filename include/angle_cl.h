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

using MemoryCB  = void(CL_CALLBACK *)(cl_mem memobj, void *user_data);
using ProgramCB = void(CL_CALLBACK *)(cl_program program, void *user_data);
using EventCB   = void(CL_CALLBACK *)(cl_event event, cl_int event_command_status, void *user_data);
using UserFunc  = void(CL_CALLBACK *)(void *args);

template <typename T = void>
struct Dispatch
{
    Dispatch() : mDispatch(sDispatch) {}

    const cl_icd_dispatch &getDispatch() const { return *mDispatch; }

    bool isValid() const { return mDispatch == sDispatch; }

    static bool IsValid(const Dispatch *p) { return p != nullptr && p->isValid(); }

    static const cl_icd_dispatch *sDispatch;

  protected:
    // This has to be the first member to be OpenCL ICD compatible
    const cl_icd_dispatch *const mDispatch;
};

template <typename T>
const cl_icd_dispatch *Dispatch<T>::sDispatch = nullptr;

template <typename NativeObjectType>
struct NativeObject : public Dispatch<>
{
    NativeObject()
    {
        static_assert(std::is_standard_layout<NativeObjectType>::value &&
                          offsetof(NativeObjectType, mDispatch) == 0u,
                      "Not ICD compatible");
    }

    template <typename T>
    T &cast()
    {
        return static_cast<T &>(*this);
    }

    template <typename T>
    const T &cast() const
    {
        return static_cast<const T &>(*this);
    }

    NativeObjectType *getNative() { return static_cast<NativeObjectType *>(this); }

    const NativeObjectType *getNative() const
    {
        return static_cast<const NativeObjectType *>(this);
    }

    static NativeObjectType *CastNative(NativeObjectType *p) { return p; }
};

}  // namespace cl

struct _cl_platform_id : public cl::NativeObject<_cl_platform_id>
{};

struct _cl_device_id : public cl::NativeObject<_cl_device_id>
{};

struct _cl_context : public cl::NativeObject<_cl_context>
{};

struct _cl_command_queue : public cl::NativeObject<_cl_command_queue>
{};

struct _cl_mem : public cl::NativeObject<_cl_mem>
{};

struct _cl_program : public cl::NativeObject<_cl_program>
{};

struct _cl_kernel : public cl::NativeObject<_cl_kernel>
{};

struct _cl_event : public cl::NativeObject<_cl_event>
{};

struct _cl_sampler : public cl::NativeObject<_cl_sampler>
{};

#endif  // ANGLECL_H_
