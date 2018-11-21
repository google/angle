//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_internal_shaders.h:
//   Pre-generated shader library for the ANGLE Vulkan back-end.

#ifndef LIBANGLE_RENDERER_VULKAN_VK_INTERNAL_SHADERS_H_
#define LIBANGLE_RENDERER_VULKAN_VK_INTERNAL_SHADERS_H_

#include "common/PackedEnums.h"
#include "libANGLE/renderer/vulkan/vk_internal_shaders_autogen.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace rx
{
namespace vk
{
class ShaderLibrary final : angle::NonCopyable
{
  public:
    ShaderLibrary();
    ~ShaderLibrary();

    angle::Result getShader(Context *context,
                            InternalShaderID shaderID,
                            RefCounted<ShaderAndSerial> **shaderOut);
    void destroy(VkDevice device);

  private:
    angle::PackedEnumMap<InternalShaderID, RefCounted<ShaderAndSerial>> mShaders;
};
}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_INTERNAL_SHADERS_H_
