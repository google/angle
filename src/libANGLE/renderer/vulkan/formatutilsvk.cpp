//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// formatutilsvk:
//   Helper for Vulkan format code.

#include "libANGLE/renderer/vulkan/formatutilsvk.h"

#include "libANGLE/renderer/load_functions_table.h"

namespace rx
{

namespace vk
{

const angle::Format &Format::format() const
{
    return angle::Format::Get(formatID);
}

LoadFunctionMap Format::getLoadFunctions() const
{
    return GetLoadFunctionsMap(internalFormat, formatID);
}

}  // namespace vk

}  // namespace rx
