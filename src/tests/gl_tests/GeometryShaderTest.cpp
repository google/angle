//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// GeometryShaderTest.cpp : Tests of the implementation of geometry shader

#include "test_utils/ANGLETest.h"

using namespace angle;

namespace
{

class GeometryShaderTest : public ANGLETest
{
  protected:
    static std::string CreateEmptyGeometryShader(const std::string &inputPrimitive,
                                                 const std::string &outputPrimitive,
                                                 int invocations,
                                                 int maxVertices)
    {
        std::ostringstream ostream;
        ostream << "#version 310 es\n"
                   "#extension GL_EXT_geometry_shader : require\n";
        if (!inputPrimitive.empty())
        {
            ostream << "layout (" << inputPrimitive << ") in;\n";
        }
        if (!outputPrimitive.empty())
        {
            ostream << "layout (" << outputPrimitive << ") out;\n";
        }
        if (invocations > 0)
        {
            ostream << "layout (invocations = " << invocations << ") in;\n";
        }
        if (maxVertices >= 0)
        {
            ostream << "layout (max_vertices = " << maxVertices << ") out;\n";
        }
        ostream << "void main()\n"
                   "{\n"
                   "}";
        return ostream.str();
    }

    const std::string kDefaultVertexShader =
        R"(#version 310 es
        void main()
        {
            gl_Position = vec4(1.0, 0.0, 0.0, 1.0);
        })";

    const std::string kDefaultFragmentShader =
        R"(#version 310 es
        precision mediump float;
        layout (location = 0) out vec4 frag_out;
        void main()
        {
            frag_out = vec4(1.0, 0.0, 0.0, 1.0);
        })";
};

class GeometryShaderTestES3 : public ANGLETest
{
};

// Verify that Geometry Shader cannot be created in an OpenGL ES 3.0 context.
TEST_P(GeometryShaderTestES3, CreateGeometryShaderInES3)
{
    EXPECT_TRUE(!extensionEnabled("GL_EXT_geometry_shader"));
    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER_EXT);
    EXPECT_EQ(0u, geometryShader);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
}

// Verify that Geometry Shader can be created and attached to a program.
TEST_P(GeometryShaderTest, CreateAndAttachGeometryShader)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_geometry_shader"));

    const std::string &geometryShaderSource =
        R"(#version 310 es
        #extension GL_EXT_geometry_shader : require
        layout (invocations = 3, triangles) in;
        layout (triangle_strip, max_vertices = 3) out;
        in vec4 texcoord[];
        out vec4 o_texcoord;
        void main()
        {
            int n;
            for (n = 0; n < gl_in.length(); n++)
            {
                gl_Position = gl_in[n].gl_Position;
                gl_Layer   = gl_InvocationID;
                o_texcoord = texcoord[n];
                EmitVertex();
            }
            EndPrimitive();
        })";

    GLuint geometryShader = CompileShader(GL_GEOMETRY_SHADER_EXT, geometryShaderSource);

    EXPECT_NE(0u, geometryShader);

    GLuint programID = glCreateProgram();
    glAttachShader(programID, geometryShader);

    glDetachShader(programID, geometryShader);
    glDeleteShader(geometryShader);
    glDeleteProgram(programID);

    EXPECT_GL_NO_ERROR();
}

// Verify that all the implementation dependent geometry shader related resource limits meet the
// requirement of GL_EXT_geometry_shader SPEC.
TEST_P(GeometryShaderTest, GeometryShaderImplementationDependentLimits)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_geometry_shader"));

    const std::map<GLenum, int> limits = {{GL_MAX_FRAMEBUFFER_LAYERS_EXT, 256},
                                          {GL_MAX_GEOMETRY_UNIFORM_COMPONENTS_EXT, 1024},
                                          {GL_MAX_GEOMETRY_UNIFORM_BLOCKS_EXT, 12},
                                          {GL_MAX_GEOMETRY_INPUT_COMPONENTS_EXT, 64},
                                          {GL_MAX_GEOMETRY_OUTPUT_COMPONENTS_EXT, 64},
                                          {GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT, 256},
                                          {GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS_EXT, 1024},
                                          {GL_MAX_GEOMETRY_TEXTURE_IMAGE_UNITS_EXT, 16},
                                          {GL_MAX_GEOMETRY_ATOMIC_COUNTER_BUFFERS_EXT, 0},
                                          {GL_MAX_GEOMETRY_ATOMIC_COUNTERS_EXT, 0},
                                          {GL_MAX_GEOMETRY_SHADER_STORAGE_BLOCKS_EXT, 0},
                                          {GL_MAX_GEOMETRY_IMAGE_UNIFORMS_EXT, 0},
                                          {GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT, 32}};

    GLint value;
    for (const auto &limit : limits)
    {
        value = 0;
        glGetIntegerv(limit.first, &value);
        EXPECT_GL_NO_ERROR();
        EXPECT_GE(value, limit.second);
    }

    value = 0;
    glGetIntegerv(GL_LAYER_PROVOKING_VERTEX_EXT, &value);
    EXPECT_GL_NO_ERROR();
    EXPECT_TRUE(value == GL_FIRST_VERTEX_CONVENTION_EXT || value == GL_LAST_VERTEX_CONVENTION_EXT ||
                value == GL_UNDEFINED_VERTEX_EXT);
}

