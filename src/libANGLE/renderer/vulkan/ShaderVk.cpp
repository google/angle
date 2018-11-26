//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderVk.cpp:
//    Implements the class methods for ShaderVk.
//

#include "libANGLE/renderer/vulkan/ShaderVk.h"

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "platform/FeaturesVk.h"

namespace rx
{

ShaderVk::ShaderVk(const gl::ShaderState &data) : ShaderImpl(data)
{
}

ShaderVk::~ShaderVk()
{
}

ShCompileOptions ShaderVk::prepareSourceAndReturnOptions(const gl::Context *context,
                                                         std::stringstream *sourceStream,
                                                         std::string *sourcePath)
{
    *sourceStream << mData.getSource();

    ShCompileOptions compileOptions = SH_INITIALIZE_UNINITIALIZED_LOCALS;

    ContextVk *contextVk = vk::GetImpl(context);

    if (contextVk->getFeatures().clampPointSize)
    {
        compileOptions |= SH_CLAMP_POINT_SIZE;
    }

    return compileOptions;
}

bool ShaderVk::postTranslateCompile(gl::ShCompilerInstance *compiler, std::string *infoLog)
{
    // No work to do here.
    return true;
}

std::string ShaderVk::getDebugInfo() const
{
    return mData.getTranslatedSource();
}

}  // namespace rx
