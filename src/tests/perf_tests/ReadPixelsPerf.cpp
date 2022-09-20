//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ReadPixelsBenchmark:
//   Performance test for ANGLE ReadPixels.
//
//
#include "ANGLEPerfTest.h"

#include <iostream>
#include <sstream>

#include "GLES/gl.h"
#include "test_utils/gl_raii.h"
#include "util/gles_loader_autogen.h"
#include "util/random_utils.h"
#include "util/shader_utils.h"

using namespace angle;

namespace
{
constexpr unsigned int kIterationsPerStep = 100;

struct ReadPixelsParams final : public RenderTestParams
{
    ReadPixelsParams()
    {
        iterationsPerStep = kIterationsPerStep;

        // Common default params
        majorVersion   = 2;
        minorVersion   = 0;
        windowWidth    = 1280;
        windowHeight   = 720;
        drawBeforeRead = false;
    }

    std::string story() const override;

    bool drawBeforeRead;
};

std::ostream &operator<<(std::ostream &os, const ReadPixelsParams &params)
{
    os << params.backendAndStory().substr(1);
    return os;
}

class ReadPixelsBenchmark : public ANGLERenderTest,
                            public ::testing::WithParamInterface<ReadPixelsParams>
{
  public:
    ReadPixelsBenchmark();

    void initializeBenchmark() override;
    void destroyBenchmark() override;
    void drawBenchmark() override;

  private:
    struct GLColor
    {
        GLubyte rgba[4];
    };

    GLProgram mProgram;
    GLBuffer mBuffer;
    GLTexture mTexture;
    GLFramebuffer mFramebuffer;
    std::vector<GLColor> readResult;
};

std::string ReadPixelsParams::story() const
{
    std::stringstream strstr;

    strstr << RenderTestParams::story() << (drawBeforeRead ? "_withDraw" : "");

    return strstr.str();
}

ReadPixelsBenchmark::ReadPixelsBenchmark() : ANGLERenderTest("ReadPixels", GetParam())
{
    disableTestHarnessSwap();
}

void ReadPixelsBenchmark::initializeBenchmark()
{
    const auto &params = GetParam();
    std::cout << glGetString(GL_VENDOR) << "\n";
    std::cout << glGetString(GL_RENDERER) << "\n";

    constexpr const char *kVS = R"(
      attribute vec4 position;
      void main() {
        gl_Position = position;
      }
    )";

    constexpr const char *kFS = R"(
      precision mediump float;
      void main() {
        gl_FragColor = vec4(0, 1, 0, 1);
      }
    )";
    mProgram.makeRaster(kVS, kFS);

    constexpr static float positions[] = {
        -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
    };

    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

    GLint positionLocation = glGetAttribLocation(mProgram, "position");
    ASSERT_NE(-1, positionLocation);

    glVertexAttribPointer(positionLocation, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(positionLocation);

    glViewport(0, 0, params.windowWidth, params.windowHeight);

    readResult.resize(params.windowWidth * params.windowHeight);

    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, params.windowWidth, params.windowHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);

    ASSERT_GL_NO_ERROR();
}

void ReadPixelsBenchmark::destroyBenchmark() {}

void ReadPixelsBenchmark::drawBenchmark()
{
    const auto &params = GetParam();

    for (unsigned int it = 0; it < params.iterationsPerStep; it++)
    {
        if (params.drawBeforeRead)
        {
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glReadPixels(0, 0, params.windowWidth, params.windowHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                     readResult.data());
    }

    ASSERT_GL_NO_ERROR();
}

ReadPixelsParams D3D11Params()
{
    ReadPixelsParams params;
    params.eglParameters = egl_platform::D3D11();
    return params;
}

ReadPixelsParams D3D11WithDrawParams()
{
    ReadPixelsParams params;
    params.eglParameters  = egl_platform::D3D11();
    params.drawBeforeRead = true;
    return params;
}

ReadPixelsParams MetalParams()
{
    ReadPixelsParams params;
    params.eglParameters = egl_platform::METAL();
    return params;
}

ReadPixelsParams MetalWithDrawParams()
{
    ReadPixelsParams params;
    params.eglParameters  = egl_platform::METAL();
    params.drawBeforeRead = true;
    return params;
}

ReadPixelsParams OpenGLOrGLESParams()
{
    ReadPixelsParams params;
    params.eglParameters = egl_platform::OPENGL_OR_GLES();
    return params;
}

ReadPixelsParams OpenGLOrGLESWithDrawParams()
{
    ReadPixelsParams params;
    params.eglParameters  = egl_platform::OPENGL_OR_GLES();
    params.drawBeforeRead = true;
    return params;
}

ReadPixelsParams VulkanParams()
{
    ReadPixelsParams params;
    params.eglParameters = egl_platform::VULKAN();
    return params;
}

ReadPixelsParams VulkanWithDrawParams()
{
    ReadPixelsParams params;
    params.eglParameters  = egl_platform::VULKAN();
    params.drawBeforeRead = true;
    return params;
}

}  // namespace

// Tests the performance of ReadPixels.
TEST_P(ReadPixelsBenchmark, Run)
{
    run();
}

ANGLE_INSTANTIATE_TEST(ReadPixelsBenchmark,
                       D3D11Params(),
                       D3D11WithDrawParams(),
                       MetalParams(),
                       MetalWithDrawParams(),
                       OpenGLOrGLESParams(),
                       OpenGLOrGLESWithDrawParams(),
                       VulkanParams(),
                       VulkanWithDrawParams());
