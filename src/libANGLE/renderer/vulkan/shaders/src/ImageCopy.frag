//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImageCopy.frag: Copy parts of an image to another.

#version 450 core

#extension GL_EXT_samplerless_texture_functions : require

#if SrcIsFloat
#define SRC_RESOURCE(type) type
#define SrcType vec4
#elif SrcIsInt
#define SRC_RESOURCE(type) i ## type
#define SrcType ivec4
#elif SrcIsUint
#define SRC_RESOURCE(type) u ## type
#define SrcType uvec4
#else
#error "Not all source formats are accounted for"
#endif

#if DestIsFloat
#define DestType vec4
#elif DestIsInt
#define DestType ivec4
#elif DestIsUint
#define DestType uvec4
#else
#error "Not all destinatoin formats are accounted for"
#endif

layout(set = 0, binding = 0) uniform SRC_RESOURCE(texture2D) src;
layout(location = 0) out DestType dest;

layout(push_constant) uniform PushConstants {
    // Whether y needs to be flipped
    bool flipY;
    // Whether destination is emulated luminance/alpha.
    bool destHasLuminance;
    bool destIsAlpha;
    // Translation from source to destination coordinates.
    int srcMip;
    ivec2 srcOffset;
    ivec2 destOffset;
} params;

void main()
{
    ivec2 destSubImageCoords = ivec2(gl_FragCoord.xy) - params.destOffset;

    ivec2 srcSubImageCoords = destSubImageCoords;

    // If flipping Y, srcOffset would contain the opposite y coordinate, so we can
    // simply reverse the direction in which y grows.
    if (params.flipY)
        srcSubImageCoords.y = -srcSubImageCoords.y;

    SrcType srcValue = texelFetch(src, params.srcOffset + srcSubImageCoords, params.srcMip);

    // Convert value to destination type.
    DestType destValue = DestType(srcValue);

    // If dest is luminance/alpha, it's implemented with R or RG.  Do the appropriate swizzle.
    if (params.destHasLuminance)
    {
        destValue.rg = destValue.ra;
    }
    else if (params.destIsAlpha)
    {
        destValue.r = destValue.a;
    }

    dest = destValue;
}
