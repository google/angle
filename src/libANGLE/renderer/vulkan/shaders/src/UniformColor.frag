//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// UniformColor.frag: Simple solid color fragment shader.

#version 450 core

layout(set = 0, binding = 1) uniform block {
    vec4 colorIn;
};

layout(location = 0) out vec4 colorOut;

void main()
{
    colorOut = colorIn;
}
