//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DrawCallPerf:
//   Performance tests for ANGLE draw call overhead.
//

#include "ANGLEPerfTest.h"
#include "DrawCallPerfParams.h"
#include "test_utils/draw_call_perf_utils.h"

namespace
{

struct DrawArraysPerfParams : public DrawCallPerfParams
{
    DrawArraysPerfParams(const DrawCallPerfParams &base) : DrawCallPerfParams(base) {}

    std::string suffix() const override;

    bool changeVertexBuffer = false;
};

std::string DrawArraysPerfParams::suffix() const
{
    std::stringstream strstr;

    strstr << DrawCallPerfParams::suffix();

    if (changeVertexBuffer)
    {
        strstr << "_vbo_change";
    }

    return strstr.str();
}

std::ostream &operator<<(std::ostream &os, const DrawArraysPerfParams &params)
{
    os << params.suffix().substr(1);
    return os;
}

class DrawCallPerfBenchmark : public ANGLERenderTest,
                              public ::testing::WithParamInterface<DrawArraysPerfParams>
{
  public:
    DrawCallPerfBenchmark();

    void initializeBenchmark() override;
    void destroyBenchmark() override;
    void drawBenchmark() override;

  private:
    GLuint mProgram = 0;
    GLuint mBuffer1 = 0;
    GLuint mBuffer2 = 0;
    GLuint mFBO     = 0;
    GLuint mTexture = 0;
    int mNumTris    = GetParam().numTris;
};

DrawCallPerfBenchmark::DrawCallPerfBenchmark() : ANGLERenderTest("DrawCallPerf", GetParam())
{
    mRunTimeSeconds = GetParam().runTimeSeconds;
}

void DrawCallPerfBenchmark::initializeBenchmark()
{
    const auto &params = GetParam();

    ASSERT_LT(0u, params.iterations);

    mProgram = SetupSimpleDrawProgram();
    ASSERT_NE(0u, mProgram);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    mBuffer1 = Create2DTriangleBuffer(mNumTris, GL_STATIC_DRAW);
    mBuffer2 = Create2DTriangleBuffer(mNumTris, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Set the viewport
    glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());

    if (params.useFBO)
    {
        CreateColorFBO(getWindow()->getWidth(), getWindow()->getHeight(), &mTexture, &mFBO);
    }

    ASSERT_GL_NO_ERROR();
}

void DrawCallPerfBenchmark::destroyBenchmark()
{
    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mBuffer1);
    glDeleteBuffers(1, &mBuffer2);
    glDeleteTextures(1, &mTexture);
    glDeleteFramebuffers(1, &mFBO);
}

void ClearThenDraw(unsigned int iterations, GLsizei numElements)
{
    glClear(GL_COLOR_BUFFER_BIT);

    for (unsigned int it = 0; it < iterations; it++)
    {
        glDrawArrays(GL_TRIANGLES, 0, numElements);
    }
}

void JustDraw(unsigned int iterations, GLsizei numElements)
{
    for (unsigned int it = 0; it < iterations; it++)
    {
        glDrawArrays(GL_TRIANGLES, 0, numElements);
    }
}

void ChangeVerticesThenDraw(unsigned int iterations,
                            GLsizei numElements,
                            GLuint buffer1,
                            GLuint buffer2)
{
    for (unsigned int it = 0; it < iterations; it++)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffer1);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, numElements);

        glBindBuffer(GL_ARRAY_BUFFER, buffer2);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, numElements);
    }
}

void DrawCallPerfBenchmark::drawBenchmark()
{
    // This workaround fixes a huge queue of graphics commands accumulating on the GL
    // back-end. The GL back-end doesn't have a proper NULL device at the moment.
    // TODO(jmadill): Remove this when/if we ever get a proper OpenGL NULL device.
    const auto &eglParams = GetParam().eglParameters;
    const auto &params    = GetParam();
    GLsizei numElements   = static_cast<GLsizei>(3 * mNumTris);

    if (params.changeVertexBuffer)
    {
        ChangeVerticesThenDraw(params.iterations, numElements, mBuffer1, mBuffer2);
    }
    else if (eglParams.deviceType != EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE ||
             (eglParams.renderer != EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE &&
              eglParams.renderer != EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE))
    {
        ClearThenDraw(params.iterations, numElements);
    }
    else
    {
        JustDraw(params.iterations, numElements);
    }

    ASSERT_GL_NO_ERROR();
}

TEST_P(DrawCallPerfBenchmark, Run)
{
    run();
}

DrawArraysPerfParams DrawArrays(const DrawCallPerfParams &base, bool withBufferChange)
{
    DrawArraysPerfParams params(base);
    params.changeVertexBuffer = withBufferChange;
    return params;
}

ANGLE_INSTANTIATE_TEST(DrawCallPerfBenchmark,
                       DrawArrays(DrawCallPerfD3D9Params(false, false), false),
                       DrawArrays(DrawCallPerfD3D9Params(true, false), false),
                       DrawArrays(DrawCallPerfD3D11Params(false, false), false),
                       DrawArrays(DrawCallPerfD3D11Params(true, false), false),
                       DrawArrays(DrawCallPerfD3D11Params(true, true), false),
                       DrawArrays(DrawCallPerfD3D11Params(true, false), true),
                       DrawArrays(DrawCallPerfOpenGLOrGLESParams(false, false), false),
                       DrawArrays(DrawCallPerfOpenGLOrGLESParams(true, false), false),
                       DrawArrays(DrawCallPerfOpenGLOrGLESParams(true, true), false),
                       DrawArrays(DrawCallPerfOpenGLOrGLESParams(true, false), true),
                       DrawArrays(DrawCallPerfValidationOnly(), false),
                       DrawArrays(DrawCallPerfVulkanParams(false), false),
                       DrawArrays(DrawCallPerfVulkanParams(false), true));

} // namespace
