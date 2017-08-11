//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Multiview draw tests:
// Test issuing multiview Draw* commands.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

class MultiviewDrawTest : public ANGLETest
{
  protected:
    MultiviewDrawTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setWebGLCompatibilityEnabled(true);
    }
    virtual ~MultiviewDrawTest() {}

    void SetUp() override
    {
        ANGLETest::SetUp();

        glRequestExtensionANGLE = reinterpret_cast<PFNGLREQUESTEXTENSIONANGLEPROC>(
            eglGetProcAddress("glRequestExtensionANGLE"));
    }

    // Requests the ANGLE_multiview extension and returns true if the operation succeeds.
    bool requestMultiviewExtension()
    {
        if (extensionRequestable("GL_ANGLE_multiview"))
        {
            glRequestExtensionANGLE("GL_ANGLE_multiview");
        }

        if (!extensionEnabled("GL_ANGLE_multiview"))
        {
            std::cout << "Test skipped due to missing GL_ANGLE_multiview." << std::endl;
            return false;
        }
        return true;
    }

    PFNGLREQUESTEXTENSIONANGLEPROC glRequestExtensionANGLE = nullptr;
};

class MultiviewDrawValidationTest : public MultiviewDrawTest
{
  protected:
    MultiviewDrawValidationTest() {}

    void SetUp() override
    {
        MultiviewDrawTest::SetUp();

        glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);

        glBindTexture(GL_TEXTURE_2D, mTex2d);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindVertexArray(mVao);

        const float kVertexData[3] = {0.0f};
        glBindBuffer(GL_ARRAY_BUFFER, mVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3u, &kVertexData[0], GL_STATIC_DRAW);

        const unsigned int kIndices[3] = {0u, 1u, 2u};
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3, &kIndices[0],
                     GL_STATIC_DRAW);
        ASSERT_GL_NO_ERROR();
    }

    GLTexture mTex2d;
    GLVertexArray mVao;
    GLBuffer mVbo;
    GLBuffer mIbo;
    GLFramebuffer mFramebuffer;
};

class MultiviewSideBySideRenderTest : public MultiviewDrawTest
{
  protected:
    void createFBO(int width, int height, int numViews)
    {
        // Assert that width is a multiple of numViews.
        ASSERT_TRUE(width % numViews == 0);
        const int widthPerView = width / numViews;

        // Create color and depth textures.
        glBindTexture(GL_TEXTURE_2D, mColorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        ASSERT_GL_NO_ERROR();

        glBindTexture(GL_TEXTURE_2D, mDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT,
                     GL_FLOAT, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);
        ASSERT_GL_NO_ERROR();

        // Create draw framebuffer to be used for side-by-side rendering.
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mDrawFramebuffer);
        std::vector<GLint> viewportOffsets(numViews * 2u);
        for (int i = 0u; i < numViews; ++i)
        {
            viewportOffsets[i * 2]     = i * widthPerView;
            viewportOffsets[i * 2 + 1] = 0;
        }
        glFramebufferTextureMultiviewSideBySideANGLE(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                                     mColorTexture, 0, numViews,
                                                     &viewportOffsets[0]);
        glFramebufferTextureMultiviewSideBySideANGLE(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                                     mDepthTexture, 0, numViews,
                                                     &viewportOffsets[0]);

        GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(1, DrawBuffers);
        ASSERT_GL_NO_ERROR();
        ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));

        // Create read framebuffer to be used to retrieve the pixel information for testing
        // purposes.
        glBindFramebuffer(GL_READ_FRAMEBUFFER, mReadFramebuffer);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               mColorTexture, 0);
        ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_READ_FRAMEBUFFER));

        // Clear the buffers.
        glViewport(0, 0, widthPerView, height);
        glScissor(0, 0, widthPerView, height);
        glEnable(GL_SCISSOR_TEST);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Set viewport and scissor of each view.
        glViewport(0, 0, widthPerView, height);
        glScissor(0, 0, widthPerView, height);
    }

    GLTexture mColorTexture;
    GLTexture mDepthTexture;
    GLFramebuffer mDrawFramebuffer;
    GLFramebuffer mReadFramebuffer;
};

class MultiviewSideBySideRenderDualViewTest : public MultiviewSideBySideRenderTest
{
  protected:
    MultiviewSideBySideRenderDualViewTest() : mProgram(0u) {}
    ~MultiviewSideBySideRenderDualViewTest()
    {
        if (mProgram != 0u)
        {
            glDeleteProgram(mProgram);
        }
    }

