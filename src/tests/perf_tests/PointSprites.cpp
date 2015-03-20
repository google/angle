//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// PointSpritesBenchmark:
//   Performance test for ANGLE point sprites.
//

#include <cassert>
#include <sstream>
#include <iostream>

#include "ANGLEPerfTest.h"
#include "shader_utils.h"
#include "random_utils.h"

namespace
{

struct PointSpritesParams final : public PerfTestParams
{
    std::string suffix() const override;

    unsigned int count;
    float size;
    unsigned int numVaryings;

    // static parameters
    unsigned int iterations;
};

class PointSpritesBenchmark : public ANGLEPerfTest,
                              public ::testing::WithParamInterface<PointSpritesParams>
{
  public:
    PointSpritesBenchmark();

    bool initializeBenchmark() override;
    void destroyBenchmark() override;
    void beginDrawBenchmark() override;
    void drawBenchmark() override;

  private:
    DISALLOW_COPY_AND_ASSIGN(PointSpritesBenchmark);

    GLuint mProgram;
    GLuint mBuffer;
};

std::string PointSpritesParams::suffix() const
{
    std::stringstream strstr;

    strstr << PerfTestParams::suffix()
           << "_" << count << "_" << size << "px"
           << "_" << numVaryings << "vars";

    return strstr.str();
}

PointSpritesBenchmark::PointSpritesBenchmark()
    : ANGLEPerfTest("PointSprites", GetParam())
{
}

bool PointSpritesBenchmark::initializeBenchmark()
{
    const auto &params = GetParam();

    mDrawIterations = params.iterations;
    assert(params.iterations > 0);

    std::stringstream vstrstr;

    // Verify "numVaryings" is within MAX_VARYINGS limit
    GLint maxVaryings;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    if (params.numVaryings > static_cast<unsigned int>(maxVaryings))
    {
        std::cerr << "Varying count (" << params.numVaryings << ")"
                  << " exceeds maximum varyings: " << maxVaryings << std::endl;
        return false;
    }

    vstrstr << "attribute vec2 vPosition;\n"
               "uniform float uPointSize;\n";

    for (unsigned int varCount = 0; varCount < params.numVaryings; varCount++)
    {
        vstrstr << "varying vec4 v" << varCount << ";\n";
    }

    vstrstr << "void main()\n"
               "{\n";

    for (unsigned int varCount = 0; varCount < params.numVaryings; varCount++)
    {
        vstrstr << "    v" << varCount << " = vec4(1.0);\n";
    }

    vstrstr << "    gl_Position = vec4(vPosition, 0, 1.0);\n"
               "    gl_PointSize = uPointSize;\n"
               "}";

    std::stringstream fstrstr;

    fstrstr << "precision mediump float;\n";

    for (unsigned int varCount = 0; varCount < params.numVaryings; varCount++)
    {
        fstrstr << "varying vec4 v" << varCount << ";\n";
    }

    fstrstr << "void main()\n"
               "{\n"
               "    vec4 colorOut = vec4(1.0, 0.0, 0.0, 1.0);\n";

    for (unsigned int varCount = 0; varCount < params.numVaryings; varCount++)
    {
        fstrstr << "    colorOut.r += v" << varCount << ".r;\n";
    }

    fstrstr << "    gl_FragColor = colorOut;\n"
               "}\n";

    mProgram = CompileProgram(vstrstr.str(), fstrstr.str());
    if (!mProgram)
    {
        return false;
    }

    // Use the program object
    glUseProgram(mProgram);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    std::vector<float> vertexPositions(params.count * 2);
    for (size_t pointIndex = 0; pointIndex < vertexPositions.size(); ++pointIndex)
    {
        vertexPositions[pointIndex] = RandomBetween(-1.0f, 1.0f);
    }

    glGenBuffers(1, &mBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertexPositions.size() * sizeof(float), &vertexPositions[0], GL_STATIC_DRAW);

    int positionLocation = glGetAttribLocation(mProgram, "vPosition");
    if (positionLocation == -1)
    {
        return false;
    }

    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(positionLocation);

    // Set the viewport
    glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());

    int pointSizeLocation = glGetUniformLocation(mProgram, "uPointSize");
    if (pointSizeLocation == -1)
    {
        return false;
    }

    glUniform1f(pointSizeLocation, params.size);

    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR)
    {
        return false;
    }

    return true;
}

void PointSpritesBenchmark::destroyBenchmark()
{
    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mBuffer);
}

void PointSpritesBenchmark::beginDrawBenchmark()
{
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);
}

void PointSpritesBenchmark::drawBenchmark()
{
    const auto &params = GetParam();

    for (unsigned int it = 0; it < params.iterations; it++)
    {
        //TODO(jmadill): Indexed point rendering. ANGLE is bad at this.
        glDrawArrays(GL_POINTS, 0, params.count);
    }
}

PointSpritesParams D3D11Params()
{
    PointSpritesParams params;

    params.glesMajorVersion = 2;
    params.widowWidth = 1280;
    params.windowHeight = 720;
    params.requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
    params.deviceType = EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE;

    params.iterations = 10;
    params.count = 10;
    params.size = 3.0f;
    params.numVaryings = 3;

    return params;
}

PointSpritesParams D3D9Params()
{
    PointSpritesParams params;

    params.glesMajorVersion = 2;
    params.widowWidth = 1280;
    params.windowHeight = 720;
    params.requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE;
    params.deviceType = EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE;

    params.iterations = 10;
    params.count = 10;
    params.size = 3.0f;
    params.numVaryings = 3;

    return params;
}

} // namespace

TEST_P(PointSpritesBenchmark, Run)
{
    run();
}

INSTANTIATE_TEST_CASE_P(Render,
                        PointSpritesBenchmark,
                        ::testing::Values(D3D11Params(), D3D9Params()));
