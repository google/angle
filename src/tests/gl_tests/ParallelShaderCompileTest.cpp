//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ParallelShaderCompileTest.cpp : Tests of the GL_KHR_parallel_shader_compile extension.

#include "test_utils/ANGLETest.h"

using namespace angle;

namespace
{

class ParallelShaderCompileTest : public ANGLETest
{
  protected:
    ParallelShaderCompileTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void SetUp() override { ANGLETest::SetUp(); }

    void TearDown() override { ANGLETest::TearDown(); }

    bool ensureParallelShaderCompileExtensionAvailable()
    {
        if (extensionRequestable("GL_KHR_parallel_shader_compile"))
        {
            glRequestExtensionANGLE("GL_KHR_parallel_shader_compile");
        }

        if (!extensionEnabled("GL_KHR_parallel_shader_compile"))
        {
            return false;
        }
        return true;
    }
};

// Test basic functionality of GL_KHR_parallel_shader_compile
TEST_P(ParallelShaderCompileTest, Basic)
{
    ANGLE_SKIP_TEST_IF(!ensureParallelShaderCompileExtensionAvailable());

    GLint count = 0;
    glMaxShaderCompilerThreadsKHR(8);
    EXPECT_GL_NO_ERROR();
    glGetIntegerv(GL_MAX_SHADER_COMPILER_THREADS_KHR, &count);
    EXPECT_GL_NO_ERROR();
    EXPECT_EQ(8, count);
}

ANGLE_INSTANTIATE_TEST(ParallelShaderCompileTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES(),
                       ES2_VULKAN());

}  // namespace
