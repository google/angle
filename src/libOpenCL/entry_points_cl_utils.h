//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// entry_points_cl_utils.h:
//   These helpers are used in CL entry point routines.

#ifndef LIBOPENCL_ENTRY_POINTS_CL_UTILS_H_
#define LIBOPENCL_ENTRY_POINTS_CL_UTILS_H_

#include <cinttypes>
#include <cstdio>

#if defined(ANGLE_TRACE_ENABLED)
#    define CL_EVENT(entryPoint, ...)                    \
        std::printf("CL " #entryPoint ": " __VA_ARGS__); \
        std::printf("\n")
#else
#    define CL_EVENT(entryPoint, ...) (void(0))
#endif

#endif  // LIBOPENCL_ENTRY_POINTS_CL_UTILS_H_
