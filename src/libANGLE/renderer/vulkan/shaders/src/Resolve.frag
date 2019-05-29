//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Resolve.frag: Resolve multisampled color or depth/stencil images.

#version 450 core

#define MAKE_SRC_RESOURCE(prefix, type) prefix ## type

#if ResolveColorFloat

#define IsResolveColor 1
#define COLOR_SRC_RESOURCE(type) type
#define ColorType vec4

#elif ResolveColorInt

#define IsResolveColor 1
#define COLOR_SRC_RESOURCE(type) MAKE_SRC_RESOURCE(i, type)
#define ColorType ivec4

#elif ResolveColorUint

#define IsResolveColor 1
#define COLOR_SRC_RESOURCE(type) MAKE_SRC_RESOURCE(u, type)
#define ColorType uvec4

#elif ResolveDepth

#define IsResolveDepth 1

#elif ResolveStencil

#define IsResolveStencil 1

#elif ResolveDepthStencil

#define IsResolveDepth 1
#define IsResolveStencil 1

#else

#error "Not all resolve targets are accounted for"

#endif

#if IsResolveColor && (IsResolveDepth || IsResolveStencil)
#error "The shader doesn't resolve color and depth/stencil at the same time."
#endif

#extension GL_EXT_samplerless_texture_functions : require
#if IsResolveStencil
#extension GL_ARB_shader_stencil_export : require
#endif

#define MAKE_SRC_RESOURCE(prefix, type) prefix ## type

#define DEPTH_SRC_RESOURCE(type) type
#define STENCIL_SRC_RESOURCE(type) MAKE_SRC_RESOURCE(u, type)

#if SrcIsArray
#define SRC_RESOURCE_NAME texture2DMSArray
#define TEXEL_FETCH(src, coord, sample) texelFetch(src, ivec3(coord, params.srcLayer), sample)
#else
#define SRC_RESOURCE_NAME texture2DMS
#define TEXEL_FETCH(src, coord, sample) texelFetch(src, coord, sample)
#endif

layout(push_constant) uniform PushConstants {
    // Robust access.
    ivec2 srcExtent;
    // Translation from source to destination coordinates.
    ivec2 srcOffset;
    ivec2 destOffset;
    int srcLayer;
    int samples;
    float invSamples;
    // Mask to output only to color attachments that are actually present.
    int outputMask;
    // Flip control.
    bool flipX;
    bool flipY;
} params;

#if IsResolveColor
layout(set = 0, binding = 0) uniform COLOR_SRC_RESOURCE(SRC_RESOURCE_NAME) color;

layout(location = 0) out ColorType colorOut0;
layout(location = 1) out ColorType colorOut1;
layout(location = 2) out ColorType colorOut2;
layout(location = 3) out ColorType colorOut3;
layout(location = 4) out ColorType colorOut4;
layout(location = 5) out ColorType colorOut5;
layout(location = 6) out ColorType colorOut6;
layout(location = 7) out ColorType colorOut7;
#endif
#if IsResolveDepth
layout(set = 0, binding = 0) uniform DEPTH_SRC_RESOURCE(SRC_RESOURCE_NAME) depth;
#endif
#if IsResolveStencil
layout(set = 0, binding = 1) uniform STENCIL_SRC_RESOURCE(SRC_RESOURCE_NAME) stencil;
#endif

void main()
{
    ivec2 destSubImageCoords = ivec2(gl_FragCoord.xy) - params.destOffset;

    ivec2 srcSubImageCoords = destSubImageCoords;

    // If flipping, srcOffset would contain the opposite coordinates, so we can
    // simply reverse the direction in which x/y grows.
    if (params.flipX)
        srcSubImageCoords.x = -srcSubImageCoords.x;
    if (params.flipY)
        srcSubImageCoords.y = -srcSubImageCoords.y;

    ivec2 srcImageCoords = params.srcOffset + srcSubImageCoords;

    bool isWithinSrcBounds = any(lessThanEqual(ivec2(0), srcImageCoords)) &&
                             any(lessThan(srcImageCoords, params.srcExtent));

#if IsResolveColor
    ColorType colorValue = ColorType(0, 0, 0, 1);
    if (isWithinSrcBounds)
    {
        for (int i = 0; i < params.samples; ++i)
        {
            colorValue += TEXEL_FETCH(color, srcImageCoords, i);
        }
#if IsFloat
        colorValue *= params.invSamples;
#else
        colorValue = ColorType(round(colorValue * params.invSamples));
#endif
    }

    // Note: not exporting to render targets that are not present optimizes the number of export
    // instructions, which would have otherwise been a likely bottleneck.
    if ((params.outputMask & (1 << 0)) != 0)
    {
        colorOut0 = colorValue;
    }
    if ((params.outputMask & (1 << 1)) != 0)
    {
        colorOut1 = colorValue;
    }
    if ((params.outputMask & (1 << 2)) != 0)
    {
        colorOut2 = colorValue;
    }
    if ((params.outputMask & (1 << 3)) != 0)
    {
        colorOut3 = colorValue;
    }
    if ((params.outputMask & (1 << 4)) != 0)
    {
        colorOut4 = colorValue;
    }
    if ((params.outputMask & (1 << 5)) != 0)
    {
        colorOut5 = colorValue;
    }
    if ((params.outputMask & (1 << 6)) != 0)
    {
        colorOut6 = colorValue;
    }
    if ((params.outputMask & (1 << 7)) != 0)
    {
        colorOut7 = colorValue;
    }
#endif  // IsResolveColor

    // Note: always resolve depth/stencil using sample 0.  GLES3 gives us freedom in choosing how
    // to resolve depth/stencil images.

#if IsResolveDepth
    float depthValue = 0;
    if (isWithinSrcBounds)
    {
        depthValue = TEXEL_FETCH(depth, srcImageCoords, 0).x;
    }

    gl_FragDepth = depthValue;
#endif  // IsResolveDepth

#if IsResolveStencil
    uint stencilValue = 0;
    if (isWithinSrcBounds)
    {

        stencilValue = TEXEL_FETCH(stencil, srcImageCoords, 0).x;
    }

    gl_FragStencilRefARB = int(stencilValue);
#endif  // IsResolveStencil
}
