//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_internal_shaders.cpp:
//   Pre-generated shader library for the ANGLE Vulkan back-end.

#include "libANGLE/renderer/vulkan/vk_internal_shaders.h"

#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{
namespace vk
{
ShaderLibrary::ShaderLibrary()
{
}

ShaderLibrary::~ShaderLibrary()
{
}

void ShaderLibrary::destroy(VkDevice device)
{
    for (ShaderAndSerial &shader : mShaders)
    {
        shader.get().destroy(device);
    }
}

Error ShaderLibrary::getShader(RendererVk *renderer,
                               InternalShaderID shaderID,
                               const ShaderAndSerial **shaderOut)
{
    ShaderAndSerial &shader = mShaders[shaderID];
    *shaderOut              = &shader;

    if (shader.get().valid())
    {
        return NoError();
    }

    const priv::ShaderBlob &shaderCode = priv::GetInternalShaderBlob(shaderID);

    // Create shader lazily. Access will need to be locked for multi-threading.
    VkShaderModuleCreateInfo createInfo;
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext    = nullptr;
    createInfo.flags    = 0;
    createInfo.codeSize = shaderCode.codeSize;
    createInfo.pCode    = shaderCode.code;

    ANGLE_TRY(shader.get().init(renderer->getDevice(), createInfo));
    shader.updateSerial(renderer->issueShaderSerial());
    return NoError();
}
}  // namespace vk
}  // namespace rx
