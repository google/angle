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

    void runStd140RowMajorMatrixTest(const char *computeShaderSource)
    {
        ANGLE_GL_COMPUTE_PROGRAM(program, computeShaderSource);

        glUseProgram(program);

        constexpr unsigned int kColumns           = 2;
        constexpr unsigned int kRows              = 3;
        constexpr unsigned int kBytesPerComponent = sizeof(float);
        constexpr unsigned int kMatrixStride      = 16;
        // kMatrixStride / kBytesPerComponent is used instead of kColumns is because std140 layout
        // requires that base alignment and stride of arrays of scalars and vectors are rounded up a
        // multiple of the base alignment of a vec4.
        constexpr float kInputDada[kRows][kMatrixStride / kBytesPerComponent] = {
            {0.1, 0.2, 0.0, 0.0}, {0.3, 0.4, 0.0, 0.0}, {0.5, 0.6, 0.0, 0.0}};
        // Create shader storage buffer
        GLBuffer shaderStorageBuffer[2];
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, kRows * kMatrixStride, kInputDada, GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, kRows * kMatrixStride, nullptr, GL_STATIC_DRAW);

        // Bind shader storage buffer
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shaderStorageBuffer[0]);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer[1]);

        glDispatchCompute(1, 1, 1);
        glFinish();

        // Read back shader storage buffer
        constexpr float kExpectedValues[kRows][kColumns] = {{0.1, 0.2}, {0.3, 0.4}, {0.5, 0.6}};
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
        const GLfloat *ptr = reinterpret_cast<const GLfloat *>(
            glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kRows * kMatrixStride, GL_MAP_READ_BIT));
        for (unsigned int idx = 0; idx < kRows; idx++)
        {
            for (unsigned int idy = 0; idy < kColumns; idy++)
            {
                EXPECT_EQ(kExpectedValues[idx][idy],
                          *(ptr + idx * (kMatrixStride / kBytesPerComponent) + idy));
            }
        }

        EXPECT_GL_NO_ERROR();
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
    GLint maxVertexShaderStorageBlocks = 0;
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxVertexShaderStorageBlocks);
    EXPECT_GL_NO_ERROR();
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

// Linking should fail if the sum of the number of active shader storage blocks exceeds
// MAX_COMBINED_SHADER_STORAGE_BLOCKS.
TEST_P(ShaderStorageBufferTest31, ExceedMaxCombinedShaderStorageBlocks)
{
    std::ostringstream vertexInstanceCount;
    GLint maxVertexShaderStorageBlocks = 0;
    glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &maxVertexShaderStorageBlocks);
    vertexInstanceCount << maxVertexShaderStorageBlocks;

    GLint maxFragmentShaderStorageBlocks = 0;
    glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &maxFragmentShaderStorageBlocks);

    GLint maxCombinedShaderStorageBlocks = 0;
    glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &maxCombinedShaderStorageBlocks);
    EXPECT_GL_NO_ERROR();

    ASSERT_GE(maxCombinedShaderStorageBlocks, maxVertexShaderStorageBlocks);
    ASSERT_GE(maxCombinedShaderStorageBlocks, maxFragmentShaderStorageBlocks);

    // As SPEC allows MAX_VERTEX_SHADER_STORAGE_BLOCKS and MAX_FRAGMENT_SHADER_STORAGE_BLOCKS to be
    // 0, in this situation we should skip this test to prevent these unexpected compile errors.
    ANGLE_SKIP_TEST_IF(maxVertexShaderStorageBlocks == 0 || maxFragmentShaderStorageBlocks == 0);

    GLint fragmentShaderStorageBlocks =
        maxCombinedShaderStorageBlocks - maxVertexShaderStorageBlocks + 1;
    ANGLE_SKIP_TEST_IF(fragmentShaderStorageBlocks > maxFragmentShaderStorageBlocks);

    std::ostringstream fragmentInstanceCount;
    fragmentInstanceCount << fragmentShaderStorageBlocks;

    const std::string &vertexShaderSource =
        "#version 310 es\n"
        "layout(shared) buffer blockName0 {\n"
        "    uint data;\n"
        "} instance0[" +
        vertexInstanceCount.str() +
        "];\n"
        "void main()\n"
        "{\n"
        "}\n";
    const std::string &fragmentShaderSource =
        "#version 310 es\n"
        "layout(shared) buffer blockName1 {\n"
        "    uint data;\n"
        "} instance1[" +
        fragmentInstanceCount.str() +
        "];\n"
        "void main()\n"
        "{\n"
        "}\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

