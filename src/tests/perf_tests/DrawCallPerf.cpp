//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DrawCallPerf:
//   Performance tests for ANGLE draw call overhead.
//

#include <cassert>
#include <sstream>

#include "ANGLEPerfTest.h"
#include "shader_utils.h"

namespace
{

struct DrawCallPerfParams final : public RenderTestParams
{
    std::string suffix() const override
    {
        std::stringstream strstr;

        strstr << RenderTestParams::suffix();

        return strstr.str();
    }

    unsigned int iterations;
};

class DrawCallPerfBenchmark : public ANGLERenderTest,
                              public ::testing::WithParamInterface<DrawCallPerfParams>
{
  public:
    DrawCallPerfBenchmark();

    bool initializeBenchmark() override;
    void destroyBenchmark() override;
    void beginDrawBenchmark() override;
    void drawBenchmark() override;

  private:
    GLuint mProgram;
    GLuint mBuffer;
    int mNumTris;
};

DrawCallPerfBenchmark::DrawCallPerfBenchmark()
    : ANGLERenderTest("DrawCallPerf", GetParam()),
      mProgram(0),
      mBuffer(0),
      mNumTris(0)
{
}

bool DrawCallPerfBenchmark::initializeBenchmark()
{
    const auto &params = GetParam();

    assert(params.iterations > 0);
    mDrawIterations = params.iterations;

    const std::string vs = SHADER_SOURCE
    (
        attribute vec2 vPosition;
        uniform float uScale;
        uniform float uOffset;
        void main()
        {
            gl_Position = vec4(vPosition * vec2(uScale) - vec2(uOffset), 0, 1);
        }
    );

    const std::string fs = SHADER_SOURCE
    (
        precision mediump float;
        void main()
        {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    mProgram = CompileProgram(vs, fs);
    if (!mProgram)
    {
        return false;
    }

    // Use the program object
    glUseProgram(mProgram);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);


    std::vector<float> floatData(6);
    floatData[0] = 1;
    floatData[1] = 2;
    floatData[2] = 0;
    floatData[3] = 0;
    floatData[4] = 2;
    floatData[5] = 0;

    glGenBuffers(1, &mBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glBufferData(GL_ARRAY_BUFFER, floatData.size() * sizeof(float), &floatData[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Set the viewport
    glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());

    GLfloat scale = 0.5f;
    GLfloat offset = 0.5f;

    glUniform1f(glGetUniformLocation(mProgram, "uScale"), scale);
    glUniform1f(glGetUniformLocation(mProgram, "uOffset"), offset);

    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR)
    {
        return false;
    }

    return true;
}

void DrawCallPerfBenchmark::destroyBenchmark()
{
    const auto &params = GetParam();

    // print static parameters
    printResult("iterations", static_cast<size_t>(params.iterations), "updates", false);

    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mBuffer);
}

void DrawCallPerfBenchmark::beginDrawBenchmark()
{
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);
}

void DrawCallPerfBenchmark::drawBenchmark()
{
    const auto &params = GetParam();

    for (unsigned int it = 0; it < params.iterations; it++)
    {
        glDrawArrays(GL_TRIANGLES, 0, 3 * mNumTris);
    }
}

DrawCallPerfParams DrawCallPerfD3D11Params()
{
    DrawCallPerfParams params;
    params.glesMajorVersion = 2;
    params.widowWidth = 1280;
    params.windowHeight = 720;
    params.requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
    params.deviceType = EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE;
    params.iterations = 50;
    return params;
}

DrawCallPerfParams DrawCallPerfD3D9Params()
{
    DrawCallPerfParams params;
    params.glesMajorVersion = 2;
    params.widowWidth = 1280;
    params.windowHeight = 720;
    params.requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE;
    params.deviceType = EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE;
    params.iterations = 50;
    return params;
}

TEST_P(DrawCallPerfBenchmark, Run)
{
    run();
}

INSTANTIATE_TEST_CASE_P(DrawCallPerf,
                        DrawCallPerfBenchmark,
                        ::testing::Values(DrawCallPerfD3D11Params(), DrawCallPerfD3D9Params()));

} // namespace