    void SetUp() override
    {
        MultiviewSideBySideRenderTest::SetUp();

        if (!requestMultiviewExtension())
        {
            return;
        }

        const std::string vsSource =
            "#version 300 es\n"
            "#extension GL_OVR_multiview : require\n"
            "layout(num_views = 2) in;\n"
            "in vec4 vPosition;\n"
            "void main()\n"
            "{\n"
            "   gl_Position.x = (gl_ViewID_OVR == 0u ? vPosition.x*0.5 + 0.5 : vPosition.x*0.5);\n"
            "   gl_Position.yzw = vPosition.yzw;\n"
            "}\n";

        const std::string fsSource =
            "#version 300 es\n"
            "#extension GL_OVR_multiview : require\n"
            "precision mediump float;\n"
            "out vec4 col;\n"
            "void main()\n"
            "{\n"
            "   col = vec4(1,0,0,0);\n"
            "}\n";

        createFBO(4, 1, 2);
        createProgram(vsSource, fsSource);
    }

    void createProgram(const std::string &vs, const std::string &fs)
    {
        mProgram = CompileProgram(vs, fs);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }
        glUseProgram(mProgram);
        ASSERT_GL_NO_ERROR();
    }

    void checkOutput()
    {
        EXPECT_PIXEL_EQ(0, 0, 0, 0, 0, 0);
        EXPECT_PIXEL_EQ(1, 0, 255, 0, 0, 0);
        EXPECT_PIXEL_EQ(2, 0, 255, 0, 0, 0);
        EXPECT_PIXEL_EQ(3, 0, 0, 0, 0, 0);
    }

    GLuint mProgram;
};

class MultiviewSideBySideOcclusionQueryTest : public MultiviewSideBySideRenderTest
{
  protected:
    MultiviewSideBySideOcclusionQueryTest() {}

    GLuint drawAndRetrieveOcclusionQueryResult(GLuint program)
    {
        GLuint query;
        glGenQueries(1, &query);
        glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
        drawQuad(program, "vPosition", 0.0f, 1.0f, true);
        glEndQueryEXT(GL_ANY_SAMPLES_PASSED);

        GLuint result = GL_TRUE;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &result);
        return result;
    }
};

// The test verifies that glDraw*Indirect:
// 1) generates an INVALID_OPERATION error if the number of views in the draw framebuffer is greater
// than 1.
// 2) does not generate any error if the draw framebuffer has exactly 1 view.
TEST_P(MultiviewDrawValidationTest, IndirectDraw)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const GLint viewportOffsets[4] = {0, 0, 2, 0};

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "void main()\n"
        "{}\n";

    GLBuffer commandBuffer;
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
    const GLuint commandData[] = {1u, 1u, 0u, 0u, 0u};
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(GLuint) * 5u, &commandData[0], GL_STATIC_DRAW);
    ASSERT_GL_NO_ERROR();

    // Check for a GL_INVALID_OPERATION error with the framebuffer having 2 views.
    {
        const std::string &vsSource =
            "#version 300 es\n"
            "#extension GL_OVR_multiview : require\n"
            "layout(num_views = 2) in;\n"
            "void main()\n"
            "{}\n";
        ANGLE_GL_PROGRAM(program, vsSource, fsSource);
        glUseProgram(program);

        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 2, &viewportOffsets[0]);

        glDrawArraysIndirect(GL_TRIANGLES, nullptr);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);

        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }

    // Check that no errors are generated if the number of views is 1.
    {
        const std::string &vsSource =
            "#version 300 es\n"
            "#extension GL_OVR_multiview : require\n"
            "layout(num_views = 1) in;\n"
            "void main()\n"
            "{}\n";
        ANGLE_GL_PROGRAM(program, vsSource, fsSource);
        glUseProgram(program);

        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 1, &viewportOffsets[0]);

        glDrawArraysIndirect(GL_TRIANGLES, nullptr);
        EXPECT_GL_NO_ERROR();

        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
        EXPECT_GL_NO_ERROR();
    }
}

