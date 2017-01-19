//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// WEBGL_multiview_test.cpp:
//   Test that shaders with gl_ViewID_OVR are validated correctly.
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "tests/test_utils/ShaderCompileTreeTest.h"

using namespace sh;

class WEBGLMultiviewVertexShaderTest : public ShaderCompileTreeTest
{
  public:
    WEBGLMultiviewVertexShaderTest() {}
  protected:
    ::GLenum getShaderType() const override { return GL_VERTEX_SHADER; }
    ShShaderSpec getShaderSpec() const override { return SH_WEBGL3_SPEC; }
    void initResources(ShBuiltInResources *resources) override
    {
        resources->OVR_multiview = 1;
        resources->MaxViewsOVR   = 4;
    }
};

class WEBGLMultiviewFragmentShaderTest : public ShaderCompileTreeTest
{
  public:
    WEBGLMultiviewFragmentShaderTest() {}
  protected:
    ::GLenum getShaderType() const override { return GL_FRAGMENT_SHADER; }
    ShShaderSpec getShaderSpec() const override { return SH_WEBGL3_SPEC; }
    void initResources(ShBuiltInResources *resources) override
    {
        resources->OVR_multiview = 1;
        resources->MaxViewsOVR   = 4;
    }
};

// Invalid combination of extensions (restricted in the WEBGL_multiview spec).
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidBothMultiviewAndMultiview2)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "#extension GL_OVR_multiview2 : enable\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{\n"
        "    gl_Position.x = (gl_ViewID_OVR == 0u) ? 1.0 : 0.0;\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Invalid combination of non-matching num_views declarations.
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidNumViewsMismatch)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "layout(num_views = 2) in;\n"
        "layout(num_views = 1) in;\n"
        "void main()\n"
        "{\n"
        "    gl_Position.x = (gl_ViewID_OVR == 0u) ? 1.0 : 0.0;\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Invalid value zero for num_views.
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidNumViewsZero)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "layout(num_views = 0) in;\n"
        "void main()\n"
        "{\n"
        "    gl_Position.x = (gl_ViewID_OVR == 0u) ? 1.0 : 0.0;\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Too large value for num_views.
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidNumViewsGreaterThanMax)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "layout(num_views = 5) in;\n"
        "void main()\n"
        "{\n"
        "    gl_Position.x = (gl_ViewID_OVR == 0u) ? 1.0 : 0.0;\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Valid use of gl_ViewID_OVR in a ternary operator.
TEST_F(WEBGLMultiviewVertexShaderTest, ValidViewIDInTernary)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "layout(num_views = 2) in;  // Duplicated on purpose\n"
        "void main()\n"
        "{\n"
        "    gl_Position.x = (gl_ViewID_OVR == 0u) ? 1.0 : 0.0;\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "}\n";
    if (!compile(shaderString))
    {
        FAIL() << "Shader compilation failed, expecting success:\n" << mInfoLog;
    }
}

// Valid use of gl_ViewID_OVR in an if statement.
TEST_F(WEBGLMultiviewVertexShaderTest, ValidViewIDInIf)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{\n"
        "    if (gl_ViewID_OVR == 0u)\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "    }\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "}\n";
    if (!compile(shaderString))
    {
        FAIL() << "Shader compilation failed, expecting success:\n" << mInfoLog;
    }
}

// Valid normal write of gl_Position in addition to the write that's dependent on gl_ViewID_OVR.
TEST_F(WEBGLMultiviewVertexShaderTest, ValidWriteOfGlPosition)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{\n"
        "    if (0u == gl_ViewID_OVR)\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "    }\n"
        "    gl_Position = vec4(1, 1, 1, 1);\n"
        "}\n";
    if (!compile(shaderString))
    {
        FAIL() << "Shader compilation failed, expecting success:\n" << mInfoLog;
    }
}

