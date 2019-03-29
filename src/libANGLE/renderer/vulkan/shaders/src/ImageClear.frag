//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImageClear.frag: Clear image to a solid color.

#version 450 core

layout(push_constant) uniform PushConstants {
    vec4 clearColor;
    int clearBufferMask;
} params;

// ANGLE supports a maximum of 8 draw buffers
layout(location = 0) out vec4 colorOut0;
layout(location = 1) out vec4 colorOut1;
layout(location = 2) out vec4 colorOut2;
layout(location = 3) out vec4 colorOut3;
layout(location = 4) out vec4 colorOut4;
layout(location = 5) out vec4 colorOut5;
layout(location = 6) out vec4 colorOut6;
layout(location = 7) out vec4 colorOut7;

void main()
{
    if ((params.clearBufferMask & (1 << 0)) != 0)
    {
        colorOut0 = params.clearColor;
    }
    if ((params.clearBufferMask & (1 << 1)) != 0)
    {
        colorOut1 = params.clearColor;
    }
    if ((params.clearBufferMask & (1 << 2)) != 0)
    {
        colorOut2 = params.clearColor;
    }
    if ((params.clearBufferMask & (1 << 3)) != 0)
    {
        colorOut3 = params.clearColor;
    }
    if ((params.clearBufferMask & (1 << 4)) != 0)
    {
        colorOut4 = params.clearColor;
    }
    if ((params.clearBufferMask & (1 << 5)) != 0)
    {
        colorOut5 = params.clearColor;
    }
    if ((params.clearBufferMask & (1 << 6)) != 0)
    {
        colorOut6 = params.clearColor;
    }
    if ((params.clearBufferMask & (1 << 7)) != 0)
    {
        colorOut7 = params.clearColor;
    }
}
