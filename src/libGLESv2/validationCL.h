//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationCL.h: Validation functions for generic CL entry point parameters

#ifndef LIBGLESV2_VALIDATIONCL_H_
#define LIBGLESV2_VALIDATIONCL_H_

#include "common/PackedCLEnums_autogen.h"

#include <type_traits>

namespace cl
{
// First case: handling packed enums.
template <typename PackedT, typename FromT>
typename std::enable_if<std::is_enum<PackedT>::value, PackedT>::type PackParam(FromT from)
{
    return FromCLenum<PackedT>(from);
}

template <typename PackedT, typename FromT>
inline typename std::enable_if<!std::is_enum<PackedT>::value,
                               typename std::remove_reference<PackedT>::type>::type
PackParam(FromT from)
{
    return reinterpret_cast<PackedT>(from);
}

// First case: handling packed enums.
template <typename UnpackedT, typename FromT>
typename std::enable_if<std::is_enum<FromT>::value, UnpackedT>::type UnpackParam(FromT from)
{
    return ToCLenum(from);
}

template <typename UnpackedT, typename FromT>
inline typename std::enable_if<!std::is_enum<FromT>::value, UnpackedT>::type UnpackParam(FromT from)
{
    return reinterpret_cast<UnpackedT>(from);
}
}  // namespace cl

#endif  // LIBGLESV2_VALIDATIONCL_H_
