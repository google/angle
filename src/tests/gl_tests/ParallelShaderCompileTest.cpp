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

        bool compile()
        {
            mVertexShader =
                CompileShader(GL_VERTEX_SHADER, insertRandomString(essl1_shaders::vs::Simple()));
            mFragmentShader = CompileShader(GL_FRAGMENT_SHADER,
                                            insertRandomString(essl1_shaders::fs::UniformColor()));
            return (mVertexShader != 0 && mFragmentShader != 0);
        }

        bool isCompileCompleted()
        {
            GLint status;
            glGetShaderiv(mVertexShader, GL_COMPLETION_STATUS_KHR, &status);
            if (status == GL_TRUE)
            {
                glGetShaderiv(mFragmentShader, GL_COMPLETION_STATUS_KHR, &status);
                return (status == GL_TRUE);
            }
            return false;
        }

        bool link()
        {
            mProgram = 0;
            if (checkShader(mVertexShader) && checkShader(mFragmentShader))
            {
                mProgram = glCreateProgram();
                glAttachShader(mProgram, mVertexShader);
                glAttachShader(mProgram, mFragmentShader);
                glLinkProgram(mProgram);
            }
            glDeleteShader(mVertexShader);
            glDeleteShader(mFragmentShader);
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

        GLuint CompileShader(GLenum type, const std::string &source)
        {
            GLuint shader = glCreateShader(type);

            const char *sourceArray[1] = {source.c_str()};
            glShaderSource(shader, 1, sourceArray, nullptr);
            glCompileShader(shader);
            return shader;
        }

        bool checkShader(GLuint shader)
        {
            GLint compileResult;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

            if (compileResult == 0)
            {
                GLint infoLogLength;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

                // Info log length includes the null terminator, so 1 means that the info log is an
                // empty string.
                if (infoLogLength > 1)
                {
                    std::vector<GLchar> infoLog(infoLogLength);
                    glGetShaderInfoLog(shader, static_cast<GLsizei>(infoLog.size()), nullptr,
                                       &infoLog[0]);
                    std::cerr << "shader compilation failed: " << &infoLog[0];
                }
                else
                {
                    std::cerr << "shader compilation failed. <Empty log message>";
                }
                std::cerr << std::endl;
            }
            return (compileResult == GL_TRUE);
        }

        GLColor mColor;
        GLuint mVertexShader;
        GLuint mFragmentShader;
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

    std::vector<std::unique_ptr<ClearColorWithDraw>> compileTasks;
    constexpr int kTaskCount = 32;
    for (int i = 0; i < kTaskCount; ++i)
    {
        std::unique_ptr<ClearColorWithDraw> task(new ClearColorWithDraw(i * 255 / kTaskCount));
        bool isCompiling = task->compile();
        ASSERT_TRUE(isCompiling);
        compileTasks.push_back(std::move(task));
    }

    constexpr unsigned int kPollInterval = 100;

    std::vector<std::unique_ptr<ClearColorWithDraw>> linkTasks;
    while (!compileTasks.empty())
    {
        for (unsigned int i = 0; i < compileTasks.size();)
        {
            auto &task = compileTasks[i];

            if (task->isCompileCompleted())
            {
                bool isLinking = task->link();
                ASSERT_TRUE(isLinking);
                linkTasks.push_back(std::move(task));
                compileTasks.erase(compileTasks.begin() + i);
                continue;
            }
            ++i;
        }
        Sleep(kPollInterval);
    }

    while (!linkTasks.empty())
    {
        for (unsigned int i = 0; i < linkTasks.size();)
        {
            auto &task = linkTasks[i];

            if (task->isLinkCompleted())
            {
                task->drawAndVerify(this);
                linkTasks.erase(linkTasks.begin() + i);
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
