//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderStorageBufferTest:
//   Various tests related for shader storage buffers.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

class ShaderStorageBufferTest31 : public ANGLETest
{
  protected:
    ShaderStorageBufferTest31()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

// Matched block names within a shader interface must match in terms of having the same number of
// declarations with the same sequence of types.
TEST_P(ShaderStorageBufferTest31, MatchedBlockNameWithDifferentMemberType)
{
    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "buffer blockName {\n"
        "    float data;\n"
        "};\n"
        "void main()\n"
        "{\n"
        "}\n";
    const std::string &fragmentShaderSource =
        "#version 310 es\n"
        "buffer blockName {\n"
        "    uint data;\n"
        "};\n"
        "void main()\n"
        "{\n"
        "}\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

// Linking should fail if blocks in vertex shader exceed GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS.
TEST_P(ShaderStorageBufferTest31, ExceedMaxVertexShaderStorageBlocks)
{
    std::ostringstream instanceCount;
    GLint maxVertexShaderStorageBlocks;
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxVertexShaderStorageBlocks);
    instanceCount << maxVertexShaderStorageBlocks;

    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "layout(shared) buffer blockName {\n"
        "    uint data;\n"
        "} instance[" +
        instanceCount.str() +
        " + 1];\n"
        "void main()\n"
        "{\n"
        "}\n";
    const std::string &fragmentShaderSource =
        "#version 310 es\n"
        "void main()\n"
        "{\n"
        "}\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

// Test shader storage buffer read write.
TEST_P(ShaderStorageBufferTest31, ShaderStorageBufferReadWrite)
{
    // TODO(jiajia.qin@intel.com): Figure out why it fails on AMD platform.
    ANGLE_SKIP_TEST_IF(IsAMD() && IsDesktopOpenGL());
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsOpenGL());

    const std::string &csSource =
        "#version 310 es\n"
        "layout(local_size_x=1, local_size_y=1, local_size_z=1) in;\n"
        "layout(binding = 1) buffer blockName {\n"
        "    uint data[2];\n"
        "} instanceName;\n"
        "void main()\n"
        "{\n"
        "    instanceName.data[0] = 3u;\n"
        "    if (instanceName.data[0] == 3u)\n"
        "        instanceName.data[1] = 4u;\n"
        "    else\n"
        "        instanceName.data[1] = 5u;\n"
        "}\n";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);

    glUseProgram(program.get());

    unsigned int bufferData[2] = {0u};
    // Create shader storage buffer
    GLBuffer shaderStorageBuffer;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(bufferData), nullptr, GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer);

    // Dispath compute
    glDispatchCompute(1, 1, 1);

    glFinish();

    // Read back shader storage buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    void *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bufferData), GL_MAP_READ_BIT);
    memcpy(bufferData, ptr, sizeof(bufferData));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    EXPECT_EQ(3u, bufferData[0]);
    EXPECT_EQ(4u, bufferData[1]);

    EXPECT_GL_NO_ERROR();
}

// Test atomic memory functions.
TEST_P(ShaderStorageBufferTest31, AtomicMemoryFunctions)
{
    ANGLE_SKIP_TEST_IF(IsAMD() && IsDesktopOpenGL() && IsWindows());
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsOpenGL());
    // anglebug.com/2255
    ANGLE_SKIP_TEST_IF(IsLinux() && IsAMD() && IsDesktopOpenGL());

    const std::string &csSource =
        R"(#version 310 es

        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(binding = 1) buffer blockName {
            uint data[2];
        } instanceName;

        void main()
        {
            instanceName.data[0] = 0u;
            instanceName.data[1] = 0u;
            atomicAdd(instanceName.data[0], 5u);
            atomicMax(instanceName.data[1], 7u);

        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);

    glUseProgram(program.get());

    unsigned int bufferData[2] = {0u};
    // Create shader storage buffer
    GLBuffer shaderStorageBuffer;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(bufferData), nullptr, GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer);

    // Dispath compute
    glDispatchCompute(1, 1, 1);

    glFinish();

    // Read back shader storage buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    void *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(bufferData), GL_MAP_READ_BIT);
    memcpy(bufferData, ptr, sizeof(bufferData));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    EXPECT_EQ(5u, bufferData[0]);
    EXPECT_EQ(7u, bufferData[1]);

    EXPECT_GL_NO_ERROR();
}

