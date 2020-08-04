//
// Copyright 2019 The ANGLE Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// common.h: Common header for other metal source code.

#ifndef LIBANGLE_RENDERER_METAL_SHADERS_COMMON_H_
#define LIBANGLE_RENDERER_METAL_SHADERS_COMMON_H_

#ifndef SKIP_STD_HEADERS
#    include <simd/simd.h>
#    include <metal_stdlib>
#endif

#include "constants.h"

#define ANGLE_KERNEL_GUARD(IDX, MAX_COUNT) \
    if (IDX >= MAX_COUNT)                  \
    {                                      \
        return;                            \
    }

using namespace metal;

// Common constant defined number of color outputs
constant uint32_t kNumColorOutputs [[function_constant(0)]];
constant bool kColorOutputAvailable0 = kNumColorOutputs > 0;
constant bool kColorOutputAvailable1 = kNumColorOutputs > 1;
constant bool kColorOutputAvailable2 = kNumColorOutputs > 2;
constant bool kColorOutputAvailable3 = kNumColorOutputs > 3;

namespace rx
{
namespace mtl_shader
{

// Full screen triangle's vertices
constant float2 gCorners[3] = {float2(-1.0f, -1.0f), float2(3.0f, -1.0f), float2(-1.0f, 3.0f)};

template <typename T>
struct MultipleColorOutputs
{
    vec<T, 4> color0 [[color(0), function_constant(kColorOutputAvailable0)]];
    vec<T, 4> color1 [[color(1), function_constant(kColorOutputAvailable1)]];
    vec<T, 4> color2 [[color(2), function_constant(kColorOutputAvailable2)]];
    vec<T, 4> color3 [[color(3), function_constant(kColorOutputAvailable3)]];
};

#define ANGLE_ASSIGN_COLOR_OUPUT(STRUCT_VARIABLE, COLOR_INDEX, VALUE) \
    do                                                                \
    {                                                                 \
        if (kColorOutputAvailable##COLOR_INDEX)                       \
        {                                                             \
            STRUCT_VARIABLE.color##COLOR_INDEX = VALUE;               \
        }                                                             \
    } while (0)

template <typename T>
static inline MultipleColorOutputs<T> toMultipleColorOutputs(vec<T, 4> color)
{
    MultipleColorOutputs<T> re;

    ANGLE_ASSIGN_COLOR_OUPUT(re, 0, color);
    ANGLE_ASSIGN_COLOR_OUPUT(re, 1, color);
    ANGLE_ASSIGN_COLOR_OUPUT(re, 2, color);
    ANGLE_ASSIGN_COLOR_OUPUT(re, 3, color);

    return re;
}

static inline float3 cubeTexcoords(float2 texcoords, int face)
{
    texcoords = 2.0 * texcoords - 1.0;
    switch (face)
    {
        case 0:
            return float3(1.0, -texcoords.y, -texcoords.x);
        case 1:
            return float3(-1.0, -texcoords.y, texcoords.x);
        case 2:
            return float3(texcoords.x, 1.0, texcoords.y);
        case 3:
            return float3(texcoords.x, -1.0, -texcoords.y);
        case 4:
            return float3(texcoords.x, -texcoords.y, 1.0);
        case 5:
            return float3(-texcoords.x, -texcoords.y, -1.0);
    }
    return float3(texcoords, 0);
}

template <typename T>
static inline vec<T, 4> resolveTextureMS(texture2d_ms<T> srcTexture, uint2 coords)
{
    uint samples = srcTexture.get_num_samples();

    vec<T, 4> output(0);

    for (uint sample = 0; sample < samples; ++sample)
    {
        output += srcTexture.read(coords, sample);
    }

    output = output / samples;

    return output;
}

static inline float4 sRGBtoLinear(float4 color)
{
    float3 linear1 = color.rgb / 12.92;
    float3 linear2 = pow((color.rgb + float3(0.055)) / 1.055, 2.4);
    float3 factor  = float3(color.rgb <= float3(0.04045));
    float4 linear  = float4(factor * linear1 + float3(1.0 - factor) * linear2, color.a);

    return linear;
}

static inline float linearToSRGB(float color)
{
    if (color <= 0.0f)
        return 0.0f;
    else if (color < 0.0031308f)
        return 12.92f * color;
    else if (color < 1.0f)
        return 1.055f * pow(color, 0.41666f) - 0.055f;
    else
        return 1.0f;
}

static inline float4 linearToSRGB(float4 color)
{
    return float4(linearToSRGB(color.r), linearToSRGB(color.g), linearToSRGB(color.b), color.a);
}

}  // namespace mtl_shader
}  // namespace rx

#endif /* LIBANGLE_RENDERER_METAL_SHADERS_COMMON_H_ */
