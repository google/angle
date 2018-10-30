//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

class InstancingTest : public ANGLETest
{
  protected:
    InstancingTest() : mProgram(0), mVertexBuffer(0)
    {
        setWindowWidth(256);
        setWindowHeight(256);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    ~InstancingTest() override
    {
        glDeleteBuffers(1, &mVertexBuffer);
        glDeleteProgram(mProgram);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        // Initialize the vertex and index vectors
        constexpr GLfloat qvertex1[3] = {-quadRadius, quadRadius, 0.0f};
        constexpr GLfloat qvertex2[3] = {-quadRadius, -quadRadius, 0.0f};
        constexpr GLfloat qvertex3[3] = {quadRadius, -quadRadius, 0.0f};
        constexpr GLfloat qvertex4[3] = {quadRadius, quadRadius, 0.0f};
        mQuadVertices.insert(mQuadVertices.end(), qvertex1, qvertex1 + 3);
        mQuadVertices.insert(mQuadVertices.end(), qvertex2, qvertex2 + 3);
        mQuadVertices.insert(mQuadVertices.end(), qvertex3, qvertex3 + 3);
        mQuadVertices.insert(mQuadVertices.end(), qvertex4, qvertex4 + 3);

        constexpr GLfloat coord1[2] = {0.0f, 0.0f};
        constexpr GLfloat coord2[2] = {0.0f, 1.0f};
        constexpr GLfloat coord3[2] = {1.0f, 1.0f};
        constexpr GLfloat coord4[2] = {1.0f, 0.0f};
        mTexcoords.insert(mTexcoords.end(), coord1, coord1 + 2);
        mTexcoords.insert(mTexcoords.end(), coord2, coord2 + 2);
        mTexcoords.insert(mTexcoords.end(), coord3, coord3 + 2);
        mTexcoords.insert(mTexcoords.end(), coord4, coord4 + 2);

        mIndices.push_back(0);
        mIndices.push_back(1);
        mIndices.push_back(2);
        mIndices.push_back(0);
        mIndices.push_back(2);
        mIndices.push_back(3);

        for (size_t vertexIndex = 0; vertexIndex < 6; ++vertexIndex)
        {
            mNonIndexedVertices.insert(mNonIndexedVertices.end(),
                                       mQuadVertices.begin() + mIndices[vertexIndex] * 3,
                                       mQuadVertices.begin() + mIndices[vertexIndex] * 3 + 3);
        }

        for (size_t vertexIndex = 0; vertexIndex < 6; ++vertexIndex)
        {
            mNonIndexedVertices.insert(mNonIndexedVertices.end(),
                                       mQuadVertices.begin() + mIndices[vertexIndex] * 3,
                                       mQuadVertices.begin() + mIndices[vertexIndex] * 3 + 3);
        }

        // Tile a 2x2 grid of the tiles
        for (float y = -1.0f + quadRadius; y < 1.0f - quadRadius; y += quadRadius * 3)
        {
            for (float x = -1.0f + quadRadius; x < 1.0f - quadRadius; x += quadRadius * 3)
            {
                const GLfloat instance[3] = {x + quadRadius, y + quadRadius, 0.0f};
                mInstances.insert(mInstances.end(), instance, instance + 3);
            }
        }

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        glGenBuffers(1, &mVertexBuffer);

        ASSERT_GL_NO_ERROR();
    }

    void setupDrawArraysTest(const std::string &vs)
    {
        mProgram = CompileProgram(vs, essl1_shaders::fs::Red());
        ASSERT_NE(0u, mProgram);

        // Set the viewport
        glViewport(0, 0, getWindowWidth(), getWindowHeight());

        // Clear the color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Use the program object
        glUseProgram(mProgram);
    }

    void setupInstancedPointsTest()
    {
        mIndices.clear();
        mIndices.push_back(0);
        mIndices.push_back(1);
        mIndices.push_back(2);
        mIndices.push_back(3);

        // clang-format off
        const std::string vs =
            "attribute vec3 a_position;\n"
            "attribute vec3 a_instancePos;\n"
            "void main()\n"
            "{\n"
            "    gl_Position  = vec4(a_position.xyz, 1.0);\n"
            "    gl_Position  = vec4(a_instancePos.xyz, 1.0);\n"
            "    gl_PointSize = 6.0;\n"
            "}\n";
        // clang-format on

        mProgram = CompileProgram(vs, essl1_shaders::fs::Red());
        ASSERT_NE(0u, mProgram);

        // Set the viewport
        glViewport(0, 0, getWindowWidth(), getWindowHeight());

        // Clear the color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Use the program object
        glUseProgram(mProgram);
    }

    void runDrawArraysTest(GLint first, GLsizei count, GLsizei instanceCount, const float *offset)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, mInstances.size() * sizeof(mInstances[0]), &mInstances[0],
                     GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Get the attribute locations
        GLint positionLoc    = glGetAttribLocation(mProgram, "a_position");
        GLint instancePosLoc = glGetAttribLocation(mProgram, "a_instancePos");

        // Load the vertex position
        glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, mNonIndexedVertices.data());
        glEnableVertexAttribArray(positionLoc);

