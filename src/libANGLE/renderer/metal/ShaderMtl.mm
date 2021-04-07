//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderMtl.mm:
//    Implements the class methods for ShaderMtl.
//

#include "libANGLE/renderer/metal/ShaderMtl.h"

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/metal/ContextMtl.h"
#include "libANGLE/renderer/metal/DisplayMtl.h"

namespace rx
{

ShaderMtl::ShaderMtl(const gl::ShaderState &state) : ShaderImpl(state) {}

ShaderMtl::~ShaderMtl() {}

std::shared_ptr<WaitableCompileEvent> ShaderMtl::compile(const gl::Context *context,
                                                         gl::ShCompilerInstance *compilerInstance,
                                                         ShCompileOptions options)
{
    ContextMtl *contextMtl = mtl::GetImpl(context);

    ShCompileOptions compileOptions = SH_INITIALIZE_UNINITIALIZED_LOCALS;

    bool isWebGL = context->getExtensions().webglCompatibility;
    if (isWebGL && mState.getShaderType() != gl::ShaderType::Compute)
    {
        compileOptions |= SH_INIT_OUTPUT_VARIABLES;
    }

    compileOptions |= SH_CLAMP_POINT_SIZE;

    // Transform feedback is always emulated on Metal.
    compileOptions |= SH_ADD_VULKAN_XFB_EMULATION_SUPPORT_CODE;

    if (contextMtl->getDisplay()->getFeatures().directSPIRVGeneration.enabled)
    {
        compileOptions |= SH_GENERATE_SPIRV_DIRECTLY;
    }

    return compileImpl(context, compilerInstance, mState.getSource(), compileOptions | options);
}

std::string ShaderMtl::getDebugInfo() const
{
    return mState.getCompiledBinary().empty() ? "" : "<binary blob>";
}

}  // namespace rx