// Test shader storage buffer read write.
TEST_P(ShaderStorageBufferTest31, ShaderStorageBufferReadWrite)
{
    const std::string &csSource =
        "#version 310 es\n"
        "layout(local_size_x=1, local_size_y=1, local_size_z=1) in;\n"
        "layout(std140, binding = 1) buffer blockName {\n"
        "    uint data[2];\n"
        "} instanceName;\n"
        "void main()\n"
        "{\n"
        "    instanceName.data[0] = 3u;\n"
        "    instanceName.data[1] = 4u;\n"
        "}\n";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);

    glUseProgram(program.get());

    constexpr unsigned int kElementCount = 2;
    // The array stride are rounded up to the base alignment of a vec4 for std140 layout.
    constexpr unsigned int kArrayStride = 16;
    // Create shader storage buffer
    GLBuffer shaderStorageBuffer;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kElementCount * kArrayStride, nullptr, GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer);

    // Dispath compute
    glDispatchCompute(1, 1, 1);

    glFinish();

    // Read back shader storage buffer
    constexpr unsigned int kExpectedValues[2] = {3u, 4u};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    void *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kElementCount * kArrayStride,
                                 GL_MAP_READ_BIT);
    for (unsigned int idx = 0; idx < kElementCount; idx++)
    {
        EXPECT_EQ(kExpectedValues[idx],
                  *(reinterpret_cast<const GLuint *>(reinterpret_cast<const GLbyte *>(ptr) +
                                                     idx * kArrayStride)));
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    EXPECT_GL_NO_ERROR();
}

// Test that access/write to vector data in shader storage buffer.
TEST_P(ShaderStorageBufferTest31, ShaderStorageBufferVector)
{
    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
 layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
 layout(std140, binding = 0) buffer blockIn {
     uvec2 data;
 } instanceIn;
 layout(std140, binding = 1) buffer blockOut {
     uvec2 data;
 } instanceOut;
 void main()
 {
     instanceOut.data[0] = instanceIn.data[0];
     instanceOut.data[1] = instanceIn.data[1];
 }
 )";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);

    glUseProgram(program.get());

    constexpr unsigned int kComponentCount                  = 2;
    constexpr unsigned int kBytesPerComponent               = sizeof(unsigned int);
    constexpr unsigned int kExpectedValues[kComponentCount] = {3u, 4u};
    // Create shader storage buffer
    GLBuffer shaderStorageBuffer[2];
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kComponentCount * kBytesPerComponent, kExpectedValues,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kComponentCount * kBytesPerComponent, nullptr,
                 GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shaderStorageBuffer[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer[1]);

    glDispatchCompute(1, 1, 1);

    glFinish();

    // Read back shader storage buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    const GLuint *ptr = reinterpret_cast<const GLuint *>(glMapBufferRange(
        GL_SHADER_STORAGE_BUFFER, 0, kComponentCount * kBytesPerComponent, GL_MAP_READ_BIT));
    for (unsigned int idx = 0; idx < kComponentCount; idx++)
    {
        EXPECT_EQ(kExpectedValues[idx], *(ptr + idx));
    }

    EXPECT_GL_NO_ERROR();
}

// Test that access/write to scalar data in matrix in shader storage block with row major.
TEST_P(ShaderStorageBufferTest31, ScalarDataInMatrixInSSBOWithRowMajorQualifier)
{
    // TODO(jiajia.qin@intel.com): Figure out why it fails on Intel Linux platform.
    // http://anglebug.com/1951
    ANGLE_SKIP_TEST_IF(IsIntel() && IsLinux());
    ANGLE_SKIP_TEST_IF(IsAndroid());

    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
layout(std140, binding = 0) buffer blockIn {
    layout(row_major) mat2x3 data;
} instanceIn;
layout(std140, binding = 1) buffer blockOut {
    layout(row_major) mat2x3 data;
} instanceOut;
void main()
{
    instanceOut.data[0][0] = instanceIn.data[0][0];
    instanceOut.data[0][1] = instanceIn.data[0][1];
    instanceOut.data[0][2] = instanceIn.data[0][2];
    instanceOut.data[1][0] = instanceIn.data[1][0];
    instanceOut.data[1][1] = instanceIn.data[1][1];
    instanceOut.data[1][2] = instanceIn.data[1][2];
}
)";

    runStd140RowMajorMatrixTest(kComputeShaderSource);
}