        // Load the instance position
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glVertexAttribPointer(instancePosLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glEnableVertexAttribArray(instancePosLoc);

        // Enable instancing
        glVertexAttribDivisorANGLE(instancePosLoc, 1);

        // Offset
        GLint uniformLoc = glGetUniformLocation(mProgram, "u_offset");
        ASSERT_NE(-1, uniformLoc);
        glUniform3fv(uniformLoc, 1, offset);

        // Do the instanced draw
        glDrawArraysInstancedANGLE(GL_TRIANGLES, first, count, instanceCount);

        ASSERT_GL_NO_ERROR();
    }

    virtual void runDrawElementsTest(std::string vs, bool shouldAttribZeroBeInstanced)
    {
        const std::string fs =
            "precision mediump float;\n"
            "void main()\n"
            "{\n"
            "    gl_FragColor = vec4(1.0, 0, 0, 1.0);\n"
            "}\n";

        ANGLE_GL_PROGRAM(program, vs, fs);

        // Get the attribute locations
        GLint positionLoc    = glGetAttribLocation(program, "a_position");
        GLint instancePosLoc = glGetAttribLocation(program, "a_instancePos");

        // If this ASSERT fails then the vertex shader code should be refactored
        ASSERT_EQ(shouldAttribZeroBeInstanced, (instancePosLoc == 0));

        // Set the viewport
        glViewport(0, 0, getWindowWidth(), getWindowHeight());

        // Clear the color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Use the program object
        glUseProgram(program);

        // Load the vertex position
        glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, mQuadVertices.data());
        glEnableVertexAttribArray(positionLoc);

        // Load the instance position
        glVertexAttribPointer(instancePosLoc, 3, GL_FLOAT, GL_FALSE, 0, mInstances.data());
        glEnableVertexAttribArray(instancePosLoc);

        // Enable instancing
        glVertexAttribDivisorANGLE(instancePosLoc, 1);

        // Do the instanced draw
        glDrawElementsInstancedANGLE(GL_TRIANGLES, static_cast<GLsizei>(mIndices.size()),
                                     GL_UNSIGNED_SHORT, mIndices.data(),
                                     static_cast<GLsizei>(mInstances.size()) / 3);

        ASSERT_GL_NO_ERROR();

        checkQuads();
    }

    void checkQuads()
    {
        // Check that various pixels are the expected color.
        for (unsigned int quadIndex = 0; quadIndex < 4; ++quadIndex)
        {
            unsigned int baseOffset = quadIndex * 3;

            int quadx =
                static_cast<int>(((mInstances[baseOffset + 0]) * 0.5f + 0.5f) * getWindowWidth());
            int quady =
                static_cast<int>(((mInstances[baseOffset + 1]) * 0.5f + 0.5f) * getWindowHeight());

            EXPECT_PIXEL_EQ(quadx, quady, 255, 0, 0, 255);
        }
    }

    // Vertex data
    std::vector<GLfloat> mQuadVertices;
    std::vector<GLfloat> mNonIndexedVertices;
    std::vector<GLfloat> mTexcoords;
    std::vector<GLfloat> mInstances;
    std::vector<GLushort> mIndices;

    static constexpr GLfloat quadRadius = 0.30f;

    GLuint mProgram;
    GLuint mVertexBuffer;
};

class InstancingTestAllConfigs : public InstancingTest
{
  protected:
    InstancingTestAllConfigs() {}
};

class InstancingTestNo9_3 : public InstancingTest
{
  protected:
    InstancingTestNo9_3() {}
};

class InstancingTestPoints : public InstancingTest
{
  protected:
    InstancingTestPoints() {}
};

