//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VertexArrayPerfTest:
//   Performance test for glBindVertexArray.
//

#include "ANGLEPerfTest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{
struct VertexArrayParams final : public RenderTestParams
{
    VertexArrayParams()
    {
        iterationsPerStep = 1;

        // Common default params
        majorVersion = 3;
        minorVersion = 0;
        windowWidth  = 720;
        windowHeight = 720;
    }

    std::string story() const override;
};

std::ostream &operator<<(std::ostream &os, const VertexArrayParams &params)
{
    os << params.backendAndStory().substr(1);
    return os;
}

std::string VertexArrayParams::story() const
{
    std::stringstream strstr;

    strstr << RenderTestParams::story();

    return strstr.str();
}

class VertexArrayBenchmark : public ANGLERenderTest,
                             public ::testing::WithParamInterface<VertexArrayParams>
{
  public:
    VertexArrayBenchmark();

    void initializeBenchmark() override;
    void destroyBenchmark() override;
    void drawBenchmark() override;

  private:
    GLBuffer mBuffer;
    GLuint mProgram       = 0;
    GLint mAttribLocation = 0;
    GLVertexArray vertexArrays[1000];
};

VertexArrayBenchmark::VertexArrayBenchmark() : ANGLERenderTest("VertexArrayPerf", GetParam()) {}

void VertexArrayBenchmark::initializeBenchmark()
{
    constexpr char kVS[] = R"(attribute vec4 position;
attribute float in_attrib;
varying float v_attrib;
void main()
{
    v_attrib = in_attrib;
    gl_Position = position;
})";

    constexpr char kFS[] = R"(precision mediump float;
varying float v_attrib;
void main()
{
    gl_FragColor = vec4(v_attrib, 0, 0, 1);
})";

    mProgram = CompileProgram(kVS, kFS);
    ASSERT_NE(0u, mProgram);

    mAttribLocation = glGetAttribLocation(mProgram, "in_attrib");
    ASSERT_NE(mAttribLocation, -1);
}

void VertexArrayBenchmark::destroyBenchmark()
{
    glDeleteProgram(mProgram);
}

void VertexArrayBenchmark::drawBenchmark()
{
    // Bind one VBO to 1000 VAOs.
    for (GLuint i = 0; i < 1000; i++)
    {
        glBindVertexArray(vertexArrays[i]);
        glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
        glEnableVertexAttribArray(mAttribLocation);
        glVertexAttribPointer(mAttribLocation, 1, GL_FLOAT, GL_FALSE, 4, nullptr);
        glVertexAttribDivisor(mAttribLocation, 1);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glBufferData(GL_ARRAY_BUFFER, 128, nullptr, GL_STATIC_DRAW);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexArrayParams VulkanParams()
{
    VertexArrayParams params;
    params.eglParameters = egl_platform::VULKAN();

    return params;
}

TEST_P(VertexArrayBenchmark, Run)
{
    run();
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VertexArrayBenchmark);
ANGLE_INSTANTIATE_TEST(VertexArrayBenchmark, VulkanParams());
}  // namespace
