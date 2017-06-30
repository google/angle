//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Contants.h: Defines some implementation specific and gl constants

#ifndef LIBANGLE_CONSTANTS_H_
#define LIBANGLE_CONSTANTS_H_

#include "common/platform.h"

namespace gl
{

// The size to set for the program cache for default and low-end device cases.
// Temporarily disabled to prevent double memory use in Chrome.
// TODO(jmadill): Re-enable once we have memory cache control.
//#if !defined(ANGLE_PLATFORM_ANDROID)
// const size_t kDefaultMaxProgramCacheMemoryBytes = 6 * 1024 * 1024;
//#else
// const size_t kDefaultMaxProgramCacheMemoryBytes = 2 * 1024 * 1024;
// const size_t kLowEndMaxProgramCacheMemoryBytes  = 512 * 1024;
//#endif
const size_t kDefaultMaxProgramCacheMemoryBytes = 0;

enum
{
    MAX_VERTEX_ATTRIBS         = 16,
    MAX_VERTEX_ATTRIB_BINDINGS = 16,

    // Implementation upper limits, real maximums depend on the hardware
    IMPLEMENTATION_MAX_VARYING_VECTORS = 32,
    IMPLEMENTATION_MAX_DRAW_BUFFERS    = 8,
    IMPLEMENTATION_MAX_FRAMEBUFFER_ATTACHMENTS =
        IMPLEMENTATION_MAX_DRAW_BUFFERS + 2,  // 2 extra for depth and/or stencil buffers

    IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS   = 16,
    IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS = 16,
    IMPLEMENTATION_MAX_COMBINED_SHADER_UNIFORM_BUFFERS =
        IMPLEMENTATION_MAX_VERTEX_SHADER_UNIFORM_BUFFERS +
        IMPLEMENTATION_MAX_FRAGMENT_SHADER_UNIFORM_BUFFERS,

    IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS = 4,

    // These are the maximums the implementation can support
    // The actual GL caps are limited by the device caps
    // and should be queried from the Context
    IMPLEMENTATION_MAX_2D_TEXTURE_SIZE         = 16384,
    IMPLEMENTATION_MAX_CUBE_MAP_TEXTURE_SIZE   = 16384,
    IMPLEMENTATION_MAX_3D_TEXTURE_SIZE         = 2048,
    IMPLEMENTATION_MAX_2D_ARRAY_TEXTURE_LAYERS = 2048,

    IMPLEMENTATION_MAX_TEXTURE_LEVELS = 15  // 1+log2 of MAX_TEXTURE_SIZE
};
}

#endif // LIBANGLE_CONSTANTS_H_