// Test that access/write to scalar data in structure matrix in shader storage block with row major.
TEST_P(ShaderStorageBufferTest31, ScalarDataInMatrixInStructureInSSBOWithRowMajorQualifier)
{
    // TODO(jiajia.qin@intel.com): Figure out why it fails on Intel Linux platform.
    // http://anglebug.com/1951
    ANGLE_SKIP_TEST_IF(IsIntel() && IsLinux());
    ANGLE_SKIP_TEST_IF(IsAndroid());

    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
struct S
{
    mat2x3 data;
};
layout(std140, binding = 0) buffer blockIn {
    layout(row_major) S s;
} instanceIn;
layout(std140, binding = 1) buffer blockOut {
    layout(row_major) S s;
} instanceOut;
void main()
{
    instanceOut.s.data[0][0] = instanceIn.s.data[0][0];
    instanceOut.s.data[0][1] = instanceIn.s.data[0][1];
    instanceOut.s.data[0][2] = instanceIn.s.data[0][2];
    instanceOut.s.data[1][0] = instanceIn.s.data[1][0];
    instanceOut.s.data[1][1] = instanceIn.s.data[1][1];
    instanceOut.s.data[1][2] = instanceIn.s.data[1][2];
}
)";

    runStd140RowMajorMatrixTest(kComputeShaderSource);
}

// Test that access/write to column major matrix data in shader storage buffer.
TEST_P(ShaderStorageBufferTest31, ShaderStorageBufferMatrix)
{
    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
layout(std140, binding = 0) buffer blockIn {
    mat2x3 data;
} instanceIn;
layout(std140, binding = 1) buffer blockOut {
    mat2x3 data;
} instanceOut;
void main()
{
    instanceOut.data[0][0] = instanceIn.data[0][0];
    instanceOut.data[0][1] = instanceIn.data[0][1];
    instanceOut.data[0][2] = instanceIn.data[0][2];
    instanceOut.data[1][0] = instanceIn.data[1][0];
    instanceOut.data[1][1] = instanceIn.data[1][1];
    instanceOut.data[1][2] = instanceIn.data[1][2];
}
)";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);

    glUseProgram(program.get());

    constexpr unsigned int kColumns           = 2;
    constexpr unsigned int kRows              = 3;
    constexpr unsigned int kBytesPerComponent = sizeof(float);
    constexpr unsigned int kVectorStride      = 16;
    // kVectorStride / kBytesPerComponent is used instead of kRows is because std140 layout requires
    // that base alignment and stride of arrays of scalars and vectors are rounded up a multiple of
    // the base alignment of a vec4.
    constexpr float kInputDada[kColumns][kVectorStride / kBytesPerComponent] = {
        {0.1, 0.2, 0.3, 0.0}, {0.4, 0.5, 0.6, 0.0}};
    // Create shader storage buffer
    GLBuffer shaderStorageBuffer[2];
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kColumns * kVectorStride, kInputDada, GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kColumns * kVectorStride, nullptr, GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shaderStorageBuffer[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer[1]);

    glDispatchCompute(1, 1, 1);

    glFinish();

    // Read back shader storage buffer
    constexpr float kExpectedValues[kColumns][kRows] = {{0.1, 0.2, 0.3}, {0.4, 0.5, 0.6}};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    const GLfloat *ptr = reinterpret_cast<const GLfloat *>(
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kColumns * kVectorStride, GL_MAP_READ_BIT));
    for (unsigned int idx = 0; idx < kColumns; idx++)
    {
        for (unsigned int idy = 0; idy < kRows; idy++)
        {
            EXPECT_EQ(kExpectedValues[idx][idy],
                      *(ptr + idx * (kVectorStride / kBytesPerComponent) + idy));
        }
    }

    EXPECT_GL_NO_ERROR();
}

