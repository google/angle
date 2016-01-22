//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TimerQueriesTest.cpp
//   Various tests for EXT_disjoint_timer_query functionality and validation
//

#include "system_utils.h"
#include "test_utils/ANGLETest.h"
#include "random_utils.h"

using namespace angle;

class TimerQueriesTest : public ANGLETest
{
  protected:
    TimerQueriesTest() : mProgram(0), mProgramCostly(0)
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const std::string passthroughVS =
            "attribute highp vec4 position; void main(void)\n"
            "{\n"
            "    gl_Position = position;\n"
            "}\n";

        const std::string passthroughPS =
            "precision highp float; void main(void)\n"
            "{\n"
            "    gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
            "}\n";

        const std::string costlyVS =
            "attribute highp vec4 position; varying highp vec4 testPos; void main(void)\n"
            "{\n"
            "    testPos     = position;\n"
            "    gl_Position = position;\n"
            "}\n";

        const std::string costlyPS =
            "precision highp float; varying highp vec4 testPos; void main(void)\n"
            "{\n"
            "    vec4 test = testPos;\n"
            "    for (int i = 0; i < 500; i++)\n"
            "    {\n"
            "        test = sqrt(test);\n"
            "    }\n"
            "    gl_FragColor = test;\n"
            "}\n";

        mProgram = CompileProgram(passthroughVS, passthroughPS);
        ASSERT_NE(0u, mProgram) << "shader compilation failed.";

        mProgramCostly = CompileProgram(costlyVS, costlyPS);
        ASSERT_NE(0u, mProgramCostly) << "shader compilation failed.";
    }

    virtual void TearDown()
    {
        glDeleteProgram(mProgram);
        glDeleteProgram(mProgramCostly);
        ANGLETest::TearDown();
    }

    GLuint mProgram;
    GLuint mProgramCostly;
};

// Tests the time elapsed query
TEST_P(TimerQueriesTest, TimeElapsed)
{
    if (!extensionEnabled("GL_EXT_disjoint_timer_query"))
    {
        std::cout << "Test skipped because GL_EXT_disjoint_timer_query is not available."
                  << std::endl;
        return;
    }

    GLint queryTimeElapsedBits = 0;
    glGetQueryivEXT(GL_TIME_ELAPSED_EXT, GL_QUERY_COUNTER_BITS_EXT, &queryTimeElapsedBits);
    ASSERT_GL_NO_ERROR();

    std::cout << "Time elapsed counter bits: " << queryTimeElapsedBits << std::endl;

    // Skip test if the number of bits is 0
    if (queryTimeElapsedBits == 0)
    {
        std::cout << "Test skipped because of 0 counter bits" << std::endl;
        return;
    }

    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    GLuint query1 = 0;
    GLuint query2 = 0;
    glGenQueriesEXT(1, &query1);
    glGenQueriesEXT(1, &query2);

    // Test time elapsed for a single quad
    glBeginQueryEXT(GL_TIME_ELAPSED_EXT, query1);
    drawQuad(mProgram, "position", 0.8f);
    glEndQueryEXT(GL_TIME_ELAPSED_EXT);
    ASSERT_GL_NO_ERROR();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Test time elapsed for costly quad
    glBeginQueryEXT(GL_TIME_ELAPSED_EXT, query2);
    drawQuad(mProgramCostly, "position", 0.8f);
    glEndQueryEXT(GL_TIME_ELAPSED_EXT);
    ASSERT_GL_NO_ERROR();

    swapBuffers();

    int timeout  = 10000;
    GLuint ready = GL_FALSE;
    while (ready == GL_FALSE && timeout > 0)
    {
        angle::Sleep(0);
        glGetQueryObjectuivEXT(query1, GL_QUERY_RESULT_AVAILABLE_EXT, &ready);
        timeout--;
    }
    ready = GL_FALSE;
    while (ready == GL_FALSE && timeout > 0)
    {
        angle::Sleep(0);
        glGetQueryObjectuivEXT(query2, GL_QUERY_RESULT_AVAILABLE_EXT, &ready);
        timeout--;
    }
    ASSERT_LT(0, timeout) << "Query result available timed out" << std::endl;

    GLuint64 result1 = 0;
    GLuint64 result2 = 0;
    glGetQueryObjectui64vEXT(query1, GL_QUERY_RESULT_EXT, &result1);
    glGetQueryObjectui64vEXT(query2, GL_QUERY_RESULT_EXT, &result2);
    ASSERT_GL_NO_ERROR();

    glDeleteQueriesEXT(1, &query1);
    glDeleteQueriesEXT(1, &query2);
    ASSERT_GL_NO_ERROR();

    std::cout << "Elapsed time: " << result1 << " cheap quad" << std::endl;
    std::cout << "Elapsed time: " << result2 << " costly quad" << std::endl;

    // The time elapsed should be nonzero
    EXPECT_LT(0ul, result1);
    EXPECT_LT(0ul, result2);

    // The costly quad should take longer than the cheap quad
    EXPECT_LT(result1, result2);
}

