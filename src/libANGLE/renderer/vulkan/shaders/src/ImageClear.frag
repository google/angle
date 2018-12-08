//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImageClear.frag: Clear image to a solid color.

#version 450 core

layout(push_constant) uniform PushConstants {
    vec4 clearColor;
} params;

layout(location = 0) out vec4 colorOut;

void main()
{
    colorOut = params.clearColor;
}
