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

ANGLE_INSTANTIATE_TEST(MultiviewDrawValidationTest, ES31_OPENGL());