// Test multiple storage buffers work correctly when program switching. In angle, storage buffer
// bindings are updated accord to current program. If switch program, need to update storage buffer
// bindings again.
TEST_P(ShaderStorageBufferTest31, MultiStorageBuffersForMultiPrograms)
{
    const std::string &csSource1 =
        R"(#version 310 es
        layout(local_size_x=3, local_size_y=1, local_size_z=1) in;
        layout(binding = 1) buffer Output {
            uint result1[];
        } sb_out1;
        void main()
        {
            highp uint offset = gl_LocalInvocationID.x;
            sb_out1.result1[gl_LocalInvocationIndex] = gl_LocalInvocationIndex + 1u;
        })";

    const std::string &csSource2 =
        R"(#version 310 es
        layout(local_size_x=3, local_size_y=1, local_size_z=1) in;
        layout(binding = 2) buffer Output {
            uint result2[];
        } sb_out2;
        void main()
        {
            highp uint offset = gl_LocalInvocationID.x;
            sb_out2.result2[gl_LocalInvocationIndex] = gl_LocalInvocationIndex + 2u;
        })";

    constexpr unsigned int numInvocations = 3;
    int arrayStride1 = 0, arrayStride2 = 0;
    GLenum props[] = {GL_ARRAY_STRIDE};
    GLBuffer shaderStorageBuffer1, shaderStorageBuffer2;

    ANGLE_GL_COMPUTE_PROGRAM(program1, csSource1);
    ANGLE_GL_COMPUTE_PROGRAM(program2, csSource2);
    EXPECT_GL_NO_ERROR();

    unsigned int outVarIndex1 =
        glGetProgramResourceIndex(program1.get(), GL_BUFFER_VARIABLE, "Output.result1");
    glGetProgramResourceiv(program1.get(), GL_BUFFER_VARIABLE, outVarIndex1, 1, props, 1, 0,
                           &arrayStride1);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numInvocations * arrayStride1, nullptr, GL_STREAM_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer1);
    EXPECT_GL_NO_ERROR();

    unsigned int outVarIndex2 =
        glGetProgramResourceIndex(program2.get(), GL_BUFFER_VARIABLE, "Output.result2");
    glGetProgramResourceiv(program2.get(), GL_BUFFER_VARIABLE, outVarIndex2, 1, props, 1, 0,
                           &arrayStride2);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer2);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numInvocations * arrayStride2, nullptr, GL_STREAM_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, shaderStorageBuffer2);
    EXPECT_GL_NO_ERROR();

    glUseProgram(program1.get());
    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();
    glUseProgram(program2.get());
    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer1);
    const void *ptr1 =
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 3 * arrayStride1, GL_MAP_READ_BIT);
    for (unsigned int idx = 0; idx < numInvocations; idx++)
    {
        EXPECT_EQ(idx + 1, *((const GLuint *)((const GLbyte *)ptr1 + idx * arrayStride1)));
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    EXPECT_GL_NO_ERROR();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer2);
    const void *ptr2 =
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 3 * arrayStride2, GL_MAP_READ_BIT);
    EXPECT_GL_NO_ERROR();
    for (unsigned int idx = 0; idx < numInvocations; idx++)
    {
        EXPECT_EQ(idx + 2, *((const GLuint *)((const GLbyte *)ptr2 + idx * arrayStride2)));
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    EXPECT_GL_NO_ERROR();

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    EXPECT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST(ShaderStorageBufferTest31, ES31_OPENGL(), ES31_OPENGLES());

}  // namespace
