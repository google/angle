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

// The test verifies that glDraw*Indirect:
// 1) generates an INVALID_OPERATION error if the number of views in the draw framebuffer is greater
// than 1.
// 2) does not generate any error if the draw framebuffer has exactly 1 view.
TEST_P(MultiviewDrawTest, IndirectDraw)
{
    if (!requestMultiviewExtension())
    {
        return;
    }

    GLTexture tex2d;
    glBindTexture(GL_TEXTURE_2D, tex2d);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    const GLint viewportOffsets[4] = {0, 0, 2, 0};
    ASSERT_GL_NO_ERROR();

    const std::string fsSource =
        "#version 300 es\n"
        "#extension GL_OVR_multiview : require\n"
        "precision mediump float;\n"
        "void main()\n"
        "{}\n";

    GLVertexArray vao;
    glBindVertexArray(vao);

    const float kVertexData[3]     = {0.0f};
    const unsigned int kIndices[3] = {0u, 1u, 2u};

    GLBuffer vbo;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3u, &kVertexData[0], GL_STATIC_DRAW);
    ASSERT_GL_NO_ERROR();

    GLBuffer ibo;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 3, &kIndices[0], GL_STATIC_DRAW);
    ASSERT_GL_NO_ERROR();

    GLBuffer commandBuffer;
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
    const GLuint commandData[] = {1u, 1u, 0u, 0u, 0u};
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(GLuint) * 5u, &commandData[0], GL_STATIC_DRAW);
    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

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

        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex2d, 0,
                                                     2, &viewportOffsets[0]);

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

        glFramebufferTextureMultiviewSideBySideANGLE(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex2d, 0,
                                                     1, &viewportOffsets[0]);

        glDrawArraysIndirect(GL_TRIANGLES, nullptr);
        EXPECT_GL_NO_ERROR();

        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
        EXPECT_GL_NO_ERROR();
    }
}

ANGLE_INSTANTIATE_TEST(MultiviewDrawTest, ES31_OPENGL());