// This test uses a vertex shader with the first attribute (attribute zero) instanced.
// On D3D9 and D3D11 FL9_3, this triggers a special codepath that rearranges the input layout sent
// to D3D, to ensure that slot/stream zero of the input layout doesn't contain per-instance data.
TEST_P(InstancingTestAllConfigs, AttributeZeroInstanced)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    const std::string vs =
        "attribute vec3 a_instancePos;\n"
        "attribute vec3 a_position;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_position.xyz + a_instancePos.xyz, 1.0);\n"
        "}\n";

    runDrawElementsTest(vs, true);
}

// Same as AttributeZeroInstanced, but attribute zero is not instanced.
// This ensures the general instancing codepath (i.e. without rearranging the input layout) works as
// expected.
TEST_P(InstancingTestAllConfigs, AttributeZeroNotInstanced)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    const std::string vs =
        "attribute vec3 a_position;\n"
        "attribute vec3 a_instancePos;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_position.xyz + a_instancePos.xyz, 1.0);\n"
        "}\n";

    runDrawElementsTest(vs, false);
}

// Tests that the "first" parameter to glDrawArraysInstancedANGLE is only an offset into
// the non-instanced vertex attributes.
TEST_P(InstancingTestNo9_3, DrawArraysWithOffset)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    const std::string vs =
        "attribute vec3 a_position;\n"
        "attribute vec3 a_instancePos;\n"
        "uniform vec3 u_offset;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_position.xyz + a_instancePos.xyz + u_offset, 1.0);\n"
        "}\n";

    setupDrawArraysTest(vs);

    constexpr float offset1[3] = {0, 0, 0};
    runDrawArraysTest(0, 6, 2, offset1);

    constexpr float offset2[3] = {0.0f, 1.0f, 0};
    runDrawArraysTest(6, 6, 2, offset2);

    checkQuads();
}

// This test verifies instancing with GL_POINTS with glDrawArraysInstanced works.
// On D3D11 FL9_3, this triggers a special codepath that emulates instanced points rendering.
TEST_P(InstancingTestPoints, DrawArrays)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    // Disable D3D11 SDK Layers warnings checks, see ANGLE issue 667 for details
    // On Win7, the D3D SDK Layers emits a false warning for these tests.
    // This doesn't occur on Windows 10 (Version 1511) though.
    ignoreD3D11SDKLayersWarnings();

    setupInstancedPointsTest();

    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, mInstances.size() * sizeof(mInstances[0]), &mInstances[0],
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Get the attribute locations
    GLint positionLoc    = glGetAttribLocation(mProgram, "a_position");
    GLint instancePosLoc = glGetAttribLocation(mProgram, "a_instancePos");

    // Load the vertex position
    constexpr GLfloat pos[3] = {0, 0, 0};
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, pos);
    glEnableVertexAttribArray(positionLoc);

    // Load the instance position
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glVertexAttribPointer(instancePosLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(instancePosLoc);

    // Enable instancing
    glVertexAttribDivisorANGLE(instancePosLoc, 1);

    // Do the instanced draw
    glDrawArraysInstancedANGLE(GL_POINTS, 0, 1, static_cast<GLsizei>(mInstances.size()) / 3);

    ASSERT_GL_NO_ERROR();

    checkQuads();
}

// This test verifies instancing with GL_POINTS with glDrawElementsInstanced works.
// On D3D11 FL9_3, this triggers a special codepath that emulates instanced points rendering.
TEST_P(InstancingTestPoints, DrawElements)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    // Disable D3D11 SDK Layers warnings checks, see ANGLE issue 667 for details
    // On Win7, the D3D SDK Layers emits a false warning for these tests.
    // This doesn't occur on Windows 10 (Version 1511) though.
    ignoreD3D11SDKLayersWarnings();

    setupInstancedPointsTest();

    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, mInstances.size() * sizeof(mInstances[0]), &mInstances[0],
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Get the attribute locations
    GLint positionLoc    = glGetAttribLocation(mProgram, "a_position");
    GLint instancePosLoc = glGetAttribLocation(mProgram, "a_instancePos");

    // Load the vertex position
    const Vector3 pos[] = {Vector3(0), Vector3(0), Vector3(0), Vector3(0)};
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, pos);
    glEnableVertexAttribArray(positionLoc);

    // Load the instance position
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glVertexAttribPointer(instancePosLoc, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glEnableVertexAttribArray(instancePosLoc);

    // Enable instancing
    glVertexAttribDivisorANGLE(instancePosLoc, 1);

    // Do the instanced draw
    glDrawElementsInstancedANGLE(GL_POINTS, static_cast<GLsizei>(mIndices.size()),
                                 GL_UNSIGNED_SHORT, mIndices.data(),
                                 static_cast<GLsizei>(mInstances.size()) / 3);

    ASSERT_GL_NO_ERROR();

    checkQuads();
}

