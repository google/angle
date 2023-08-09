//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// LinkAndRelinkFailureTest:
//   Link and relink failure tests for rendering pipeline and compute pipeline.

#include <vector>
#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

class LinkAndRelinkTest : public ANGLETest<>
{
  protected:
    LinkAndRelinkTest() {}
};

class LinkAndRelinkTestES31 : public ANGLETest<>
{
  protected:
    LinkAndRelinkTestES31() {}
};

class LinkAndRelinkTestES32 : public ANGLETest<>
{
  protected:
    LinkAndRelinkTestES32() {}
};

// When a program link or relink fails, if you try to install the unsuccessfully
// linked program (via UseProgram) and start rendering or dispatch compute,
// We can not always report INVALID_OPERATION for rendering/compute pipeline.
// The result depends on the previous state: Whether a valid program is
// installed in current GL state before the link.
// If a program successfully relinks when it is in use, the program might
// change from a rendering program to a compute program in theory,
// or vice versa.

// When program link fails and no valid rendering program is installed in the GL
// state before the link, it should report an error for UseProgram
TEST_P(LinkAndRelinkTest, RenderingProgramFailsWithoutProgramInstalled)
{
    glUseProgram(0);
    GLuint program = glCreateProgram();

    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_FALSE(linkStatus);

    glUseProgram(program);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();
}

// When program link or relink fails and a valid rendering program is installed
// in the GL state before the link, using the failed program via UseProgram
// should report an error, but starting rendering should succeed.
// However, dispatching compute always fails.
TEST_P(LinkAndRelinkTest, RenderingProgramFailsWithProgramInstalled)
{
    // Install a render program in current GL state via UseProgram, then render.
    // It should succeed.
    constexpr char kVS[] = "void main() {}";
    constexpr char kFS[] = "void main() {}";

    GLuint program = glCreateProgram();

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVS);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFS);

    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, fs);

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    EXPECT_GL_NO_ERROR();

    glUseProgram(program);
    EXPECT_GL_NO_ERROR();
    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Link failure, and a valid program has been installed in the GL state.
    GLuint programNull = glCreateProgram();

    glLinkProgram(programNull);
    glGetProgramiv(programNull, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_FALSE(linkStatus);

    // Starting rendering should succeed.
    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Using the unsuccessfully linked program should report an error.
    glUseProgram(programNull);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Using the unsuccessfully linked program, that program should not
    // replace the program binary residing in the GL state. It will not make
    // the installed program invalid either, like what UseProgram(0) can do.
    // So, starting rendering should succeed.
    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // We try to relink the installed program, but make it fail.

    // No vertex shader, relink fails.
    glDetachShader(program, vs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_FALSE(linkStatus);
    EXPECT_GL_NO_ERROR();

    // Starting rendering should succeed.
    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Using the unsuccessfully relinked program should report an error.
    glUseProgram(program);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Using the unsuccessfully relinked program, that program should not
    // replace the program binary residing in the GL state. It will not make
    // the installed program invalid either, like what UseProgram(0) can do.
    // So, starting rendering should succeed.
    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Tests uniform default values.
TEST_P(LinkAndRelinkTest, UniformDefaultValues)
{
    // TODO(anglebug.com/3969): Understand why rectangle texture CLs made this fail.
    ANGLE_SKIP_TEST_IF(IsOzone() && IsIntel());
    constexpr char kFS[] = R"(precision mediump float;
uniform vec4 u_uniform;

bool isZero(vec4 value) {
    return value == vec4(0,0,0,0);
}

void main()
{
    gl_FragColor = isZero(u_uniform) ? vec4(0, 1, 0, 1) : vec4(1, 0, 0, 1);
})";

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), kFS);
    glUseProgram(program);

    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f, 1.0f, true);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);

    GLint loc = glGetUniformLocation(program, "u_uniform");
    ASSERT_NE(-1, loc);
    glUniform4f(loc, 0.1f, 0.2f, 0.3f, 0.4f);

    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f, 1.0f, true);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    glLinkProgram(program);
    ASSERT_GL_NO_ERROR();

    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f, 1.0f, true);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
}

