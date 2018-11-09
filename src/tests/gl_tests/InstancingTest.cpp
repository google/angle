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
    InstancingTest()
    {
        setWindowWidth(256);
        setWindowHeight(256);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void TearDown() override
    {
        glDeleteBuffers(1, &mInstanceBuffer);
        glDeleteProgram(mProgram0);
        glDeleteProgram(mProgram1);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        for (int i = 0; i < kMaxDrawn; ++i)
        {
            mInstanceData[i] = i * kDrawSize;
        }
        glGenBuffers(1, &mInstanceBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mInstanceBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mInstanceData), mInstanceData, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        const std::string inst = "attribute float a_instance;";
        const std::string pos  = "attribute vec2 a_position;";
        const std::string main = R"(
            void main()
            {
                gl_PointSize = 6.0;
                gl_Position = vec4(a_position.x, a_position.y + a_instance, 0, 1);
            }
        )";

        // attrib 0 is instanced
        mProgram0 = CompileProgram(inst + pos + main, essl1_shaders::fs::Red());
        ASSERT_NE(0u, mProgram0);
        ASSERT_EQ(0, glGetAttribLocation(mProgram0, "a_instance"));
        ASSERT_EQ(1, glGetAttribLocation(mProgram0, "a_position"));

        // attrib 1 is instanced
        mProgram1 = CompileProgram(pos + inst + main, essl1_shaders::fs::Red());
        ASSERT_NE(0u, mProgram1);
        ASSERT_EQ(1, glGetAttribLocation(mProgram1, "a_instance"));
        ASSERT_EQ(0, glGetAttribLocation(mProgram1, "a_position"));

        glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    }

    void runTest(
        unsigned numInstance,
        unsigned divisor,
        bool attribZeroInstanced,  // true: attrib 0 is instance, false: attrib 1 is instanced
        bool points,               // true: draw points, false: draw quad
        bool indexed,              // true: DrawElements, false: DrawArrays
        bool offset,               // true: pass nonzero offset to DrawArrays, false: zero offset
        bool buffer)               // true: use instance data in buffer, false: in client memory
    {
        // The window is divided into kMaxDrawn slices of size kDrawSize.
        // The slice drawn into is determined by the instance datum.
        // The instance data array selects all the slices in order.
        // 'lastDrawn' is the index (zero-based) of the last slice into which we draw.
        const unsigned lastDrawn = (numInstance - 1) / divisor;
        ASSERT(lastDrawn < kMaxDrawn);

        const int instanceAttrib = attribZeroInstanced ? 0 : 1;
        const int positionAttrib = attribZeroInstanced ? 1 : 0;

        glUseProgram(attribZeroInstanced ? mProgram0 : mProgram1);

        glBindBuffer(GL_ARRAY_BUFFER, buffer ? mInstanceBuffer : 0);
        glVertexAttribPointer(instanceAttrib, 1, GL_FLOAT, GL_FALSE, 0,
                              buffer ? nullptr : mInstanceData);
        glEnableVertexAttribArray(instanceAttrib);
        glVertexAttribDivisorANGLE(instanceAttrib, divisor);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribPointer(positionAttrib, 2, GL_FLOAT, GL_FALSE, 0,
                              points ? kPointVertices : kQuadVertices);
        glEnableVertexAttribArray(positionAttrib);
        glVertexAttribDivisorANGLE(positionAttrib, 0);

        glClear(GL_COLOR_BUFFER_BIT);

        if (points)
        {
            if (indexed)
                glDrawElementsInstancedANGLE(GL_POINTS, ArraySize(kPointIndices), GL_UNSIGNED_SHORT,
                                             kPointIndices, numInstance);
            else
                glDrawArraysInstancedANGLE(GL_POINTS, offset ? 2 : 0, 4, numInstance);
        }
        else
        {
            if (indexed)
                glDrawElementsInstancedANGLE(GL_TRIANGLES, ArraySize(kQuadIndices),
                                             GL_UNSIGNED_SHORT, kQuadIndices, numInstance);
            else
                glDrawArraysInstancedANGLE(GL_TRIANGLES, offset ? 4 : 0, 6, numInstance);
        }

        ASSERT_GL_NO_ERROR();
        checkDrawing(lastDrawn);
    }

    void checkDrawing(unsigned lastDrawn)
    {
        for (unsigned i = 0; i < kMaxDrawn; ++i)
        {
            float y = -1.0 + kDrawSize / 2.0 + i * kDrawSize;
            int iy  = (y + 1.0) / 2.0 * getWindowHeight();
            for (unsigned j = 0; j < 8; j += 2)
            {
                int ix = (kPointVertices[j] + 1.0) / 2.0 * getWindowWidth();
                EXPECT_PIXEL_COLOR_EQ(ix, iy, i <= lastDrawn ? GLColor::red : GLColor::blue);
            }
        }
    }

    GLuint mProgram0;
    GLuint mProgram1;
    GLuint mInstanceBuffer;

    static constexpr int kMaxDrawn   = 4;
    static constexpr float kDrawSize = 2.0 / kMaxDrawn;
    GLfloat mInstanceData[kMaxDrawn];

    // clang-format off

    // Vertices 0-5 are two triangles that form a quad filling the first "slice" of the window.
    // See above about slices.  Vertices 4-9 are the same two triangles.
    static constexpr GLfloat kQuadVertices[] = {
        -1, -1,
         1, -1,
        -1, -1 + kDrawSize,
         1, -1,
         1, -1 + kDrawSize,
        -1, -1 + kDrawSize,
         1, -1,
         1, -1,
        -1, -1 + kDrawSize,
        -1, -1,
    };

    // Points 0-3 are spread across the first "slice."
    // Points 2-4 are the same.
    static constexpr GLfloat kPointVertices[] = {
        -0.6f, -1 + kDrawSize / 2.0,
        -0.2f, -1 + kDrawSize / 2.0,
         0.2f, -1 + kDrawSize / 2.0,
         0.6f, -1 + kDrawSize / 2.0,
        -0.2f, -1 + kDrawSize / 2.0,
        -0.6f, -1 + kDrawSize / 2.0,
    };
    // clang-format on

    // Same two triangles as described above.
    static constexpr GLushort kQuadIndices[] = {2, 9, 7, 5, 6, 4};

    // Same four points as described above.
    static constexpr GLushort kPointIndices[] = {1, 5, 3, 2};
};

