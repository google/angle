//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MultiviewTest:
//   Implementation of helpers for multiview testing.
//

#include "test_utils/MultiviewTest.h"
#include "platform/WorkaroundsD3D.h"

namespace angle
{

GLuint CreateSimplePassthroughProgram(int numViews)
{
    const std::string vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = " +
        ToString(numViews) +
        ") in;\n"
        "layout(location=0) in vec2 vPosition;\n"
        "void main()\n"
        "{\n"
        "   gl_PointSize = 1.;\n"
        "   gl_Position = vec4(vPosition.xy, 0.0, 1.0);\n"
        "}\n";

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "   col = vec4(0,1,0,1);\n"
        "}\n";
    return CompileProgram(vsSource, fsSource);
}

std::ostream &operator<<(std::ostream &os, const MultiviewImplementationParams &params)
{
    const PlatformParameters &base = static_cast<const PlatformParameters &>(params);
    os << base;
    if (params.mForceUseGeometryShaderOnD3D)
    {
        os << "_force_geom_shader";
    }
    else
    {
        os << "_vertex_shader";
    }
    return os;
}

MultiviewImplementationParams VertexShaderOpenGL(GLint majorVersion, GLint minorVersion)
{
    return MultiviewImplementationParams(majorVersion, minorVersion, false, egl_platform::OPENGL());
}

MultiviewImplementationParams VertexShaderD3D11(GLint majorVersion, GLint minorVersion)
{
    return MultiviewImplementationParams(majorVersion, minorVersion, false, egl_platform::D3D11());
}

MultiviewImplementationParams GeomShaderD3D11(GLint majorVersion, GLint minorVersion)
{
    return MultiviewImplementationParams(majorVersion, minorVersion, true, egl_platform::D3D11());
}

void MultiviewTest::overrideWorkaroundsD3D(WorkaroundsD3D *workarounds)
{
    workarounds->selectViewInGeometryShader = GetParam().mForceUseGeometryShaderOnD3D;
}

}  // namespace angle
