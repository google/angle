//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ClearPerf:
//   Performance test for clearing framebuffers.
//

#include "ANGLEPerfTest.h"

#include <iostream>
#include <random>
#include <sstream>

#include "test_utils/gl_raii.h"
#include "util/shader_utils.h"

using namespace angle;

namespace
{
constexpr unsigned int kIterationsPerStep = 256;

struct ClearParams final : public RenderTestParams
{
    ClearParams()
    {
        iterationsPerStep = kIterationsPerStep;
        trackGpuTime      = true;

        fboSize     = 2048;
        textureSize = 16;
    }

    std::string story() const override;

    GLsizei fboSize;
    GLsizei textureSize;
};

std::ostream &operator<<(std::ostream &os, const ClearParams &params)
{
    os << params.backendAndStory().substr(1);
    return os;
}

std::string ClearParams::story() const
{
    std::stringstream strstr;

    strstr << RenderTestParams::story();

    return strstr.str();
}

class ClearBenchmark : public ANGLERenderTest, public ::testing::WithParamInterface<ClearParams>
{
  public:
    ClearBenchmark();

    void initializeBenchmark() override;
    void destroyBenchmark() override;
    void drawBenchmark() override;

  private:
    void initShaders();

    std::vector<GLuint> mTextures;

    GLuint mProgram;
};

ClearBenchmark::ClearBenchmark() : ANGLERenderTest("Clear", GetParam()), mProgram(0u)
{
    // Crashes on nvidia+d3d11. http://crbug.com/945415
    if (GetParam().getRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
    {
        mSkipTest = true;
    }
}

void ClearBenchmark::initializeBenchmark()
{
    initShaders();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());

    ASSERT_GL_NO_ERROR();
}

void ClearBenchmark::initShaders()
{
    constexpr char kVS[] = R"(void main()
{
    gl_Position = vec4(0, 0, 0, 1);
})";

    constexpr char kFS[] = R"(precision mediump float;
void main()
{
    gl_FragColor = vec4(0);
})";

    mProgram = CompileProgram(kVS, kFS);
    ASSERT_NE(0u, mProgram);

    glUseProgram(mProgram);

    glDisable(GL_DEPTH_TEST);

    ASSERT_GL_NO_ERROR();
}

void ClearBenchmark::destroyBenchmark()
{
    glDeleteProgram(mProgram);
}

void ClearBenchmark::drawBenchmark()
{
    const auto &params = GetParam();

    std::vector<float> textureData(params.textureSize * params.textureSize * 4, 0.5);

    GLRenderbuffer colorRbo;
    glBindRenderbuffer(GL_RENDERBUFFER, colorRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, params.fboSize, params.fboSize);

    GLRenderbuffer depthRbo;
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, params.fboSize, params.fboSize);

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);

    startGpuTimer();
    for (size_t it = 0; it < params.iterationsPerStep; ++it)
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    stopGpuTimer();

    ASSERT_GL_NO_ERROR();
}

ClearParams D3D11Params()
{
    ClearParams params;
    params.eglParameters = egl_platform::D3D11();
    return params;
}

ClearParams OpenGLOrGLESParams()
{
    ClearParams params;
    params.eglParameters = egl_platform::OPENGL_OR_GLES();
    return params;
}

ClearParams VulkanParams()
{
    ClearParams params;
    params.eglParameters = egl_platform::VULKAN();
    return params;
}

}  // anonymous namespace

TEST_P(ClearBenchmark, Run)
{
    run();
}

ANGLE_INSTANTIATE_TEST(ClearBenchmark, D3D11Params(), OpenGLOrGLESParams(), VulkanParams());
