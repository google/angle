//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FullScreenQuad.vert: Simple full-screen quad vertex shader.

#version 450 core

// This push constant is placed in the range 0-4, so any fragment shader that uses this must have
// its push constants start at an offset of at least 4.  As some fragment shaders' push constant
// can start with vec4, it would be good practice to start them at offset 16.
layout(push_constant) uniform PushConstants {
    float depth;
} params;

const vec2 kQuadVertices[] = {
    vec2(-1, 1),
    vec2(-1, -1),
    vec2(1, -1),
    vec2(-1, 1),
    vec2(1, -1),
    vec2(1, 1),
};

void main()
{
    gl_Position = vec4(kQuadVertices[gl_VertexIndex], params.depth, 1);
}