// When program link fails and no valid compute program is installed in the GL
// state before the link, it should report an error for UseProgram and
// DispatchCompute.
TEST_P(LinkAndRelinkTestES31, ComputeProgramFailsWithoutProgramInstalled)
{
    glUseProgram(0);
    GLuint program = glCreateProgram();

    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_FALSE(linkStatus);

    glUseProgram(program);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// When program link or relink fails and a valid compute program is installed in
// the GL state before the link, using the failed program via UseProgram should
// report an error, but dispatching compute should succeed.
TEST_P(LinkAndRelinkTestES31, ComputeProgramFailsWithProgramInstalled)
{
    // Install a compute program in the GL state via UseProgram, then dispatch
    // compute. It should succeed.
    constexpr char kCS[] =
        R"(#version 310 es
        layout(local_size_x=1) in;
        void main()
        {
        })";

    GLuint program = glCreateProgram();

    GLuint cs = CompileShader(GL_COMPUTE_SHADER, kCS);
    EXPECT_NE(0u, cs);

    glAttachShader(program, cs);
    glDeleteShader(cs);

    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    EXPECT_GL_NO_ERROR();

    glUseProgram(program);
    EXPECT_GL_NO_ERROR();
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    // Link failure, and a valid program has been installed in the GL state.
    GLuint programNull = glCreateProgram();

    glLinkProgram(programNull);
    glGetProgramiv(programNull, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_FALSE(linkStatus);

    // Dispatching compute should succeed.
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    // Using the unsuccessfully linked program should report an error.
    glUseProgram(programNull);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Using the unsuccessfully linked program, that program should not
    // replace the program binary residing in the GL state. It will not make
    // the installed program invalid either, like what UseProgram(0) can do.
    // So, dispatching compute should succeed.
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    // We try to relink the installed program, but make it fail.

    // No compute shader, relink fails.
    glDetachShader(program, cs);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_FALSE(linkStatus);
    EXPECT_GL_NO_ERROR();

    // Dispatching compute should succeed.
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    // Using the unsuccessfully relinked program should report an error.
    glUseProgram(program);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Using the unsuccessfully relinked program, that program should not
    // replace the program binary residing in the GL state. It will not make
    // the installed program invalid either, like what UseProgram(0) can do.
    // So, dispatching compute should succeed.
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();
}

// If you compile and link a compute program successfully and use the program,
// then dispatching compute and rendering can succeed (with undefined behavior).
// If you relink the compute program to a rendering program when it is in use,
// then dispatching compute will fail, but starting rendering can succeed.
TEST_P(LinkAndRelinkTestES31, RelinkProgramSucceedsFromComputeToRendering)
{
    constexpr char kCS[] = R"(#version 310 es
layout(local_size_x=1) in;
void main()
{
})";

    GLuint program = glCreateProgram();

    GLuint cs = CompileShader(GL_COMPUTE_SHADER, kCS);
    EXPECT_NE(0u, cs);

    glAttachShader(program, cs);
    glDeleteShader(cs);

    glLinkProgram(program);
    glDetachShader(program, cs);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    EXPECT_GL_NO_ERROR();

    glUseProgram(program);
    EXPECT_GL_NO_ERROR();
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    constexpr char kVS[] = "void main() {}";
    constexpr char kFS[] = "void main() {}";

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVS);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFS);
    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, fs);

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// If you compile and link a rendering program successfully and use the program,
// then starting rendering can succeed, while dispatching compute will fail.
// If you relink the rendering program to a compute program when it is in use,
// then starting rendering will fail, but dispatching compute can succeed.
TEST_P(LinkAndRelinkTestES31, RelinkProgramSucceedsFromRenderingToCompute)
{
    // http://anglebug.com/5072
    ANGLE_SKIP_TEST_IF(IsIntel() && IsLinux() && IsOpenGL());

    constexpr char kVS[] = "void main() {}";
    constexpr char kFS[] = "void main() {}";

    GLuint program = glCreateProgram();

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVS);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFS);

    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, fs);

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    EXPECT_GL_NO_ERROR();

    glUseProgram(program);
    EXPECT_GL_NO_ERROR();
    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    constexpr char kCS[] = R"(#version 310 es