// Invalid assignment to gl_Position.y inside if dependent on gl_ViewID_OVR.
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidGlPositionAssignmentInIf)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{\n"
        "    if (gl_ViewID_OVR == 0u)\n"
        "    {\n"
        "        gl_Position.y = 1.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        gl_Position.y = 1.0;\n"
        "    }\n"
        "    gl_Position.xzw = vec3(0, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Invalid multiple assignments inside if dependent on gl_ViewID_OVR.
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidMultipleGlPositionXAssignmentsInIf)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{\n"
        "    if (gl_ViewID_OVR == 0u)\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "        gl_Position.x = 2.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "    }\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Invalid read of gl_Position
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidReadOfGlPosition)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{\n"
        "    if (gl_ViewID_OVR == 0u) {\n"
        "        gl_Position.x = 1.0;\n"
        "    } else {\n"
        "        gl_Position.x = 1.0;\n"
        "    }\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "    float f = gl_Position.y;\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Read gl_Position when the shader does not refer to gl_ViewID_OVR.
TEST_F(WEBGLMultiviewVertexShaderTest, ValidReadOfGlPosition)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "uniform float u;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(0, 0, 0, 1);\n"
        "    gl_Position.y = gl_Position.x * u;\n"
        "}\n";
    if (!compile(shaderString))
    {
        FAIL() << "Shader compilation failed, expecting success:\n" << mInfoLog;
    }
}

// Read gl_FragCoord in a OVR_multiview fragment shader.
TEST_F(WEBGLMultiviewFragmentShaderTest, InvalidReadOfFragCoord)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision highp float;\n"
        "out vec4 outColor;\n"
        "void main()\n"
        "{\n"
        "    outColor = vec4(gl_FragCoord.xy, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Read gl_ViewID_OVR in an OVR_multiview fragment shader.
TEST_F(WEBGLMultiviewFragmentShaderTest, InvalidReadOfViewID)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision highp float;\n"
        "out vec4 outColor;\n"
        "void main()\n"
        "{\n"
        "    outColor = vec4(gl_ViewID_OVR, 0, 0, 1);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Tricky invalid read of view ID.
TEST_F(WEBGLMultiviewVertexShaderTest, InvalidConsumingExpressionForAssignGLPositionX)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{\n"
        "    float f = (gl_Position.x = (gl_ViewID_OVR == 0u) ? 1.0 : 0.0);\n"
        "    gl_Position.yzw = vec3(f, f, f);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}

// Using the OVR_multiview2 extension directive lifts restrictions of OVR_multiview.
TEST_F(WEBGLMultiviewVertexShaderTest, RestrictionsLiftedMultiview2)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "layout(num_views = 2) in;\n"
        "out float out_f;\n"
        "void main()\n"
        "{\n"
        "    if (gl_ViewID_OVR == 0u)\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "        gl_Position.x = 2.0;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        gl_Position.x = 1.0;\n"
        "    }\n"
        "    gl_Position.yzw = vec3(0, 0, 1);\n"
        "    gl_Position += vec4(1, 0, 0, 1);\n"
        "    out_f = float(gl_ViewID_OVR * 2u);\n"
        "}\n";
    if (!compile(shaderString))
    {
        FAIL() << "Shader compilation failed, expecting success:\n" << mInfoLog;
    }
}

// Correct use of GL_OVR_multiview macros.
TEST_F(WEBGLMultiviewVertexShaderTest, ValidUseOfExtensionMacros)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#ifdef GL_OVR_multiview\n"
        "#ifdef GL_OVR_multiview2\n"
        "#if (GL_OVR_multiview == 1) && (GL_OVR_multiview2 == 1)\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);\n"
        "}\n"
        "#endif\n"
        "#endif\n"
        "#endif\n";
    if (!compile(shaderString))
    {
        FAIL() << "Shader compilation failed, expecting success:\n" << mInfoLog;
    }
}

// Test that the parent node is tracked correctly when validating assignment to gl_Position.
TEST_F(WEBGLMultiviewVertexShaderTest, AssignmentWithViewIDInsideAssignment)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "void main()\n"
        "{\n"
        "    gl_Position.y = (gl_Position.x = (gl_ViewID_OVR == 0u) ? 1.0 : 0.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure:\n" << mInfoLog;
    }
}