// Verify that all the combined resource limits meet the requirement of GL_EXT_geometry_shader SPEC.
TEST_P(GeometryShaderTest, CombinedResourceLimits)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_geometry_shader"));

    // See http://anglebug.com/2261.
    ANGLE_SKIP_TEST_IF(IsAndroid());

    const std::map<GLenum, int> limits = {{GL_MAX_UNIFORM_BUFFER_BINDINGS, 48},
                                          {GL_MAX_COMBINED_UNIFORM_BLOCKS, 36},
                                          {GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, 64}};

    GLint value;
    for (const auto &limit : limits)
    {
        value = 0;
        glGetIntegerv(limit.first, &value);
        EXPECT_GL_NO_ERROR();
        EXPECT_GE(value, limit.second);
    }
}

// Verify that linking a program with an uncompiled geometry shader causes a link failure.
TEST_P(GeometryShaderTest, LinkWithUncompiledGeoemtryShader)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_geometry_shader"));

    GLuint vertexShader   = CompileShader(GL_VERTEX_SHADER, kDefaultVertexShader);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, kDefaultFragmentShader);
    ASSERT_NE(0u, vertexShader);
    ASSERT_NE(0u, fragmentShader);

    GLuint geometryShader = glCreateShader(GL_GEOMETRY_SHADER_EXT);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glAttachShader(program, geometryShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glDeleteShader(geometryShader);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_EQ(0, linkStatus);

    glDeleteProgram(program);
    ASSERT_GL_NO_ERROR();
}

// Verify that linking a program with geometry shader whose version is different from other shaders
// in this program causes a link error.
TEST_P(GeometryShaderTest, LinkWhenShaderVersionMismatch)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_geometry_shader"));

    const std::string &kDefaultVertexShaderVersion300 =
        R"(#version 300 es
        void main()
        {
            gl_Position = vec4(1.0, 0.0, 0.0, 1.0);
        })";

    const std::string kDefaultFragmentShaderVersion300 =
        R"(#version 300 es
        precision mediump float;
        layout (location = 0) out vec4 frag_out;
        void main()
        {
            frag_out = vec4(1.0, 0.0, 0.0, 1.0);
        })";

    const std::string &emptyGeometryShader = CreateEmptyGeometryShader("points", "points", 2, 1);

    GLuint program = CompileProgramWithGS(kDefaultVertexShaderVersion300, emptyGeometryShader,
                                          kDefaultFragmentShaderVersion300);
    EXPECT_EQ(0u, program);
}

// Verify that linking a program with geometry shader that lacks input primitive,
// output primitive, or declaration on 'max_vertices' causes a link failure.
TEST_P(GeometryShaderTest, LinkValidationOnGeometryShaderLayouts)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_geometry_shader"));

    const std::string &gsWithoutInputPrimitive  = CreateEmptyGeometryShader("", "points", 2, 1);
    const std::string &gsWithoutOutputPrimitive = CreateEmptyGeometryShader("points", "", 2, 1);
    const std::string &gsWithoutInvocations = CreateEmptyGeometryShader("points", "points", -1, 1);
    const std::string &gsWithoutMaxVertices = CreateEmptyGeometryShader("points", "points", 2, -1);

    // Linking a program with a geometry shader that only lacks 'invocations' should not cause a
    // link failure.
    GLuint program =
        CompileProgramWithGS(kDefaultVertexShader, gsWithoutInvocations, kDefaultFragmentShader);
    EXPECT_NE(0u, program);

    glDeleteProgram(program);

    // Linking a program with a geometry shader that lacks input primitive, output primitive or
    // 'max_vertices' causes a link failure.
    program =
        CompileProgramWithGS(kDefaultVertexShader, gsWithoutInputPrimitive, kDefaultFragmentShader);
    EXPECT_EQ(0u, program);

    program = CompileProgramWithGS(kDefaultVertexShader, gsWithoutOutputPrimitive,
                                   kDefaultFragmentShader);
    EXPECT_EQ(0u, program);

    program =
        CompileProgramWithGS(kDefaultVertexShader, gsWithoutMaxVertices, kDefaultFragmentShader);
    EXPECT_EQ(0u, program);

    ASSERT_GL_NO_ERROR();
}

// Verify that an link error occurs when the vertex shader has an array output and there is a
// geometry shader in the program.
TEST_P(GeometryShaderTest, VertexShaderArrayOutput)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_EXT_geometry_shader"));

    const std::string &vertexShader =
        R"(#version 310 es
        in vec4 vertex_in;
        out vec4 vertex_out[3];
        void main()
        {
            gl_Position = vertex_in;
            vertex_out[0] = vec4(1.0, 0.0, 0.0, 1.0);
            vertex_out[1] = vec4(0.0, 1.0, 0.0, 1.0);
            vertex_out[2] = vec4(0.0, 0.0, 1.0, 1.0);
        })";

    const std::string &geometryShader =
        R"(#version 310 es
        #extension GL_EXT_geometry_shader : require
        layout (invocations = 3, triangles) in;
        layout (points, max_vertices = 3) out;
        in vec4 vertex_out[];
        out vec4 geometry_color;
        void main()
        {
            gl_Position = gl_in[0].gl_Position;
            geometry_color = vertex_out[0];
            EmitVertex();
        })";

    const std::string &fragmentShader =
        R"(#version 310 es
        precision mediump float;
        in vec4 geometry_color;
        layout (location = 0) out vec4 output_color;
        void main()
        {
            output_color = geometry_color;
        })";

    GLuint program = CompileProgramWithGS(vertexShader, geometryShader, fragmentShader);
    EXPECT_EQ(0u, program);

    EXPECT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST(GeometryShaderTestES3, ES3_OPENGL(), ES3_OPENGLES(), ES3_D3D11());
ANGLE_INSTANTIATE_TEST(GeometryShaderTest, ES31_OPENGL(), ES31_OPENGLES(), ES31_D3D11());
}