// The test verifies that glDraw*:
// 1) generates an INVALID_OPERATION error if the number of views in the active draw framebuffer and
// program differs.
// 2) does not generate any error if the number of views is the same.
// 3) does not generate any error if the program does not use the multiview extension.
TEST_P(MultiviewDrawValidationTest, NumViewsMismatch)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const GLint viewportOffsets[4] = {0, 0, 2, 0};

    const std::string &vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "void main()\n"
        "{}\n";
    const std::string &fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "void main()\n"
        "{}\n";
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);

    // Check for a GL_INVALID_OPERATION error with the framebuffer and program having different
    // number of views.
    {
        // The framebuffer has only 1 view.
        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 1, &viewportOffsets[0]);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);

        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }

    // Check that no errors are generated if the number of views in both program and draw
    // framebuffer matches.
    {
        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 2, &viewportOffsets[0]);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        EXPECT_GL_NO_ERROR();

        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
        EXPECT_GL_NO_ERROR();
    }

    // Check that no errors are generated if the program does not use the multiview extension.
    {
        const std::string &vsSourceNoMultiview =
            "#version 300 es\n"
            "void main()\n"
            "{}\n";
        const std::string &fsSourceNoMultiview =
            "#version 300 es\n"
            "precision mediump float;\n"
            "void main()\n"
            "{}\n";
        ANGLE_GL_PROGRAM(programNoMultiview, vsSourceNoMultiview, fsSourceNoMultiview);
        glUseProgram(programNoMultiview);

        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 2, &viewportOffsets[0]);

        glDrawArrays(GL_TRIANGLES, 0, 3);
        EXPECT_GL_NO_ERROR();

        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);
        EXPECT_GL_NO_ERROR();
    }
}

// The test verifies that glDraw*:
// 1) generates an INVALID_OPERATION error if the number of views in the active draw framebuffer is
// greater than 1 and there is an active transform feedback object.
// 2) does not generate any error if the number of views in the draw framebuffer is 1.
TEST_P(MultiviewDrawValidationTest, ActiveTransformFeedback)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const GLint viewportOffsets[4] = {0, 0, 2, 0};

    const std::string &vsSource =
        "#version 300 es\n"
        "void main()\n"
        "{}\n";
    const std::string &fsSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "void main()\n"
        "{}\n";
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);

    GLBuffer tbo;
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, tbo);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(float) * 4u, nullptr, GL_STATIC_DRAW);

    GLTransformFeedback transformFeedback;
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedback);
    glBeginTransformFeedback(GL_TRIANGLES);
    ASSERT_GL_NO_ERROR();

    // Check that drawArrays generates an error when there is an active transform feedback object
    // and the number of views in the draw framebuffer is greater than 1.
    {
        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 2, &viewportOffsets[0]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }

    // Check that drawArrays does not generate an error when the number of views in the draw
    // framebuffer is 1.
    {
        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 1, &viewportOffsets[0]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        EXPECT_GL_NO_ERROR();
    }

    glEndTransformFeedback();
}

// The test verifies that glDraw*:
// 1) generates an INVALID_OPERATION error if the number of views in the active draw framebuffer is
// greater than 1 and there is an active query for target GL_TIME_ELAPSED_EXT.
// 2) does not generate any error if the number of views in the draw framebuffer is 1.
TEST_P(MultiviewDrawValidationTest, ActiveTimeElapsedQuery)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    if (!extensionEnabled("GL_EXT_disjoint_timer_query"))
    {
        std::cout << "Test skipped because GL_EXT_disjoint_timer_query is not available."
                  << std::endl;
        return;
    }

    const GLint viewportOffsets[4] = {0, 0, 2, 0};
    const std::string &vsSource =
        "#version 300 es\n"
        "void main()\n"
        "{}\n";
    const std::string &fsSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "void main()\n"
        "{}\n";
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);

    GLuint query = 0u;
    glGenQueriesEXT(1, &query);
    glBeginQueryEXT(GL_TIME_ELAPSED_EXT, query);

    // Check first case.
    {
        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 2, &viewportOffsets[0]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }

    // Check second case.
    {
        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTex2d,
                                                     0, 1, &viewportOffsets[0]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        EXPECT_GL_NO_ERROR();
    }

    glEndQueryEXT(GL_TIME_ELAPSED_EXT);
    glDeleteQueries(1, &query);
}

// The test checks that glDrawArrays can be used to render into two views.
TEST_P(MultiviewSideBySideRenderDualViewTest, DrawArrays)
{
    if (!requestMultiviewExtension())
    {
        return;
    }
    drawQuad(mProgram, "vPosition", 0.0f, 1.0f, true);
    ASSERT_GL_NO_ERROR();

    checkOutput();
}

// The test checks that glDrawElements can be used to render into two views.
TEST_P(MultiviewSideBySideRenderDualViewTest, DrawElements)
{
    if (!requestMultiviewExtension())
    {
        return;
    }
    drawIndexedQuad(mProgram, "vPosition", 0.0f, 1.0f, true);
    ASSERT_GL_NO_ERROR();

    checkOutput();
}

