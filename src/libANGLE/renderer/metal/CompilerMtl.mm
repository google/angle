//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CompilerMtl.mm:
//    Implements the class methods for CompilerMtl.
//

#include "libANGLE/renderer/metal/CompilerMtl.h"

#include <stdio.h>

#include "common/debug.h"
#include "common/system_utils.h"

namespace rx
{

CompilerMtl::CompilerMtl() : CompilerImpl() {}

CompilerMtl::~CompilerMtl() {}

ShShaderOutput CompilerMtl::getTranslatorOutputType() const
{
#ifdef ANGLE_ENABLE_ASSERTS
    static bool outputted = false;
#endif
#if ANGLE_ENABLE_METAL_SPIRV
    if (useDirectToMSLCompiler())
    {
#    ifdef ANGLE_ENABLE_ASSERTS
        if (!outputted)
        {
            fprintf(stderr, "Using direct-to-Metal shader compiler\n");
            outputted = true;
        }
#    endif
        return SH_MSL_METAL_OUTPUT;
    }
    else
    {
#    ifdef ANGLE_ENABLE_ASSERTS
        if (!outputted)
        {
            fprintf(stderr, "Using SPIR-V Metal shader compiler\n");
            outputted = true;
        }
#    endif
        return SH_SPIRV_METAL_OUTPUT;
    }
#else
    return SH_MSL_METAL_OUTPUT;
#endif
}

bool CompilerMtl::useDirectToMSLCompiler()
{
    static bool val = angle::GetBoolEnvironmentVar("ANGLE_USE_MSL_COMPILER");
    return val;
}

}  // namespace rx
