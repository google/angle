//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CompilerMtl.mm:
//    Implements the class methods for CompilerMtl.
//

#include "libANGLE/renderer/metal/CompilerMtl.h"

#include "common/debug.h"
#include "common/system_utils.h"

namespace rx
{

CompilerMtl::CompilerMtl() : CompilerImpl() {}

CompilerMtl::~CompilerMtl() {}

ShShaderOutput CompilerMtl::getTranslatorOutputType() const
{
#if ANGLE_ENABLE_METAL_SPIRV
    if (useDirectToMSLCompiler())
    {
        return SH_MSL_METAL_OUTPUT;
    }
    else
    {
        return SH_SPIRV_METAL_OUTPUT;
    }
#else
    return SH_MSL_METAL_OUTPUT;
#endif
}

bool CompilerMtl::useDirectToMSLCompiler()
{
    return false;
}

}  // namespace rx
