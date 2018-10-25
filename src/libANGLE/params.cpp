//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// params:
//   Parameter wrapper structs for OpenGL ES. These helpers cache re-used values
//   in entry point routines.

#include "libANGLE/params.h"

#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/VertexArray.h"

namespace gl
{
// static
constexpr ParamTypeInfo ParamsBase::TypeInfo;
}  // namespace gl
