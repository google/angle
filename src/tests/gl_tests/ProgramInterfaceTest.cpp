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

    const std::string fragmentShaderSource =
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

ANGLE_INSTANTIATE_TEST(ProgramInterfaceTestES31, ES31_OPENGL(), ES31_D3D11(), ES31_OPENGLES());
}  // anonymous namespace