layout(local_size_x=1) in;
void main()
{
})";

    GLuint cs = CompileShader(GL_COMPUTE_SHADER, kCS);
    EXPECT_NE(0u, cs);

    glAttachShader(program, cs);
    glDeleteShader(cs);

    glLinkProgram(program);
    glDetachShader(program, cs);
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    EXPECT_GL_NO_ERROR();

    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();

    glDrawArrays(GL_POINTS, 0, 1);
    EXPECT_GL_NO_ERROR();
}

// Parallel link should continue unscathed even if the attached shaders to the program are modified.
TEST_P(LinkAndRelinkTestES31, ReattachShadersWhileParallelLinking)
{
    constexpr char kVS[]      = R"(#version 300 es
void main()
{
    vec2 position = vec2(-1, -1);
    if (gl_VertexID == 1)
        position = vec2(3, -1);
    else if (gl_VertexID == 2)
        position = vec2(-1, 3);
    gl_Position = vec4(position, 0, 1);
})";
    constexpr char kFSGreen[] = R"(#version 300 es
out mediump vec4 color;
void main()
{
    color = vec4(0, 1, 0, 1);
})";
    constexpr char kFSRed[]   = R"(#version 300 es
out mediump vec4 color;
void main()
{
    color = vec4(1, 0, 0, 1);
})";

    GLuint program = glCreateProgram();

    GLuint vs    = CompileShader(GL_VERTEX_SHADER, kVS);
    GLuint green = CompileShader(GL_FRAGMENT_SHADER, kFSGreen);
    GLuint red   = CompileShader(GL_FRAGMENT_SHADER, kFSRed);

    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, green);
    EXPECT_NE(0u, red);

    glAttachShader(program, vs);
    glAttachShader(program, green);
    glLinkProgram(program);
    ASSERT_GL_NO_ERROR();

    // Immediately reattach another shader
    glDetachShader(program, green);
    glAttachShader(program, red);
    ASSERT_GL_NO_ERROR();

    // Make sure the linked program draws with green
    glUseProgram(program);
    ASSERT_GL_NO_ERROR();

    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
    ASSERT_GL_NO_ERROR();

    glDeleteShader(vs);
    glDeleteShader(green);
    glDeleteShader(red);
    ASSERT_GL_NO_ERROR();
}

// Parallel link should continue unscathed even if new shaders are attached to the program.
TEST_P(LinkAndRelinkTestES31, AttachNewShadersWhileParallelLinking)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_geometry_shader"));

    constexpr char kVS[] = R"(#version 310 es
#extension GL_EXT_geometry_shader : require
void main()
{
    vec2 position = vec2(-1, -1);
    if (gl_VertexID == 1)
        position = vec2(3, -1);
    else if (gl_VertexID == 2)
        position = vec2(-1, 3);
    gl_Position = vec4(position, 0, 1);
})";
    constexpr char kFS[] = R"(#version 310 es
#extension GL_EXT_geometry_shader : require
out mediump vec4 color;
void main()
{
    color = vec4(0, 1, 0, 1);
})";
    constexpr char kGS[] = R"(#version 310 es
#extension GL_EXT_geometry_shader : require
layout (invocations = 3, triangles) in;
layout (triangle_strip, max_vertices = 3) out;
void main()
{
})";

    GLuint program = glCreateProgram();

    GLuint vs = CompileShader(GL_VERTEX_SHADER, kVS);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kFS);
    GLuint gs = CompileShader(GL_GEOMETRY_SHADER, kGS);

    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, fs);
    EXPECT_NE(0u, gs);

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    ASSERT_GL_NO_ERROR();

    // Immediately attach another shader
    glAttachShader(program, gs);
    ASSERT_GL_NO_ERROR();

    // Make sure the linked program draws with green
    glUseProgram(program);
    ASSERT_GL_NO_ERROR();

    glDrawArrays(GL_TRIANGLES, 0, 3);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
    ASSERT_GL_NO_ERROR();

    glDeleteShader(vs);
    glDeleteShader(fs);
    glDeleteShader(gs);
    ASSERT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST_ES2_AND_ES3(LinkAndRelinkTest);

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LinkAndRelinkTestES31);
ANGLE_INSTANTIATE_TEST_ES31(LinkAndRelinkTestES31);

}  // namespace