// Test that access/write to structure data in shader storage buffer.
TEST_P(ShaderStorageBufferTest31, ShaderStorageBufferStructureArray)
{
    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
struct S
{
    uvec2 uvData;
    uint uiData[2];
};
layout(std140, binding = 0) buffer blockIn {
    S s[2];
    uint lastData;
} instanceIn;
layout(std140, binding = 1) buffer blockOut {
    S s[2];
    uint lastData;
} instanceOut;
void main()
{
    instanceOut.s[0].uvData = instanceIn.s[0].uvData;
    instanceOut.s[0].uiData[0] = instanceIn.s[0].uiData[0];
    instanceOut.s[0].uiData[1] = instanceIn.s[0].uiData[1];
    instanceOut.s[1].uvData = instanceIn.s[1].uvData;
    instanceOut.s[1].uiData[0] = instanceIn.s[1].uiData[0];
    instanceOut.s[1].uiData[1] = instanceIn.s[1].uiData[1];
    instanceOut.lastData = instanceIn.lastData;
}
)";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);

    glUseProgram(program);

    std::array<GLuint, 4> kUVData = {{
        1u, 2u, 0u, 0u,
    }};
    std::array<GLuint, 8> kUIData = {{
        3u, 0u, 0u, 0u, 4u, 0u, 0u, 0u,
    }};
    GLuint kLastData              = 5u;

    constexpr unsigned int kBytesPerComponent = sizeof(GLuint);
    constexpr unsigned int kStructureStride   = 48;
    constexpr unsigned int totalSize          = kStructureStride * 2 + sizeof(kLastData);

    // Create shader storage buffer
    GLBuffer shaderStorageBuffer[2];
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);
    GLint offset = 0;
    // upload data to instanceIn.s[0]
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUVData.size() * kBytesPerComponent,
                    kUVData.data());
    offset += (kUVData.size() * kBytesPerComponent);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUIData.size() * kBytesPerComponent,
                    kUIData.data());
    offset += (kUIData.size() * kBytesPerComponent);
    // upload data to instanceIn.s[1]
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUVData.size() * kBytesPerComponent,
                    kUVData.data());
    offset += (kUVData.size() * kBytesPerComponent);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUIData.size() * kBytesPerComponent,
                    kUIData.data());
    offset += (kUIData.size() * kBytesPerComponent);
    // upload data to instanceIn.lastData
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(kLastData), &kLastData);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shaderStorageBuffer[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer[1]);

    glDispatchCompute(1, 1, 1);
    glFinish();

    // Read back shader storage buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    constexpr float kExpectedValues[5] = {1u, 2u, 3u, 4u, 5u};
    const GLuint *ptr                  = reinterpret_cast<const GLuint *>(
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, totalSize, GL_MAP_READ_BIT));
    // instanceOut.s[0]
    EXPECT_EQ(kExpectedValues[0], *ptr);
    EXPECT_EQ(kExpectedValues[1], *(ptr + 1));
    EXPECT_EQ(kExpectedValues[2], *(ptr + 4));
    EXPECT_EQ(kExpectedValues[3], *(ptr + 8));
    // instanceOut.s[1]
    ptr += kStructureStride / kBytesPerComponent;
    EXPECT_EQ(kExpectedValues[0], *ptr);
    EXPECT_EQ(kExpectedValues[1], *(ptr + 1));
    EXPECT_EQ(kExpectedValues[2], *(ptr + 4));
    EXPECT_EQ(kExpectedValues[3], *(ptr + 8));
    // instanceOut.lastData
    ptr += kStructureStride / kBytesPerComponent;
    EXPECT_EQ(kExpectedValues[4], *(ptr));

    EXPECT_GL_NO_ERROR();
}

