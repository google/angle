//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_utils:
//    Helper functions for the Vulkan Caps.
//

#ifndef LIBANGLE_RENDERER_VULKAN_VK_CAPS_UTILS_H_
#define LIBANGLE_RENDERER_VULKAN_VK_CAPS_UTILS_H_

#include <vulkan/vulkan.h>

namespace gl
{
struct Limitations;
struct Extensions;
class TextureCapsMap;
struct Caps;
struct TextureCaps;
}

namespace rx
{

namespace vk
{
class FormatTable;

void GenerateCaps(const VkPhysicalDeviceProperties &physicalDeviceProperties,
                  const gl::TextureCapsMap &textureCaps,
                  gl::Caps *outCaps,
                  gl::Extensions *outExtensions,
                  gl::Limitations * /* outLimitations */);

gl::TextureCaps GenerateTextureFormatCaps(const VkFormatProperties &formatProperties);
}  // namespace vk
}  // namespace rx

#endif