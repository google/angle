//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramInterfaceTest: Tests of program interfaces.

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

class ProgramInterfaceTestES31 : public ANGLETest
{
  protected:
    ProgramInterfaceTestES31()
    {
        setWindowWidth(64);
        setWindowHeight(64);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

// Tests glGetProgramResourceIndex.
TEST_P(ProgramInterfaceTestES31, GetResourceIndex)
{
    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "in highp vec4 position;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = position;\n"
        "}";

    const std::string &fragmentShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "uniform vec4 color;\n"
        "out vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "    oColor = color;\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertexShaderSource, fragmentShaderSource);

    GLuint index = glGetProgramResourceIndex(program, GL_PROGRAM_INPUT, "position");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(GL_INVALID_INDEX, index);

    index = glGetProgramResourceIndex(program, GL_PROGRAM_INPUT, "missing");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(GL_INVALID_INDEX, index);

    index = glGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, "oColor");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(GL_INVALID_INDEX, index);

    index = glGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, "missing");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(GL_INVALID_INDEX, index);

    index = glGetProgramResourceIndex(program, GL_ATOMIC_COUNTER_BUFFER, "missing");
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
}

// Tests glGetProgramResourceName.
TEST_P(ProgramInterfaceTestES31, GetResourceName)
{
    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "in highp vec4 position;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = position;\n"
        "}";

    const std::string &fragmentShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "uniform vec4 color;\n"
        "out vec4 oColor[4];\n"
        "void main()\n"
        "{\n"
        "    oColor[0] = color;\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertexShaderSource, fragmentShaderSource);

    GLuint index = glGetProgramResourceIndex(program, GL_PROGRAM_INPUT, "position");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(GL_INVALID_INDEX, index);

    GLchar name[64];
    GLsizei length;
    glGetProgramResourceName(program, GL_PROGRAM_INPUT, index, sizeof(name), &length, name);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(8, length);
    EXPECT_EQ("position", std::string(name));

    glGetProgramResourceName(program, GL_PROGRAM_INPUT, index, 4, &length, name);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(3, length);
    EXPECT_EQ("pos", std::string(name));

    glGetProgramResourceName(program, GL_PROGRAM_INPUT, index, -1, &length, name);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    glGetProgramResourceName(program, GL_PROGRAM_INPUT, GL_INVALID_INDEX, sizeof(name), &length,
                             name);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    index = glGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, "oColor");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(GL_INVALID_INDEX, index);

    glGetProgramResourceName(program, GL_PROGRAM_OUTPUT, index, sizeof(name), &length, name);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(9, length);
    EXPECT_EQ("oColor[0]", std::string(name));

    glGetProgramResourceName(program, GL_PROGRAM_OUTPUT, index, 8, &length, name);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(7, length);
    EXPECT_EQ("oColor[", std::string(name));
}

// Tests glGetProgramResourceLocation.
TEST_P(ProgramInterfaceTestES31, GetResourceLocation)
{
    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "layout(location = 3) in highp vec4 position;\n"
        "in highp vec4 noLocationSpecified;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = position;\n"
        "}";

    const std::string &fragmentShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "uniform vec4 color;\n"
        "layout(location = 2) out vec4 oColor[4];\n"
        "void main()\n"
        "{\n"
        "    oColor[0] = color;\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertexShaderSource, fragmentShaderSource);

    std::array<GLenum, 5> invalidInterfaces = {{GL_UNIFORM_BLOCK, GL_TRANSFORM_FEEDBACK_VARYING,
                                                GL_BUFFER_VARIABLE, GL_SHADER_STORAGE_BLOCK,
                                                GL_ATOMIC_COUNTER_BUFFER}};
    GLint location;
    for (auto &invalidInterface : invalidInterfaces)
    {
        location = glGetProgramResourceLocation(program, invalidInterface, "any");
        EXPECT_GL_ERROR(GL_INVALID_ENUM);
        EXPECT_EQ(-1, location);
    }

    location = glGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "position");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(3, location);

    location = glGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "noLocationSpecified");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(-1, location);

    location = glGetProgramResourceLocation(program, GL_PROGRAM_INPUT, "missing");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(-1, location);

    location = glGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "oColor");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(2, location);
    location = glGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "oColor[0]");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(2, location);
    location = glGetProgramResourceLocation(program, GL_PROGRAM_OUTPUT, "oColor[3]");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(5, location);
}

ANGLE_INSTANTIATE_TEST(ProgramInterfaceTestES31, ES31_OPENGL(), ES31_D3D11(), ES31_OPENGLES());
}  // anonymous namespace