// Tests validation of query functions with respect to elapsed time query
TEST_P(TimerQueriesTest, TimeElapsedValidationTest)
{
    if (!extensionEnabled("GL_EXT_disjoint_timer_query"))
    {
        std::cout << "Test skipped because GL_EXT_disjoint_timer_query is not available."
                  << std::endl;
        return;
    }

    GLint queryTimeElapsedBits = 0;
    glGetQueryivEXT(GL_TIME_ELAPSED_EXT, GL_QUERY_COUNTER_BITS_EXT, &queryTimeElapsedBits);
    ASSERT_GL_NO_ERROR();

    std::cout << "Time elapsed counter bits: " << queryTimeElapsedBits << std::endl;

    // Skip test if the number of bits is 0
    if (queryTimeElapsedBits == 0)
    {
        std::cout << "Test skipped because of 0 counter bits" << std::endl;
        return;
    }

    GLuint query = 0;
    glGenQueriesEXT(-1, &query);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    glGenQueriesEXT(1, &query);
    EXPECT_GL_NO_ERROR();

    glBeginQueryEXT(GL_TIMESTAMP_EXT, query);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glBeginQueryEXT(GL_TIME_ELAPSED_EXT, 0);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glEndQueryEXT(GL_TIME_ELAPSED_EXT);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glBeginQueryEXT(GL_TIME_ELAPSED_EXT, query);
    EXPECT_GL_NO_ERROR();

    glBeginQueryEXT(GL_TIME_ELAPSED_EXT, query);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glEndQueryEXT(GL_TIME_ELAPSED_EXT);
    EXPECT_GL_NO_ERROR();

    glEndQueryEXT(GL_TIME_ELAPSED_EXT);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Tests GPU timestamp functionality
TEST_P(TimerQueriesTest, Timestamp)
{
    if (!extensionEnabled("GL_EXT_disjoint_timer_query"))
    {
        std::cout << "Test skipped because GL_EXT_disjoint_timer_query is not available."
                  << std::endl;
        return;
    }

    GLint queryTimestampBits = 0;
    glGetQueryivEXT(GL_TIMESTAMP_EXT, GL_QUERY_COUNTER_BITS_EXT, &queryTimestampBits);
    ASSERT_GL_NO_ERROR();

    std::cout << "Timestamp counter bits: " << queryTimestampBits << std::endl;

    // Macs for some reason return 0 bits so skip the test for now if either are 0
    if (queryTimestampBits == 0)
    {
        std::cout << "Test skipped because of 0 counter bits" << std::endl;
        return;
    }

    glDepthMask(GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    GLuint query1 = 0;
    GLuint query2 = 0;
    glGenQueriesEXT(1, &query1);
    glGenQueriesEXT(1, &query2);
    glQueryCounterEXT(query1, GL_TIMESTAMP_EXT);
    drawQuad(mProgram, "position", 0.8f);
    glQueryCounterEXT(query2, GL_TIMESTAMP_EXT);

    ASSERT_GL_NO_ERROR();

    swapBuffers();

    int timeout  = 10000;
    GLuint ready = GL_FALSE;
    while (ready == GL_FALSE && timeout > 0)
    {
        angle::Sleep(0);
        glGetQueryObjectuivEXT(query1, GL_QUERY_RESULT_AVAILABLE_EXT, &ready);
        timeout--;
    }
    ready = GL_FALSE;
    while (ready == GL_FALSE && timeout > 0)
    {
        angle::Sleep(0);
        glGetQueryObjectuivEXT(query2, GL_QUERY_RESULT_AVAILABLE_EXT, &ready);
        timeout--;
    }
    ASSERT_LT(0, timeout) << "Query result available timed out" << std::endl;

    GLuint64 result1 = 0;
    GLuint64 result2 = 0;
    glGetQueryObjectui64vEXT(query1, GL_QUERY_RESULT_EXT, &result1);
    glGetQueryObjectui64vEXT(query2, GL_QUERY_RESULT_EXT, &result2);

    ASSERT_GL_NO_ERROR();

    glDeleteQueriesEXT(1, &query1);
    glDeleteQueriesEXT(1, &query2);

    std::cout << "Timestamps: " << result1 << " " << result2 << std::endl;
    EXPECT_LT(0ul, result1);
    EXPECT_LT(0ul, result2);
    EXPECT_LT(result1, result2);
}

ANGLE_INSTANTIATE_TEST(TimerQueriesTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES2_OPENGL(),
                       ES3_OPENGL());
