//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DriverUniforms.h:
//    Defines the interface for DriverUniforms
//

#ifndef LIBANGLE_RENDERER_VULKAN_DRIVER_UNIFORMS_H_
#define LIBANGLE_RENDERER_VULKAN_DRIVER_UNIFORMS_H_

#include "GLSLANG/ShaderLang.h"
#include "common/PackedEnums.h"
#include "common/angleutils.h"
#include "libANGLE/RefCountObject.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/vulkan/vk_renderer.h"

namespace rx
{
struct GraphicsDriverUniforms
{
    // Contain packed 8-bit values for atomic counter buffer offsets.  These offsets are within
    // Vulkan's minStorageBufferOffsetAlignment limit and are used to support unaligned offsets
    // allowed in GL.
    std::array<uint32_t, 2> acbBufferOffsets;

    // .x is near, .y is far
    std::array<float, 2> depthRange;

    // Used to flip gl_FragCoord.  Packed uvec2
    uint32_t renderArea;

    // Packed vec4 of snorm8
    uint32_t flipXY;

    // Only the lower 16 bits used
    uint32_t dither;

    // Various bits of state:
    // - Surface rotation
    // - Advanced blend equation
    // - Sample count
    // - Enabled clip planes
    // - Depth transformation
    // - layered FBO
    uint32_t misc;
};
static_assert(sizeof(GraphicsDriverUniforms) % (sizeof(uint32_t) * 4) == 0,
              "GraphicsDriverUniforms should be 16bytes aligned");

// Only used when transform feedback is emulated.
struct XFBEmulationGraphicsDriverUniforms
{
    std::array<int32_t, 4> xfbBufferOffsets;
    int32_t xfbVerticesPerInstance;
    int32_t padding[3];
};
static_assert(sizeof(XFBEmulationGraphicsDriverUniforms) % (sizeof(uint32_t) * 4) == 0,
              "GraphicsDriverUniformsExtended should be 16bytes aligned");
// Driver uniforms are updated using push constants and Vulkan spec guarantees universal support for
// 128 bytes worth of push constants. For maximum compatibility ensure
// GraphicsDriverUniforms plus extended size are within that limit.
static_assert(sizeof(GraphicsDriverUniforms) + sizeof(XFBEmulationGraphicsDriverUniforms) <= 128,
              "Only 128 bytes are guaranteed for push constants");

struct ComputeDriverUniforms
{
    // Atomic counter buffer offsets with the same layout as in GraphicsDriverUniforms.
    std::array<uint32_t, 4> acbBufferOffsets;
};
}  // namespace rx
#endif  // LIBANGLE_RENDERER_VULKAN_DRIVER_UNIFORMS_H_
