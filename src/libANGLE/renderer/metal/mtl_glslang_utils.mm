//
// Copyright (c) 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// GlslangUtils: Wrapper for Khronos's glslang compiler.
//

#include "libANGLE/renderer/metal/mtl_glslang_utils.h"

#include "libANGLE/renderer/glslang_wrapper_utils.h"

namespace rx
{
namespace mtl
{

// static
void GlslangUtils::GetShaderSource(const gl::ProgramState &programState,
                                   const gl::ProgramLinkedResources &resources,
                                   gl::ShaderMap<std::string> *shaderSourcesOut)
{
    UNIMPLEMENTED();
}

// static
angle::Result GlslangUtils::GetShaderCode(ErrorHandler *context,
                                          const gl::Caps &glCaps,
                                          bool enableLineRasterEmulation,
                                          const gl::ShaderMap<std::string> &shaderSources,
                                          gl::ShaderMap<std::vector<uint32_t>> *shaderCodeOut)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
}  // namespace mtl
}  // namespace rx