// The test checks that glDrawRangeElements can be used to render into two views.
TEST_P(MultiviewSideBySideRenderDualViewTest, DrawRangeElements)
{
    if (!requestMultiviewExtension())
    {
        return;
    }
    drawIndexedQuad(mProgram, "vPosition", 0.0f, 1.0f, true, true);
    ASSERT_GL_NO_ERROR();

    checkOutput();
}

// The test checks that glDrawArrays can be used to render into four views.
TEST_P(MultiviewSideBySideRenderTest, DrawArraysFourViews)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const std::string vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "layout(num_views = 4) in;\n"
        "in vec4 vPosition;\n"
        "void main()\n"
        "{\n"
        "   if (gl_ViewID_OVR == 0u) {\n"
        "       gl_Position.x = vPosition.x*0.25 - 0.75;\n"
        "   } else if (gl_ViewID_OVR == 1u) {\n"
        "       gl_Position.x = vPosition.x*0.25 - 0.25;\n"
        "   } else if (gl_ViewID_OVR == 2u) {\n"
        "       gl_Position.x = vPosition.x*0.25 + 0.25;\n"
        "   } else {\n"
        "       gl_Position.x = vPosition.x*0.25 + 0.75;\n"
        "   }"
        "   gl_Position.yzw = vPosition.yzw;\n"
        "}\n";

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(1,0,0,0);\n"
        "}\n";

    createFBO(16, 1, 4);
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);

    drawQuad(program, "vPosition", 0.0f, 1.0f, true);
    ASSERT_GL_NO_ERROR();

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            const int arrayIndex = i * 4 + j;
            if (i == j)
            {
                EXPECT_PIXEL_EQ(arrayIndex, 0, 255, 0, 0, 0);
            }
            else
            {
                EXPECT_PIXEL_EQ(arrayIndex, 0, 0, 0, 0, 0);
            }
        }
    }
    EXPECT_GL_NO_ERROR();
}

// The test checks that glDrawArraysInstanced can be used to render into two views.
TEST_P(MultiviewSideBySideRenderTest, DrawArraysInstanced)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const std::string vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "in vec4 vPosition;\n"
        "void main()\n"
        "{\n"
        "       vec4 p = vPosition;\n"
        "       if (gl_InstanceID == 1){\n"
        "               p.y = .5*p.y + .5;\n"
        "       } else {\n"
        "               p.y = p.y*.5;\n"
        "       }\n"
        "       gl_Position.x = (gl_ViewID_OVR == 0u ? p.x*0.5 + 0.5 : p.x*0.5);\n"
        "       gl_Position.yzw = p.yzw;\n"
        "}\n";

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(1,0,0,0);\n"
        "}\n";

    createFBO(4, 2, 2);
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);

    drawQuad(program, "vPosition", 0.0f, 1.0f, true, true, 2);
    ASSERT_GL_NO_ERROR();

    const GLubyte expectedResult[4][8] = {
        {0, 255, 255, 0}, {0, 255, 255, 0},
    };
    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            EXPECT_PIXEL_EQ(col, row, expectedResult[row][col], 0, 0, 0);
        }
    }
}

