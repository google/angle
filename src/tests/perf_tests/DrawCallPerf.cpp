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
enum class StateChange
{
    NoChange,
    VertexBuffer,
    Texture,
};

struct DrawArraysPerfParams : public DrawCallPerfParams
{
    DrawArraysPerfParams(const DrawCallPerfParams &base) : DrawCallPerfParams(base) {}

    std::string suffix() const override;

    StateChange stateChange = StateChange::NoChange;
};

std::string DrawArraysPerfParams::suffix() const
{
    std::stringstream strstr;

    strstr << DrawCallPerfParams::suffix();

    switch (stateChange)
    {
        case StateChange::VertexBuffer:
            strstr << "_vbo_change";
            break;
        case StateChange::Texture:
            strstr << "_tex_change";
            break;
        default:
            break;
            ;
    }

    return strstr.str();
}

std::ostream &operator<<(std::ostream &os, const DrawArraysPerfParams &params)
{
    os << params.suffix().substr(1);
    return os;
}

GLuint CreateSimpleTexture2D()
{
    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate a texture object
    GLuint texture;
    glGenTextures(1, &texture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, texture);

    // Load the texture: 2x2 Image, 3 bytes per pixel (R, G, B)
    constexpr size_t width             = 2;
    constexpr size_t height            = 2;
    GLubyte pixels[width * height * 3] = {
        255, 0,   0,    // Red
        0,   255, 0,    // Green
        0,   0,   255,  // Blue
        255, 255, 0,    // Yellow
    };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    // Set the filtering mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture;
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
    GLuint mFBOTexture = 0;
    GLuint mTexture1   = 0;
    GLuint mTexture2   = 0;
    int mNumTris    = GetParam().numTris;
};

DrawCallPerfBenchmark::DrawCallPerfBenchmark() : ANGLERenderTest("DrawCallPerf", GetParam())
{
    mRunTimeSeconds = GetParam().runTimeSeconds;
}

void DrawCallPerfBenchmark::initializeBenchmark()
{
    const auto &params = GetParam();

    if (params.stateChange == StateChange::Texture)
    {
        mProgram = SetupSimpleTextureProgram();
    }
    else
    {
        mProgram = SetupSimpleDrawProgram();
    }

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
        CreateColorFBO(getWindow()->getWidth(), getWindow()->getHeight(), &mFBOTexture, &mFBO);
    }

    mTexture1 = CreateSimpleTexture2D();
    mTexture2 = CreateSimpleTexture2D();

    ASSERT_GL_NO_ERROR();
}

void DrawCallPerfBenchmark::destroyBenchmark()
{
    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mBuffer1);
    glDeleteBuffers(1, &mBuffer2);
    glDeleteTextures(1, &mFBOTexture);
    glDeleteTextures(1, &mTexture1);
    glDeleteTextures(1, &mTexture2);
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

void ChangeTextureThenDraw(unsigned int iterations,
                           GLsizei numElements,
                           GLuint texture1,
                           GLuint texture2)
{
    for (unsigned int it = 0; it < iterations; it++)
    {
        glBindTexture(GL_TEXTURE_2D, texture1);
        glDrawArrays(GL_TRIANGLES, 0, numElements);

        glBindTexture(GL_TEXTURE_2D, texture2);
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

    switch (params.stateChange)
    {
        case StateChange::VertexBuffer:
            ChangeVerticesThenDraw(params.iterationsPerStep, numElements, mBuffer1, mBuffer2);
            break;
        case StateChange::Texture:
            ChangeTextureThenDraw(params.iterationsPerStep, numElements, mTexture1, mTexture2);
            break;
        case StateChange::NoChange:
            if (eglParams.deviceType != EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE ||
                (eglParams.renderer != EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE &&
                 eglParams.renderer != EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE))
            {
                ClearThenDraw(params.iterationsPerStep, numElements);
            }
            else
            {
                JustDraw(params.iterationsPerStep, numElements);
            }
            break;
    }

    ASSERT_GL_NO_ERROR();
}

TEST_P(DrawCallPerfBenchmark, Run)
{
    run();
}

DrawArraysPerfParams DrawArrays(const DrawCallPerfParams &base, StateChange stateChange)
{
    DrawArraysPerfParams params(base);
    params.stateChange = stateChange;
    return params;
}

// TODO(jmadill): Fix tex change test on Vulkan. http://anglebug.com/2938

ANGLE_INSTANTIATE_TEST(
    DrawCallPerfBenchmark,
    DrawArrays(DrawCallPerfD3D9Params(false, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfD3D9Params(true, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfD3D11Params(false, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfD3D11Params(true, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfD3D11Params(true, true), StateChange::NoChange),
    DrawArrays(DrawCallPerfD3D11Params(false, false), StateChange::VertexBuffer),
    DrawArrays(DrawCallPerfD3D11Params(true, false), StateChange::VertexBuffer),
    DrawArrays(DrawCallPerfD3D11Params(false, false), StateChange::Texture),
    DrawArrays(DrawCallPerfD3D11Params(true, false), StateChange::Texture),
    DrawArrays(DrawCallPerfOpenGLOrGLESParams(false, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfOpenGLOrGLESParams(true, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfOpenGLOrGLESParams(true, true), StateChange::NoChange),
    DrawArrays(DrawCallPerfOpenGLOrGLESParams(false, false), StateChange::VertexBuffer),
    DrawArrays(DrawCallPerfOpenGLOrGLESParams(true, false), StateChange::VertexBuffer),
    DrawArrays(DrawCallPerfOpenGLOrGLESParams(false, false), StateChange::Texture),
    DrawArrays(DrawCallPerfOpenGLOrGLESParams(true, false), StateChange::Texture),
    DrawArrays(DrawCallPerfValidationOnly(), StateChange::NoChange),
    DrawArrays(DrawCallPerfVulkanParams(false, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfVulkanParams(true, false), StateChange::NoChange),
    DrawArrays(DrawCallPerfVulkanParams(false, false), StateChange::VertexBuffer),
    DrawArrays(DrawCallPerfVulkanParams(true, false), StateChange::VertexBuffer));

}  // anonymous namespace
