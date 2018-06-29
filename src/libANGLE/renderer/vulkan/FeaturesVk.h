//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_helpers:
//   Optional features for the Vulkan renderer.

#ifndef LIBANGLE_RENDERER_VULKAN_FEATURESVK_H_
#define LIBANGLE_RENDERER_VULKAN_FEATURESVK_H_

namespace rx
{
struct FeaturesVk
{
    // Line segment rasterization must follow OpenGL rules. This means using an algorithm similar
    // to Bresenham's. Vulkan uses a different algorithm. This feature enables the use of pixel
    // shader patching to implement OpenGL basic line rasterization rules. This feature will
    // normally always be enabled. Exposing it as an option enables performance testing.
    bool basicGLLineRasterization = false;
};
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_FEATURESVK_H_