// The test verifies that the attribute divisor is correctly adjusted when drawing with a multi-view
// program. The test draws 4 instances of a quad each of which covers a single pixel. The x and y
// offset of each quad are passed as separate attributes which are indexed based on the
// corresponding attribute divisors. A divisor of 1 is used for the y offset to have all quads
// drawn vertically next to each other. A divisor of 3 is used for the x offset to have the last
// quad offsetted by one pixel to the right. Note that the number of views is divisible by 1, but
// not by 3.
TEST_P(MultiviewSideBySideRenderTest, AttribDivisor)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const std::string &vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "layout(num_views = 2) in;\n"
        "in vec3 vPosition;\n"
        "in float offsetX;\n"
        "in float offsetY;\n"
        "void main()\n"
        "{\n"
        "       vec4 p = vec4(vPosition, 1.);\n"
        "       p.xy = p.xy * 0.25 - 0.75 + vec2(offsetX, offsetY);\n"
        "       gl_Position.x = (gl_ViewID_OVR == 0u ? p.x : p.x + 1.0);\n"
        "       gl_Position.yzw = p.yzw;\n"
        "}\n";

    const std::string &fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(1,0,0,0);\n"
        "}\n";
    createFBO(8, 4, 2);
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);

    GLBuffer xOffsetVBO;
    glBindBuffer(GL_ARRAY_BUFFER, xOffsetVBO);
    const GLfloat xOffsetData[4] = {0.0f, 0.5f, 1.0f, 1.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4, xOffsetData, GL_STATIC_DRAW);
    GLint xOffsetLoc = glGetAttribLocation(program, "offsetX");
    glVertexAttribPointer(xOffsetLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribDivisor(xOffsetLoc, 3);
    glEnableVertexAttribArray(xOffsetLoc);

    GLBuffer yOffsetVBO;
    glBindBuffer(GL_ARRAY_BUFFER, yOffsetVBO);
    const GLfloat yOffsetData[4] = {0.0f, 0.5f, 1.0f, 1.5f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 4, yOffsetData, GL_STATIC_DRAW);
    GLint yOffsetLoc = glGetAttribLocation(program, "offsetY");
    glVertexAttribDivisor(yOffsetLoc, 1);
    glVertexAttribPointer(yOffsetLoc, 1, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(yOffsetLoc);

    drawQuad(program, "vPosition", 0.0f, 1.0f, true, true, 4);
    ASSERT_GL_NO_ERROR();

    const GLubyte expectedRedChannel[4][8] = {{255, 0, 0, 0, 0, 0, 255, 0},
                                              {255, 0, 0, 0, 0, 0, 255, 0},
                                              {255, 0, 0, 0, 0, 0, 255, 0},
                                              {0, 255, 0, 0, 0, 0, 0, 255}};
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            EXPECT_PIXEL_EQ(col, row, expectedRedChannel[row][col], 0, 0, 0);
        }
    }
}

