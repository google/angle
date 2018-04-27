//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FullScreenQuad.vert: Simple full-screen quad vertex shader.

#version 450 core

const vec4 kQuadVertices[] = {
    vec4(-1, 1, 0, 1),
    vec4(-1, -1, 0, 1),
    vec4(1, -1, 0, 1),
    vec4(-1, 1, 0, 1),
    vec4(1, -1, 0, 1),
    vec4(1, 1, 0, 1),
};

void main()
{
    gl_Position = kQuadVertices[gl_VertexIndex];
}
