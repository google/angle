//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ParallelShaderCompileTest.cpp : Tests of the GL_KHR_parallel_shader_compile extension.

#include "test_utils/ANGLETest.h"

#include "random_utils.h"

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

    class ClearColorWithDraw
    {
      public:
        ClearColorWithDraw(GLubyte color) : mColor(color, color, color, 255) {}

        bool compileAndLink()
        {
            mProgram =
                CompileProgramParallel(insertRandomString(essl1_shaders::vs::Simple()),
                                       insertRandomString(essl1_shaders::fs::UniformColor()));
            return (mProgram != 0);
        }

        bool isLinkCompleted()
        {
            GLint status;
            glGetProgramiv(mProgram, GL_COMPLETION_STATUS_KHR, &status);
            return (status == GL_TRUE);
        }

        void drawAndVerify(ParallelShaderCompileTest *test)
        {
            glClearColor(0, 0, 0, 0);
            glClear(GL_COLOR_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
            glUseProgram(mProgram);
            ASSERT_GL_NO_ERROR();
            GLint colorUniformLocation =
                glGetUniformLocation(mProgram, essl1_shaders::ColorUniform());
            ASSERT_NE(colorUniformLocation, -1);
            auto normalizeColor = mColor.toNormalizedVector();
            glUniform4fv(colorUniformLocation, 1, normalizeColor.data());
            test->drawQuad(mProgram, essl1_shaders::PositionAttrib(), 0.5f);
            EXPECT_PIXEL_COLOR_EQ(test->getWindowWidth() / 2, test->getWindowHeight() / 2, mColor);
            glUseProgram(0);
            glDeleteProgram(mProgram);
            ASSERT_GL_NO_ERROR();
        }

      private:
        std::string insertRandomString(const std::string &source)
        {
            RNG rng;
            std::ostringstream ostream;
            ostream << "// Random string to fool program cache: " << rng.randomInt() << "\n"
                    << source;
            return ostream.str();
        }

        GLColor mColor;
        GLuint mProgram;
    };
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

// Test to compile and link many programs in parallel.
TEST_P(ParallelShaderCompileTest, LinkAndDrawManyPrograms)
{
    ANGLE_SKIP_TEST_IF(!ensureParallelShaderCompileExtensionAvailable());

    std::vector<std::unique_ptr<ClearColorWithDraw>> tasks;
    constexpr int kTaskCount = 32;
    for (int i = 0; i < kTaskCount; ++i)
    {
        std::unique_ptr<ClearColorWithDraw> task(new ClearColorWithDraw(i * 255 / kTaskCount));
        bool isLinking = task->compileAndLink();
        ASSERT_TRUE(isLinking);
        tasks.push_back(std::move(task));
    }
    constexpr unsigned int kPollInterval = 100;
    while (!tasks.empty())
    {
        for (unsigned int i = 0; i < tasks.size();)
        {
            auto &task = tasks[i];
            if (task->isLinkCompleted())
            {
                task->drawAndVerify(this);
                tasks.erase(tasks.begin() + i);
                continue;
            }
            ++i;
        }
        Sleep(kPollInterval);
    }
}

ANGLE_INSTANTIATE_TEST(ParallelShaderCompileTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES(),
                       ES2_VULKAN());

}  // namespace
