//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// entry_points_cl_utils.h:
//   These helpers are used in CL entry point routines.

#ifndef LIBGLESV2_ENTRY_POINTS_CL_UTILS_H_
#define LIBGLESV2_ENTRY_POINTS_CL_UTILS_H_

#include "libANGLE/Debug.h"

#include <cinttypes>
#include <cstdio>
#include <type_traits>

#if defined(ANGLE_ENABLE_DEBUG_TRACE)
#    define CL_EVENT(entryPoint, ...)                    \
        std::printf("CL " #entryPoint ": " __VA_ARGS__); \
        std::printf("\n")
#else
#    define CL_EVENT(entryPoint, ...) (void(0))
#endif

namespace cl
{

// First case: handling packed enums.
template <typename PackedT, typename FromT>
typename std::enable_if_t<std::is_enum<PackedT>::value, PackedT> PackParam(FromT from)
{
    return FromCLenum<PackedT>(from);
}

// Cast CL object types to ANGLE CL object types
template <typename PackedT, typename FromT>
inline std::enable_if_t<
    std::is_base_of<cl::Object, std::remove_pointer_t<std::remove_pointer_t<PackedT>>>::value,
    PackedT>
PackParam(FromT from)
{
    return reinterpret_cast<PackedT>(from);
}

}  // namespace cl

#endif  // LIBGLESV2_ENTRY_POINTS_CL_UTILS_H_