// Test that access/write to array of array structure data in shader storage buffer.
TEST_P(ShaderStorageBufferTest31, ShaderStorageBufferStructureArrayOfArray)
{
    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
struct S
{
    uvec2 uvData;
    uint uiData[2];
};
layout(std140, binding = 0) buffer blockIn {
    S s[3][2];
    uint lastData;
} instanceIn;
layout(std140, binding = 1) buffer blockOut {
    S s[3][2];
    uint lastData;
} instanceOut;
void main()
{
    instanceOut.s[1][0].uvData = instanceIn.s[1][0].uvData;
    instanceOut.s[1][0].uiData[0] = instanceIn.s[1][0].uiData[0];
    instanceOut.s[1][0].uiData[1] = instanceIn.s[1][0].uiData[1];
    instanceOut.s[1][1].uvData = instanceIn.s[1][1].uvData;
    instanceOut.s[1][1].uiData[0] = instanceIn.s[1][1].uiData[0];
    instanceOut.s[1][1].uiData[1] = instanceIn.s[1][1].uiData[1];

    instanceOut.lastData = instanceIn.lastData;
}
)";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);

    glUseProgram(program);

    std::array<GLuint, 4> kUVData = {{
        1u, 2u, 0u, 0u,
    }};
    std::array<GLuint, 8> kUIData = {{
        3u, 0u, 0u, 0u, 4u, 0u, 0u, 0u,
    }};
    GLuint kLastData              = 5u;

    constexpr unsigned int kBytesPerComponent        = sizeof(GLuint);
    constexpr unsigned int kStructureStride          = 48;
    constexpr unsigned int kStructureArrayDimension0 = 3;
    constexpr unsigned int kStructureArrayDimension1 = 2;
    constexpr unsigned int kLastDataOffset =
        kStructureStride * kStructureArrayDimension0 * kStructureArrayDimension1;
    constexpr unsigned int totalSize = kLastDataOffset + sizeof(kLastData);

    GLBuffer shaderStorageBuffer[2];
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);
    // offset of instanceIn.s[1][0]
    GLint offset      = kStructureStride * (kStructureArrayDimension1 * 1 + 0);
    GLuint uintOffset = offset / kBytesPerComponent;
    // upload data to instanceIn.s[1][0]
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUVData.size() * kBytesPerComponent,
                    kUVData.data());
    offset += (kUVData.size() * kBytesPerComponent);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUIData.size() * kBytesPerComponent,
                    kUIData.data());
    offset += (kUIData.size() * kBytesPerComponent);
    // upload data to instanceIn.s[1][1]
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUVData.size() * kBytesPerComponent,
                    kUVData.data());
    offset += (kUVData.size() * kBytesPerComponent);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kUIData.size() * kBytesPerComponent,
                    kUIData.data());
    // upload data to instanceIn.lastData
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, kLastDataOffset, sizeof(kLastData), &kLastData);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shaderStorageBuffer[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer[1]);

    glDispatchCompute(1, 1, 1);
    glFinish();

    // Read back shader storage buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    constexpr float kExpectedValues[5] = {1u, 2u, 3u, 4u, 5u};
    const GLuint *ptr                  = reinterpret_cast<const GLuint *>(
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, totalSize, GL_MAP_READ_BIT));

    // instanceOut.s[0][0]
    EXPECT_EQ(kExpectedValues[0], *(ptr + uintOffset));
    EXPECT_EQ(kExpectedValues[1], *(ptr + uintOffset + 1));
    EXPECT_EQ(kExpectedValues[2], *(ptr + uintOffset + 4));
    EXPECT_EQ(kExpectedValues[3], *(ptr + uintOffset + 8));

    // instanceOut.s[0][1]
    EXPECT_EQ(kExpectedValues[0], *(ptr + uintOffset + 12));
    EXPECT_EQ(kExpectedValues[1], *(ptr + uintOffset + 13));
    EXPECT_EQ(kExpectedValues[2], *(ptr + uintOffset + 16));
    EXPECT_EQ(kExpectedValues[3], *(ptr + uintOffset + 20));

    // instanceOut.lastData
    EXPECT_EQ(kExpectedValues[4], *(ptr + (kLastDataOffset / kBytesPerComponent)));

    EXPECT_GL_NO_ERROR();
}

// Test atomic memory functions.
TEST_P(ShaderStorageBufferTest31, AtomicMemoryFunctions)
{
    // TODO(jiajia.qin@intel.com): Don't skip this test once atomic memory functions for SSBO is
    // supported on d3d backend. http://anglebug.com/1951

    ANGLE_SKIP_TEST_IF(IsD3D11());
    const std::string &csSource =
        R"(#version 310 es

        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(std140, binding = 1) buffer blockName {
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

    constexpr unsigned int kElementCount = 2;
    // The array stride are rounded up to the base alignment of a vec4 for std140 layout.
    constexpr unsigned int kArrayStride = 16;
    // Create shader storage buffer
    GLBuffer shaderStorageBuffer;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kElementCount * kArrayStride, nullptr, GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer);

    // Dispath compute
    glDispatchCompute(1, 1, 1);

    glFinish();

    // Read back shader storage buffer
    constexpr unsigned int kExpectedValues[2] = {5u, 7u};
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer);
    void *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kElementCount * kArrayStride,
                                 GL_MAP_READ_BIT);
    for (unsigned int idx = 0; idx < kElementCount; idx++)
    {
        EXPECT_EQ(kExpectedValues[idx],
                  *(reinterpret_cast<const GLuint *>(reinterpret_cast<const GLbyte *>(ptr) +
                                                     idx * kArrayStride)));
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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

// Test that function calling is supported in SSBO access chain.
TEST_P(ShaderStorageBufferTest31, FunctionCallInSSBOAccessChain)
{
    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout (local_size_x=4) in;
highp uint getIndex (in highp uvec2 localID, uint element)
{
    return localID.x + element;
}
layout(binding=0, std430) buffer Storage
{
    highp uint values[];
} sb_store;

void main()
{
    sb_store.values[getIndex(gl_LocalInvocationID.xy, 0u)] = gl_LocalInvocationIndex;
}
)";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);
    EXPECT_GL_NO_ERROR();
}

