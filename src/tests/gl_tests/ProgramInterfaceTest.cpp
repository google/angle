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

// Tests glGetProgramResource.
TEST_P(ProgramInterfaceTestES31, GetResource)
{
    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "layout(location = 3) in highp vec4 position;\n"
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

    GLuint index = glGetProgramResourceIndex(program, GL_PROGRAM_INPUT, "position");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(GL_INVALID_INDEX, index);

    constexpr int kPropCount = 7;
    std::array<GLint, kPropCount> params;
    GLsizei length;
    std::array<GLenum, kPropCount> props = {
        {GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION, GL_NAME_LENGTH, GL_REFERENCED_BY_VERTEX_SHADER,
         GL_REFERENCED_BY_FRAGMENT_SHADER, GL_REFERENCED_BY_COMPUTE_SHADER}};
    glGetProgramResourceiv(program, GL_PROGRAM_INPUT, index, kPropCount, props.data(), kPropCount,
                           &length, params.data());
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(kPropCount, length);
    EXPECT_EQ(GL_FLOAT_VEC4, params[0]);  // type
    EXPECT_EQ(1, params[1]);              // array_size
    EXPECT_EQ(3, params[2]);              // location
    EXPECT_EQ(9, params[3]);              // name_length
    EXPECT_EQ(1, params[4]);              // referenced_by_vertex_shader
    EXPECT_EQ(0, params[5]);              // referenced_by_fragment_shader
    EXPECT_EQ(0, params[6]);              // referenced_by_compute_shader

    index = glGetProgramResourceIndex(program, GL_PROGRAM_OUTPUT, "oColor[0]");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(index, GL_INVALID_INDEX);
    glGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, index, kPropCount, props.data(),
                           kPropCount - 1, &length, params.data());
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(kPropCount - 1, length);
    EXPECT_EQ(GL_FLOAT_VEC4, params[0]);  // type
    EXPECT_EQ(4, params[1]);              // array_size
    EXPECT_EQ(2, params[2]);              // location
    EXPECT_EQ(10, params[3]);             // name_length
    EXPECT_EQ(0, params[4]);              // referenced_by_vertex_shader
    EXPECT_EQ(1, params[5]);              // referenced_by_fragment_shader

    GLenum invalidOutputProp = GL_OFFSET;
    glGetProgramResourceiv(program, GL_PROGRAM_OUTPUT, index, 1, &invalidOutputProp, 1, &length,
                           params.data());
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Tests glGetProgramInterfaceiv.
TEST_P(ProgramInterfaceTestES31, GetProgramInterface)
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
        "uniform ub {\n"
        "    vec4 mem0;\n"
        "    vec4 mem1;\n"
        "} instance;\n"
        "void main()\n"
        "{\n"
        "    oColor = color;\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertexShaderSource, fragmentShaderSource);

    GLint num;
    glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_ACTIVE_RESOURCES, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(1, num);

    glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NAME_LENGTH, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(9, num);

    glGetProgramInterfaceiv(program, GL_PROGRAM_INPUT, GL_MAX_NUM_ACTIVE_VARIABLES, &num);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_ACTIVE_RESOURCES, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(1, num);

    glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NAME_LENGTH, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(7, num);

    glGetProgramInterfaceiv(program, GL_PROGRAM_OUTPUT, GL_MAX_NUM_ACTIVE_VARIABLES, &num);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_ACTIVE_RESOURCES, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(1, num);

    glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NAME_LENGTH, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(3, num);

    glGetProgramInterfaceiv(program, GL_UNIFORM_BLOCK, GL_MAX_NUM_ACTIVE_VARIABLES, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(2, num);  // mem0, mem1

    glGetProgramInterfaceiv(program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(3, num);

    glGetProgramInterfaceiv(program, GL_UNIFORM, GL_MAX_NAME_LENGTH, &num);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(8, num);  // "ub.mem0"

    glGetProgramInterfaceiv(program, GL_UNIFORM, GL_MAX_NUM_ACTIVE_VARIABLES, &num);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Tests the resource property query for uniform can be done correctly.
TEST_P(ProgramInterfaceTestES31, GetUniformProperties)
{
    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "precision highp float;\n"
        "uniform layout(location=12) vec4 color;\n"
        "layout(binding = 2, offset = 4) uniform atomic_uint foo;\n"
        "void main()\n"
        "{\n"
        "    atomicCounterIncrement(foo);\n"
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

    GLuint index = glGetProgramResourceIndex(program, GL_UNIFORM, "color");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(GL_INVALID_INDEX, index);

    GLchar name[64];
    GLsizei length;
    glGetProgramResourceName(program, GL_UNIFORM, index, sizeof(name), &length, name);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(5, length);
    EXPECT_EQ("color", std::string(name));

    GLint location = glGetProgramResourceLocation(program, GL_UNIFORM, "color");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(12, location);

    constexpr int kPropCount = 13;
    std::array<GLint, kPropCount> params;
    std::array<GLenum, kPropCount> props = {
        {GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION, GL_NAME_LENGTH, GL_REFERENCED_BY_VERTEX_SHADER,
         GL_REFERENCED_BY_FRAGMENT_SHADER, GL_REFERENCED_BY_COMPUTE_SHADER, GL_ARRAY_STRIDE,
         GL_BLOCK_INDEX, GL_IS_ROW_MAJOR, GL_MATRIX_STRIDE, GL_OFFSET,
         GL_ATOMIC_COUNTER_BUFFER_INDEX}};
    glGetProgramResourceiv(program, GL_UNIFORM, index, kPropCount, props.data(), kPropCount,
                           &length, params.data());
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(kPropCount, length);
    EXPECT_EQ(GL_FLOAT_VEC4, params[0]);  // type
    EXPECT_EQ(1, params[1]);              // array_size
    EXPECT_EQ(12, params[2]);             // location
    EXPECT_EQ(6, params[3]);              // name_length
    EXPECT_EQ(0, params[4]);              // referenced_by_vertex_shader
    EXPECT_EQ(1, params[5]);              // referenced_by_fragment_shader
    EXPECT_EQ(0, params[6]);              // referenced_by_compute_shader
    EXPECT_EQ(-1, params[7]);             // array_stride
    EXPECT_EQ(-1, params[8]);             // block_index
    EXPECT_EQ(0, params[9]);              // is_row_major
    EXPECT_EQ(-1, params[10]);            // matrix_stride
    EXPECT_EQ(-1, params[11]);            // offset
    EXPECT_EQ(-1, params[12]);            // atomic_counter_buffer_index

    index = glGetProgramResourceIndex(program, GL_UNIFORM, "foo");
    EXPECT_GL_NO_ERROR();
    EXPECT_NE(GL_INVALID_INDEX, index);

    glGetProgramResourceName(program, GL_UNIFORM, index, sizeof(name), &length, name);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(3, length);
    EXPECT_EQ("foo", std::string(name));

    location = glGetProgramResourceLocation(program, GL_UNIFORM, "foo");
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(-1, location);

    glGetProgramResourceiv(program, GL_UNIFORM, index, kPropCount, props.data(), kPropCount,
                           &length, params.data());
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(kPropCount, length);
    EXPECT_EQ(GL_UNSIGNED_INT_ATOMIC_COUNTER, params[0]);  // type
    EXPECT_EQ(1, params[1]);                               // array_size
    EXPECT_EQ(-1, params[2]);                              // location
    EXPECT_EQ(4, params[3]);                               // name_length
    EXPECT_EQ(1, params[4]);                               // referenced_by_vertex_shader
    EXPECT_EQ(0, params[5]);                               // referenced_by_fragment_shader
    EXPECT_EQ(0, params[6]);                               // referenced_by_compute_shader
    EXPECT_EQ(0, params[7]);                               // array_stride
    EXPECT_EQ(-1, params[8]);                              // block_index
    EXPECT_EQ(0, params[9]);                               // is_row_major
    EXPECT_EQ(0, params[10]);                              // matrix_stride
    EXPECT_EQ(4, params[11]);                              // offset
    EXPECT_NE(-1, params[12]);                             // atomic_counter_buffer_index
}

ANGLE_INSTANTIATE_TEST(ProgramInterfaceTestES31, ES31_OPENGL(), ES31_OPENGLES());

}  // anonymous namespace