class InstancingTestES3 : public InstancingTest
{
  public:
    InstancingTestES3() {}
};

class InstancingTestES31 : public InstancingTest
{
  public:
    InstancingTestES31() {}
};

// Verify that VertexAttribDivisor can update both binding divisor and attribBinding.
TEST_P(InstancingTestES31, UpdateAttribBindingByVertexAttribDivisor)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    const std::string vs =
        "attribute vec3 a_instancePos;\n"
        "attribute vec3 a_position;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_position.xyz + a_instancePos.xyz, 1.0);\n"
        "}\n";

    const std::string fs =
        "precision mediump float;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(1.0, 0, 0, 1.0);\n"
        "}\n";

    constexpr GLsizei kFloatStride = 4;

    ANGLE_GL_PROGRAM(program, vs, fs);
    glUseProgram(program);

    // Get the attribute locations
    GLint positionLoc    = glGetAttribLocation(program, "a_position");
    GLint instancePosLoc = glGetAttribLocation(program, "a_instancePos");
    ASSERT_NE(-1, positionLoc);
    ASSERT_NE(-1, instancePosLoc);
    ASSERT_GL_NO_ERROR();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLBuffer quadBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, mQuadVertices.size() * kFloatStride, mQuadVertices.data(),
                 GL_STATIC_DRAW);
    GLBuffer instancesBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, instancesBuffer);
    glBufferData(GL_ARRAY_BUFFER, mInstances.size() * kFloatStride, mInstances.data(),
                 GL_STATIC_DRAW);

    // Set the formats by VertexAttribFormat
    glVertexAttribFormat(positionLoc, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribFormat(instancePosLoc, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(instancePosLoc);

    const GLint positionBinding = instancePosLoc;
    const GLint instanceBinding = positionLoc;

    // Load the vertex position into the binding indexed positionBinding (== instancePosLoc)
    // Load the instance position into the binding indexed instanceBinding (== positionLoc)
    glBindVertexBuffer(positionBinding, quadBuffer, 0, kFloatStride * 3);
    glBindVertexBuffer(instanceBinding, instancesBuffer, 0, kFloatStride * 3);

    // The attribute indexed positionLoc is using the binding indexed positionBinding
    // The attribute indexed instancePosLoc is using the binding indexed instanceBinding
    glVertexAttribBinding(positionLoc, positionBinding);
    glVertexAttribBinding(instancePosLoc, instanceBinding);

    // Enable instancing on the binding indexed instanceBinding
    glVertexBindingDivisor(instanceBinding, 1);

    // Do the first instanced draw
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mIndices.size()), GL_UNSIGNED_SHORT,
                            mIndices.data(), static_cast<GLsizei>(mInstances.size()) / 3);
    checkQuads();

    // Disable instancing.
    glVertexBindingDivisor(instanceBinding, 0);

    // Load the vertex position into the binding indexed positionLoc.
    // Load the instance position into the binding indexed instancePosLoc.
    glBindVertexBuffer(positionLoc, quadBuffer, 0, kFloatStride * 3);
    glBindVertexBuffer(instancePosLoc, instancesBuffer, 0, kFloatStride * 3);

    // The attribute indexed positionLoc is using the binding indexed positionLoc.
    glVertexAttribBinding(positionLoc, positionLoc);

    // Call VertexAttribDivisor to both enable instancing on instancePosLoc and set the attribute
    // indexed instancePosLoc using the binding indexed instancePosLoc.
    glVertexAttribDivisor(instancePosLoc, 1);

    // Do the second instanced draw
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(mIndices.size()), GL_UNSIGNED_SHORT,
                            mIndices.data(), static_cast<GLsizei>(mInstances.size()) / 3);
    checkQuads();

    glDeleteVertexArrays(1, &vao);
}

// Verify that a large divisor that also changes doesn't cause issues and renders correctly.
TEST_P(InstancingTestES3, LargeDivisor)
{
    const std::string &vs = R"(#version 300 es
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;
out vec4 v_color;
void main()
{
    gl_Position = a_position;
    gl_PointSize = 4.0f;
    v_color = a_color;
})";

    const std::string &fs = R"(#version 300 es