constexpr GLfloat InstancingTest::kQuadVertices[];
constexpr GLfloat InstancingTest::kPointVertices[];
constexpr GLushort InstancingTest::kQuadIndices[];
constexpr GLushort InstancingTest::kPointIndices[];

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

    runTest(4, 1, true /* attrib 0 instanced */, false /* quads */, true /* DrawElements */,
            false /* N/A */, false /* no buffer */);
}

// Same as AttributeZeroInstanced, but attribute zero is not instanced.
// This ensures the general instancing codepath (i.e. without rearranging the input layout) works as
// expected.
TEST_P(InstancingTestAllConfigs, AttributeZeroNotInstanced)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    runTest(4, 1, false /* attrib 1 instanced */, false /* quads */, true /* DrawElements */,
            false /* N/A */, false /* no buffer */);
}

// Tests that the "first" parameter to glDrawArraysInstancedANGLE is only an offset into
// the non-instanced vertex attributes.
TEST_P(InstancingTestNo9_3, DrawArraysWithOffset)
{
    ANGLE_SKIP_TEST_IF(!extensionEnabled("GL_ANGLE_instanced_arrays"));

    runTest(4, 1, false /* attribute 1 instanced */, false /* quads */, false /* DrawArrays */,
            true /* offset>0 */, true /* buffer */);
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

    runTest(4, 1, false /* attrib 1 instanced */, true /* points */, false /* DrawArrays */,
            false /* offset=0 */, true /* buffer */);
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

    runTest(4, 1, false /* attrib 1 instanced */, true /* points */, true /* DrawElements */,
            false /* N/A */, true /* buffer */);
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

    glUseProgram(mProgram0);

    // Get the attribute locations
    GLint positionLoc    = glGetAttribLocation(mProgram0, "a_position");
    GLint instancePosLoc = glGetAttribLocation(mProgram0, "a_instance");
    ASSERT_NE(-1, positionLoc);
    ASSERT_NE(-1, instancePosLoc);
    ASSERT_GL_NO_ERROR();

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLBuffer quadBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);

    const unsigned numInstance = 4;
    const unsigned divisor     = 1;
    const unsigned lastDrawn   = (numInstance - 1) / divisor;
    ASSERT(lastDrawn < kMaxDrawn);

    // Set the formats by VertexAttribFormat
    glVertexAttribFormat(positionLoc, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexAttribFormat(instancePosLoc, 1, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(instancePosLoc);

    const GLint positionBinding = instancePosLoc;
    const GLint instanceBinding = positionLoc;

    // Load the vertex position into the binding indexed positionBinding (== instancePosLoc)
    // Load the instance position into the binding indexed instanceBinding (== positionLoc)
    glBindVertexBuffer(positionBinding, quadBuffer, 0, 2 * sizeof(kQuadVertices[0]));
    glBindVertexBuffer(instanceBinding, mInstanceBuffer, 0, sizeof(mInstanceData[0]));

    // The attribute indexed positionLoc is using the binding indexed positionBinding
    // The attribute indexed instancePosLoc is using the binding indexed instanceBinding
    glVertexAttribBinding(positionLoc, positionBinding);
    glVertexAttribBinding(instancePosLoc, instanceBinding);

    // Enable instancing on the binding indexed instanceBinding
    glVertexBindingDivisor(instanceBinding, divisor);

    // Do the first instanced draw
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElementsInstanced(GL_TRIANGLES, ArraySize(kQuadIndices), GL_UNSIGNED_SHORT, kQuadIndices,
                            numInstance);
    checkDrawing(lastDrawn);

    // Disable instancing.
    glVertexBindingDivisor(instanceBinding, 0);

    // Load the vertex position into the binding indexed positionLoc.
    // Load the instance position into the binding indexed instancePosLoc.
    glBindVertexBuffer(positionLoc, quadBuffer, 0, 2 * sizeof(kQuadVertices[0]));
    glBindVertexBuffer(instancePosLoc, mInstanceBuffer, 0, sizeof(mInstanceData[0]));

    // The attribute indexed positionLoc is using the binding indexed positionLoc.
    glVertexAttribBinding(positionLoc, positionLoc);

    // Call VertexAttribDivisor to both enable instancing on instancePosLoc and set the attribute
    // indexed instancePosLoc using the binding indexed instancePosLoc.
    glVertexAttribDivisor(instancePosLoc, divisor);

    // Do the second instanced draw
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawElementsInstanced(GL_TRIANGLES, ArraySize(kQuadIndices), GL_UNSIGNED_SHORT, kQuadIndices,
                            numInstance);
    checkDrawing(lastDrawn);

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