// Test that unary operator is supported in SSBO access chain.
TEST_P(ShaderStorageBufferTest31, UnaryOperatorInSSBOAccessChain)
{
    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout (local_size_x=4) in;
layout(binding=0, std430) buffer Storage
{
    highp uint values[];
} sb_store;

void main()
{
    uint invocationNdx = gl_LocalInvocationIndex;
    sb_store.values[++invocationNdx] = invocationNdx;
}
)";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);
    EXPECT_GL_NO_ERROR();
}

// Test that ternary operator is supported in SSBO access chain.
TEST_P(ShaderStorageBufferTest31, TernaryOperatorInSSBOAccessChain)
{
    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout (local_size_x=4) in;
layout(binding=0, std430) buffer Storage
{
    highp uint values[];
} sb_store;

void main()
{
    sb_store.values[gl_LocalInvocationIndex > 2u ? gl_NumWorkGroups.x : gl_NumWorkGroups.y]
            = gl_LocalInvocationIndex;
}
)";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);
    EXPECT_GL_NO_ERROR();
}

TEST_P(ShaderStorageBufferTest31, LoadAndStoreBooleanValue)
{
    // TODO(jiajia.qin@intel.com): Figure out why it fails on Intel Linux platform.
    // http://anglebug.com/1951
    ANGLE_SKIP_TEST_IF(IsIntel() && IsLinux());

    constexpr char kComputeShaderSource[] =
        R"(#version 310 es
layout (local_size_x=1) in;
layout(binding=0, std140) buffer Storage0
{
    bool b1;
    bvec2 b2;
} sb_load;
layout(binding=1, std140) buffer Storage1
{
    bool b1;
    bvec2 b2;
} sb_store;
void main()
{
   sb_store.b1 = sb_load.b1;
   sb_store.b2 = sb_load.b2;
}
)";

    ANGLE_GL_COMPUTE_PROGRAM(program, kComputeShaderSource);
    EXPECT_GL_NO_ERROR();

    glUseProgram(program);

    constexpr GLuint kB1Value                 = 1u;
    constexpr GLuint kB2Value[2]              = {0u, 1u};
    constexpr unsigned int kBytesPerComponent = sizeof(GLuint);
    // Create shader storage buffer
    GLBuffer shaderStorageBuffer[2];
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[0]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * kBytesPerComponent, nullptr, GL_STATIC_DRAW);
    GLint offset = 0;
    // upload data to sb_load.b1
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, kBytesPerComponent, &kB1Value);
    offset += kBytesPerComponent;
    // upload data to sb_load.b2
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, 2 * kBytesPerComponent, kB2Value);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 3 * kBytesPerComponent, nullptr, GL_STATIC_DRAW);

    // Bind shader storage buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, shaderStorageBuffer[0]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, shaderStorageBuffer[1]);

    glDispatchCompute(1, 1, 1);
    glFinish();

    // Read back shader storage buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, shaderStorageBuffer[1]);
    const GLboolean *ptr = reinterpret_cast<const GLboolean *>(
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, 3 * kBytesPerComponent, GL_MAP_READ_BIT));
    EXPECT_EQ(GL_TRUE, *ptr);
    ptr += kBytesPerComponent;
    EXPECT_EQ(GL_FALSE, *ptr);
    ptr += kBytesPerComponent;
    EXPECT_EQ(GL_TRUE, *ptr);

    EXPECT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST(ShaderStorageBufferTest31, ES31_OPENGL(), ES31_OPENGLES(), ES31_D3D11());

}  // namespace