precision highp float;
in vec4 v_color;
out vec4 my_FragColor;
void main()
{
    my_FragColor = v_color;
})";

    ANGLE_GL_PROGRAM(program, vs, fs);
    glUseProgram(program);

    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);

    GLBuffer buf;
    glBindBuffer(GL_ARRAY_BUFFER, buf);
    std::vector<GLfloat> vertices;
    for (size_t i = 0u; i < 4u; ++i)
    {
        vertices.push_back(0.0f + i * 0.25f);
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);
        vertices.push_back(1.0f);
    }
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * 4u, vertices.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, nullptr);
    ASSERT_GL_NO_ERROR();

    GLBuffer colorBuf;
    glBindBuffer(GL_ARRAY_BUFFER, colorBuf);

    std::array<GLColor, 4> ubyteColors = {GLColor::red, GLColor::green};
    std::vector<float> floatColors;
    for (const GLColor &color : ubyteColors)
    {
        floatColors.push_back(color.R / 255.0f);
        floatColors.push_back(color.G / 255.0f);
        floatColors.push_back(color.B / 255.0f);
        floatColors.push_back(color.A / 255.0f);
    }
    glBufferData(GL_ARRAY_BUFFER, floatColors.size() * 4u, floatColors.data(), GL_DYNAMIC_DRAW);

    const GLuint kColorDivisor = 65536u * 2u;
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, false, 0, nullptr);
    glVertexAttribDivisor(1, kColorDivisor);

    std::array<GLuint, 1u> indices       = {0u};
    std::array<GLuint, 3u> divisorsToTry = {256u, 65536u, 65536u * 2u};

    for (GLuint divisorToTry : divisorsToTry)
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glVertexAttribDivisor(0, divisorToTry);

        GLuint instanceCount        = divisorToTry + 1u;
        unsigned int pointsRendered = (instanceCount - 1u) / divisorToTry + 1u;

        glDrawElementsInstanced(GL_POINTS, indices.size(), GL_UNSIGNED_INT, indices.data(),
                                instanceCount);
        ASSERT_GL_NO_ERROR();

        // Check that the intended number of points has been rendered.
        for (unsigned int pointIndex = 0u; pointIndex < pointsRendered + 1u; ++pointIndex)
        {
            GLint pointx = static_cast<GLint>((pointIndex * 0.125f + 0.5f) * getWindowWidth());
            GLint pointy = static_cast<GLint>(0.5f * getWindowHeight());

            if (pointIndex < pointsRendered)
            {
                GLuint pointColorIndex = (pointIndex * divisorToTry) / kColorDivisor;
                EXPECT_PIXEL_COLOR_EQ(pointx, pointy, ubyteColors[pointColorIndex]);
            }
            else
            {
                // Clear color.
                EXPECT_PIXEL_COLOR_EQ(pointx, pointy, GLColor::blue);
            }
        }
    }
}

// This is a regression test. If VertexAttribDivisor was returned as a signed integer, it would be
// incorrectly clamped down to the maximum signed integer.
TEST_P(InstancingTestES3, LargestDivisor)
{
    constexpr GLuint kLargeDivisor = std::numeric_limits<GLuint>::max();
    glVertexAttribDivisor(0, kLargeDivisor);

    GLuint divisor = 0;
    glGetVertexAttribIuiv(0, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
    EXPECT_EQ(kLargeDivisor, divisor)
        << "Vertex attrib divisor read was not the same that was passed in.";
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against. We test on D3D9 and D3D11 9_3 because they use special codepaths
// when attribute zero is instanced, unlike D3D11.
ANGLE_INSTANTIATE_TEST(InstancingTestAllConfigs,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES(),
                       ES2_VULKAN());

// TODO(jmadill): Figure out the situation with DrawInstanced on FL 9_3
ANGLE_INSTANTIATE_TEST(InstancingTestNo9_3, ES2_D3D9(), ES2_D3D11());

ANGLE_INSTANTIATE_TEST(InstancingTestPoints, ES2_D3D11(), ES2_D3D11_FL9_3());

ANGLE_INSTANTIATE_TEST(InstancingTestES3, ES3_OPENGL(), ES3_OPENGLES(), ES3_D3D11());

ANGLE_INSTANTIATE_TEST(InstancingTestES31, ES31_OPENGL(), ES31_OPENGLES(), ES31_D3D11());