// Test that different sequences of vertexAttribDivisor, useProgram and bindVertexArray in a
// multi-view context propagate the correct divisor to the driver.
TEST_P(MultiviewSideBySideRenderTest, DivisorOrderOfOperation)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    createFBO(2, 1, 2);

    // Create multiview program.
    const std::string &vs =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "layout(num_views = 2) in;\n"
        "layout(location = 0) in vec2 vPosition;\n"
        "layout(location = 1) in float offsetX;\n"
        "void main()\n"
        "{\n"
        "       vec4 p = vec4(vPosition, 0.0, 1.0);\n"
        "       p.x += offsetX;\n"
        "       gl_Position = p;\n"
        "}\n";

    const std::string &fs =
        "#version 300 es\n"
        "#extension GL_OVR_multiview2 : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(1,0,0,0);\n"
        "}\n";

    ANGLE_GL_PROGRAM(program, vs, fs);

    const std::string &dummyVS =
        "#version 300 es\n"
        "layout(location = 0) in vec2 vPosition;\n"
        "layout(location = 1) in float offsetX;\n"
        "void main()\n"
        "{\n"
        "       gl_Position = vec4(vPosition, 0.0, 1.0);\n"
        "}\n";

    const std::string &dummyFS =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(0,0,0,0);\n"
        "}\n";

    ANGLE_GL_PROGRAM(dummyProgram, dummyVS, dummyFS);

    GLBuffer xOffsetVBO;
    glBindBuffer(GL_ARRAY_BUFFER, xOffsetVBO);
    const GLfloat xOffsetData[12] = {0.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f,
                                     4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f};
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, xOffsetData, GL_STATIC_DRAW);

    GLBuffer vertexVBO;
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    Vector2 kQuadVertices[6] = {Vector2(-1.f, -1.f), Vector2(1.f, -1.f), Vector2(1.f, 1.f),
                                Vector2(-1.f, -1.f), Vector2(1.f, 1.f),  Vector2(-1.f, 1.f)};
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices), kQuadVertices, GL_STATIC_DRAW);

    GLVertexArray vao[2];
    for (size_t i = 0u; i < 2u; ++i)
    {
        glBindVertexArray(vao[i]);

        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, xOffsetVBO);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);
    }
    ASSERT_GL_NO_ERROR();

    glViewport(0, 0, 1, 1);
    glScissor(0, 0, 1, 1);
    glEnable(GL_SCISSOR_TEST);
    glClearColor(0, 0, 0, 0);

    // Clear the buffers, propagate divisor to the driver, bind the vao and keep it active.
    // It is necessary to call draw, so that the divisor is propagated and to guarantee that dirty
    // bits are cleared.
    glUseProgram(dummyProgram);
    glBindVertexArray(vao[0]);
    glVertexAttribDivisor(1, 0);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
    glUseProgram(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Check that vertexAttribDivisor uses the number of views to update the divisor.
    glUseProgram(program);
    glVertexAttribDivisor(1, 1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
    EXPECT_PIXEL_EQ(0, 0, 255, 0, 0, 0);
    EXPECT_PIXEL_EQ(1, 0, 255, 0, 0, 0);

    // Clear the buffers and propagate divisor to the driver.
    // We keep the vao active and propagate the divisor to guarantee that there are no unresolved
    // dirty bits when useProgram is called.
    glUseProgram(dummyProgram);
    glVertexAttribDivisor(1, 1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
    glUseProgram(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Check that useProgram uses the number of views to update the divisor.
    glUseProgram(program);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
    EXPECT_PIXEL_EQ(0, 0, 255, 0, 0, 0);
    EXPECT_PIXEL_EQ(1, 0, 255, 0, 0, 0);

    // We go through similar steps as before.
    glUseProgram(dummyProgram);
    glVertexAttribDivisor(1, 1);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
    glUseProgram(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Check that bindVertexArray uses the number of views to update the divisor.
    {
        // Call useProgram with vao[1] being active to guarantee that useProgram will adjust the
        // divisor for vao[1] only.
        glBindVertexArray(vao[1]);
        glUseProgram(program);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(0);
        ASSERT_GL_NO_ERROR();
    }
    // Bind vao[0] after useProgram is called to ensure that bindVertexArray is the call which
    // adjusts the divisor.
    glBindVertexArray(vao[0]);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 1);
    EXPECT_PIXEL_EQ(0, 0, 255, 0, 0, 0);
    EXPECT_PIXEL_EQ(1, 0, 255, 0, 0, 0);
}

// Test that no fragments pass the occlusion query for a multi-view vertex shader which always
// transforms geometry to be outside of the clip region.
TEST_P(MultiviewSideBySideOcclusionQueryTest, OcclusionQueryNothingVisible)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const std::string vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "in vec3 vPosition;\n"
        "void main()\n"
        "{\n"
        "       gl_Position.x = 2.0;\n"
        "       gl_Position.yzw = vec3(vPosition.yz, 1.);\n"
        "}\n";

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(1,0,0,0);\n"
        "}\n";
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);
    createFBO(2, 1, 2);

    GLuint result = drawAndRetrieveOcclusionQueryResult(program);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FALSE(result);
}

// Test that there are fragments passing the occlusion query if only view 0 can produce
// output.
TEST_P(MultiviewSideBySideOcclusionQueryTest, OcclusionQueryOnlyLeftVisible)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const std::string vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "in vec3 vPosition;\n"
        "void main()\n"
        "{\n"
        "       gl_Position.x = gl_ViewID_OVR == 0u ? vPosition.x : 2.0;\n"
        "       gl_Position.yzw = vec3(vPosition.yz, 1.);\n"
        "}\n";

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(1,0,0,0);\n"
        "}\n";
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);
    createFBO(2, 1, 2);

    GLuint result = drawAndRetrieveOcclusionQueryResult(program);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_TRUE(result);
}

// Test that there are fragments passing the occlusion query if only view 1 can produce
// output.
TEST_P(MultiviewSideBySideOcclusionQueryTest, OcclusionQueryOnlyRightVisible)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    const std::string vsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "layout(num_views = 2) in;\n"
        "in vec3 vPosition;\n"
        "void main()\n"
        "{\n"
        "       gl_Position.x = gl_ViewID_OVR == 1u ? vPosition.x : 2.0;\n"
        "       gl_Position.yzw = vec3(vPosition.yz, 1.);\n"
        "}\n";

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "out vec4 col;\n"
        "void main()\n"
        "{\n"
        "    col = vec4(1,0,0,0);\n"
        "}\n";
    ANGLE_GL_PROGRAM(program, vsSource, fsSource);
    glUseProgram(program);
    createFBO(2, 1, 2);

    GLuint result = drawAndRetrieveOcclusionQueryResult(program);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_TRUE(result);
}

ANGLE_INSTANTIATE_TEST(MultiviewDrawValidationTest, ES31_OPENGL());
ANGLE_INSTANTIATE_TEST(MultiviewSideBySideRenderDualViewTest, ES3_OPENGL());
ANGLE_INSTANTIATE_TEST(MultiviewSideBySideRenderTest, ES3_OPENGL());
ANGLE_INSTANTIATE_TEST(MultiviewSideBySideOcclusionQueryTest, ES3_OPENGL());
