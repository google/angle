//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// MultisampledRenderToTextureTest: Tests of EXT_multisampled_render_to_texture extension

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{
class MultisampledRenderToTextureTest : public ANGLETest
{
  protected:
    MultisampledRenderToTextureTest()
    {
        setWindowWidth(64);
        setWindowHeight(64);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void testSetUp() override {}

    void testTearDown() override {}

    void assertErrorIfNotMSRTT2(GLenum error)
    {
        if (EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"))
        {
            ASSERT_GL_NO_ERROR();
        }
        else
        {
            ASSERT_GL_ERROR(error);
        }
    }

    void setupCopyTexProgram()
    {
        mCopyTextureProgram.makeRaster(essl1_shaders::vs::Texture2D(),
                                       essl1_shaders::fs::Texture2D());
        ASSERT_GL_TRUE(mCopyTextureProgram.valid());

        mCopyTextureUniformLocation =
            glGetUniformLocation(mCopyTextureProgram, essl1_shaders::Texture2DUniform());

        ASSERT_GL_NO_ERROR();
    }

    void setupUniformColorProgramMultiRenderTarget(const bool bufferEnabled[8], GLuint *programOut)
    {
        std::stringstream fs;

        fs << "#extension GL_EXT_draw_buffers : enable\n"
              "precision highp float;\n"
              "uniform mediump vec4 "
           << essl1_shaders::ColorUniform()
           << ";\n"
              "void main()\n"
              "{\n";

        for (unsigned int index = 0; index < 8; index++)
        {
            if (bufferEnabled[index])
            {
                fs << "    gl_FragData[" << index << "] = " << essl1_shaders::ColorUniform()
                   << ";\n";
            }
        }

        fs << "}\n";

        *programOut = CompileProgram(essl1_shaders::vs::Simple(), fs.str().c_str());
        ASSERT_NE(*programOut, 0u);
    }

    void verifyResults(GLuint texture,
                       const GLColor expected,
                       GLint fboSize,
                       GLint xs,
                       GLint ys,
                       GLint xe,
                       GLint ye)
    {
        glViewport(0, 0, fboSize, fboSize);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Draw a quad with the target texture
        glUseProgram(mCopyTextureProgram);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(mCopyTextureUniformLocation, 0);

        drawQuad(mCopyTextureProgram, essl1_shaders::PositionAttrib(), 0.5f);

        // Expect that the rendered quad has the same color as the source texture
        EXPECT_PIXEL_COLOR_NEAR(xs, ys, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR(xs, ye - 1, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR(xe - 1, ys, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR(xe - 1, ye - 1, expected, 1.0);
        EXPECT_PIXEL_COLOR_NEAR((xs + xe) / 2, (ys + ye) / 2, expected, 1.0);
    }

    void clearAndDrawQuad(GLuint program, GLsizei viewportWidth, GLsizei viewportHeight)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, viewportWidth, viewportHeight);
        ASSERT_GL_NO_ERROR();

        drawQuad(program, essl1_shaders::PositionAttrib(), 0.0f);
        ASSERT_GL_NO_ERROR();
    }

    GLProgram mCopyTextureProgram;
    GLint mCopyTextureUniformLocation = -1;
};

class MultisampledRenderToTextureES3Test : public MultisampledRenderToTextureTest
{};

class MultisampledRenderToTextureES31Test : public MultisampledRenderToTextureTest
{};

// Checking against invalid parameters for RenderbufferStorageMultisampleEXT.
TEST_P(MultisampledRenderToTextureTest, RenderbufferParameterCheck)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);

    // Positive test case
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, 64, 64);
    ASSERT_GL_NO_ERROR();

    GLint samples;
    glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
    ASSERT_GL_NO_ERROR();
    EXPECT_GE(samples, 1);

    // Samples too large
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, samples + 1, GL_DEPTH_COMPONENT16, 64, 64);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    // Renderbuffer size too large
    GLint maxSize;
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxSize);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 2, GL_DEPTH_COMPONENT16, maxSize + 1,
                                        maxSize);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 2, GL_DEPTH_COMPONENT16, maxSize,
                                        maxSize + 1);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    // Retrieving samples
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, 64, 64);
    GLint param = 0;
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES_EXT, &param);
    // GE because samples may vary base on implementation. Spec says "the resulting value for
    // RENDERBUFFER_SAMPLES_EXT is guaranteed to be greater than or equal to samples and no more
    // than the next larger sample count supported by the implementation"
    EXPECT_GE(param, 4);
}

// Checking against invalid parameters for FramebufferTexture2DMultisampleEXT.
TEST_P(MultisampledRenderToTextureTest, Texture2DParameterCheck)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    bool isES3 = getClientMajorVersion() >= 3;

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    GLTexture depthTexture;
    if (isES3)
    {
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 64, 64, 0, GL_DEPTH_STENCIL,
                     GL_UNSIGNED_INT_24_8_OES, nullptr);
    }

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // Positive test case
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    ASSERT_GL_NO_ERROR();
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Attachment not COLOR_ATTACHMENT0.  Allowed only in EXT_multisampled_render_to_texture2
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                         texture, 0, 4);
    assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

    // Depth/stencil attachment.  Allowed only in EXT_multisampled_render_to_texture2
    if (isES3)
    {
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                                             depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                             depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                             GL_TEXTURE_2D, depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                             depthTexture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);
    }

    // Target not framebuffer
    glFramebufferTexture2DMultisampleEXT(GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);

    GLint samples;
    glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
    ASSERT_GL_NO_ERROR();
    EXPECT_GE(samples, 1);

    // Samples too large
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, samples + 1);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    // Retrieving samples
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    GLint param = 0;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SAMPLES_EXT, &param);
    // GE because samples may vary base on implementation. Spec says "the resulting value for
    // TEXTURE_SAMPLES_EXT is guaranteed to be greater than or equal to samples and no more than the
    // next larger sample count supported by the implementation"
    EXPECT_GE(param, 4);
}

// Checking against invalid parameters for FramebufferTexture2DMultisampleEXT (cubemap).
TEST_P(MultisampledRenderToTextureTest, TextureCubeMapParameterCheck)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    GLTexture texture;
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
    for (GLenum face = 0; face < 6; face++)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGBA, 64, 64, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, nullptr);
        ASSERT_GL_NO_ERROR();
    }

    GLint samples;
    glGetIntegerv(GL_MAX_SAMPLES_EXT, &samples);
    ASSERT_GL_NO_ERROR();
    EXPECT_GE(samples, 1);

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    for (GLenum face = 0; face < 6; face++)
    {
        // Positive test case
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        ASSERT_GL_NO_ERROR();
        EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

        // Attachment not COLOR_ATTACHMENT0.  Allowed only in EXT_multisampled_render_to_texture2
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        assertErrorIfNotMSRTT2(GL_INVALID_ENUM);

        // Target not framebuffer
        glFramebufferTexture2DMultisampleEXT(GL_RENDERBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        ASSERT_GL_ERROR(GL_INVALID_ENUM);

        // Samples too large
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0,
                                             samples + 1);
        ASSERT_GL_ERROR(GL_INVALID_VALUE);

        // Retrieving samples
        glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture, 0, 4);
        GLint param = 0;
        glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                              GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_SAMPLES_EXT,
                                              &param);
        // GE because samples may vary base on implementation. Spec says "the resulting value for
        // TEXTURE_SAMPLES_EXT is guaranteed to be greater than or equal to samples and no more than
        // the next larger sample count supported by the implementation"
        EXPECT_GE(param, 4);
    }
}

// Checking for framebuffer completeness using extension methods.
TEST_P(MultisampledRenderToTextureTest, FramebufferCompleteness)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    // Checking that Renderbuffer and texture2d having different number of samples results
    // in a FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    GLsizei maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, maxSamples, GL_DEPTH_COMPONENT16, 64, 64);
    ASSERT_GL_NO_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);

    if (maxSamples > 4)
    {
        EXPECT_GLENUM_EQ(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE,
                         glCheckFramebufferStatus(GL_FRAMEBUFFER));
    }
    else
    {
        EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    }
}

// Draw test with color attachment only.
TEST_P(MultisampledRenderToTextureTest, 2DColorAttachmentMultisampleDrawTest)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    // Set up texture and bind to FBO
    constexpr GLsizei kSize = 6;
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear to black
    glViewport(0, 0, kSize, kSize);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set up Green square program
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glUseProgram(program);
    GLint positionLocation = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    setupQuadVertexBuffer(0.5f, 0.5f);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionLocation);

    // Draw green square
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::black);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::green);

    // Set up Red square program
    ANGLE_GL_PROGRAM(program2, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glUseProgram(program2);
    GLint positionLocation2 = glGetAttribLocation(program2, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation2);

    setupQuadVertexBuffer(0.5f, 0.75f);
    glVertexAttribPointer(positionLocation2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // Draw red square
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::black);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw test using both color and depth attachments.
TEST_P(MultisampledRenderToTextureTest, 2DColorDepthMultisampleDrawTest)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 6;
    // create complete framebuffer with depth buffer
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);

    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, kSize, kSize);
    ASSERT_GL_NO_ERROR();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear framebuffer
    glViewport(0, 0, kSize, kSize);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClearDepthf(0.5f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw first green square
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GREATER);
    glUseProgram(program);
    GLint positionLocation = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    setupQuadVertexBuffer(0.8f, 0.5f);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionLocation);

    // Tests that TRIANGLES works.
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::black);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::green);

    // Draw red square behind green square
    ANGLE_GL_PROGRAM(program2, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glUseProgram(program2);
    GLint positionLocation2 = glGetAttribLocation(program2, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation2);

    setupQuadVertexBuffer(0.7f, 1.0f);
    glVertexAttribPointer(positionLocation2, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();
    glDisable(GL_DEPTH_TEST);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::green);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Read pixels with pack buffer. ES3+.
TEST_P(MultisampledRenderToTextureES3Test, ReadPixelsTest)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    constexpr GLsizei kSize = 6;
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, kSize, kSize);
    ASSERT_GL_NO_ERROR();

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Set viewport and clear to red
    glViewport(0, 0, kSize, kSize);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Bind Pack Pixel Buffer and read to it
    GLBuffer PBO;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, PBO);
    glBufferData(GL_PIXEL_PACK_BUFFER, 4 * kSize * kSize, nullptr, GL_STATIC_DRAW);
    glReadPixels(0, 0, kSize, kSize, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    ASSERT_GL_NO_ERROR();

    // Retrieving pixel color
    void *mappedPtr    = glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, 32, GL_MAP_READ_BIT);
    GLColor *dataColor = static_cast<GLColor *>(mappedPtr);
    EXPECT_GL_NO_ERROR();

    EXPECT_EQ(GLColor::red, dataColor[0]);

    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    EXPECT_GL_NO_ERROR();
}

// CopyTexImage from a multisampled texture functionality test.
TEST_P(MultisampledRenderToTextureTest, CopyTexImageTest)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 16;

    setupCopyTexProgram();
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Disable mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);

    // Set color for framebuffer
    glClearColor(0.25f, 1.0f, 0.75f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    GLTexture copyToTex;
    glBindTexture(GL_TEXTURE_2D, copyToTex);

    // Disable mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, kSize, kSize, 0);
    ASSERT_GL_NO_ERROR();

    verifyResults(copyToTex, {64, 255, 191, 255}, kSize, 0, 0, kSize, kSize);
}

// CopyTexSubImage from a multisampled texture functionality test.
TEST_P(MultisampledRenderToTextureTest, CopyTexSubImageTest)
{
    // Fails on Pixel 2. http://anglebug.com/4906
    ANGLE_SKIP_TEST_IF(IsAndroid());

    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 16;

    setupCopyTexProgram();

    GLTexture texture;
    // Create texture in copyFBO0 with color (.25, 1, .75, .5)
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Disable mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLFramebuffer copyFBO0;
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO0);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);

    // Set color for
    glClearColor(0.25f, 1.0f, 0.75f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Create texture in copyFBO[1] with color (1, .75, .5, .25)
    GLTexture texture1;
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Disable mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLFramebuffer copyFBO1;
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO1);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture1, 0, 4);

    // Set color for
    glClearColor(1.0f, 0.75f, 0.5f, 0.25f);
    glClear(GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    GLTexture copyToTex;
    glBindTexture(GL_TEXTURE_2D, copyToTex);

    // Disable mipmapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // copyFBO0 -> copyToTex
    // copyToTex should hold what was originally in copyFBO0 : (.25, 1, .75, .5)
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO0);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, kSize, kSize, 0);
    ASSERT_GL_NO_ERROR();

    const GLColor expected0(64, 255, 191, 255);
    verifyResults(copyToTex, expected0, kSize, 0, 0, kSize, kSize);

    // copyFBO[1] - copySubImage -> copyToTex
    // copyToTex should have subportion what was in copyFBO[1] : (1, .75, .5, .25)
    // The rest should still be untouched: (.25, 1, .75, .5)
    GLint half = kSize / 2;
    glBindFramebuffer(GL_FRAMEBUFFER, copyFBO1);
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, half, half, half, half, half, half);
    ASSERT_GL_NO_ERROR();

    const GLColor expected1(255, 191, 127, 255);
    verifyResults(copyToTex, expected1, kSize, half, half, kSize, kSize);

    // Verify rest is untouched
    verifyResults(copyToTex, expected0, kSize, 0, 0, half, half);
    verifyResults(copyToTex, expected0, kSize, 0, half, half, kSize);
    verifyResults(copyToTex, expected0, kSize, half, 0, kSize, half);
}

// BlitFramebuffer functionality test. ES3+.
TEST_P(MultisampledRenderToTextureES3Test, BlitFramebufferTest)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    constexpr GLsizei kSize = 16;

    // Create multisampled framebuffer to use as source.
    GLRenderbuffer depthMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthMS);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, kSize, kSize);

    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Clear depth to 0.5 and color to green.
    glClearDepthf(0.5f);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glFlush();
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawRed, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_EQUAL);
    drawQuad(drawRed, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Create single sampled framebuffer to use as dest.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    GLTexture colorSS;
    glBindTexture(GL_TEXTURE_2D, colorSS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorSS, 0);
    ASSERT_GL_NO_ERROR();

    // Bind MS to READ as SS is already bound to DRAW.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMS);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Bind SS to READ so we can readPixels from it
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);
    ASSERT_GL_NO_ERROR();
}

// GenerateMipmap functionality test
TEST_P(MultisampledRenderToTextureTest, GenerateMipmapTest)
{
    // Fails on Pixel 2. http://anglebug.com/4906
    ANGLE_SKIP_TEST_IF(IsAndroid());

    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();
    glUseProgram(mCopyTextureProgram);

    // Initialize texture with blue
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, kSize, kSize, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         texture, 0, 4);
    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, kSize, kSize);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    ASSERT_GL_NO_ERROR();

    // Generate mipmap
    glGenerateMipmap(GL_TEXTURE_2D);
    ASSERT_GL_NO_ERROR();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

    // Now draw the texture to various different sized areas.
    clearAndDrawQuad(mCopyTextureProgram, kSize, kSize);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);

    // Use mip level 1
    clearAndDrawQuad(mCopyTextureProgram, kSize / 2, kSize / 2);
    EXPECT_PIXEL_COLOR_EQ(kSize / 4, kSize / 4, GLColor::blue);

    // Use mip level 2
    clearAndDrawQuad(mCopyTextureProgram, kSize / 4, kSize / 4);
    EXPECT_PIXEL_COLOR_EQ(kSize / 8, kSize / 8, GLColor::blue);

    ASSERT_GL_NO_ERROR();
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
TEST_P(MultisampledRenderToTextureTest, DrawCopyThenBlend)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    // Create multisampled framebuffer to draw into
    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Draw again into the framebuffer, this time blending.  This tests that the framebuffer's data,
    // residing in the single-sampled texture, is available to the multisampled intermediate image
    // for blending.

    // Blend half-transparent green into the multisampled color buffer.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    ASSERT_GL_NO_ERROR();
    const GLColor expectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachment 1.
TEST_P(MultisampledRenderToTextureES3Test, ColorAttachment1)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    // Qualcomm driver crashes in the presence of VK_ATTACHMENT_UNUSED.
    // http://anglebug.com/3423
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAndroid());

    // Fails on Intel Ubuntu 19.04 Mesa 19.0.2 Vulkan. http://anglebug.com/3616
    ANGLE_SKIP_TEST_IF(IsLinux() && IsIntel() && IsVulkan());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Setup program to render into attachment 1.
    constexpr bool kBuffersEnabled[8] = {false, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_NONE, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffer.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy into it.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Blend half-transparent green into the multisampled color buffer.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor expectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();

    glDeleteProgram(drawColor);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachments 0 and 3.
TEST_P(MultisampledRenderToTextureES3Test, ColorAttachments0And3)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    // Qualcomm driver crashes in the presence of VK_ATTACHMENT_UNUSED.
    // http://anglebug.com/3423
    ANGLE_SKIP_TEST_IF(IsVulkan() && IsAndroid());

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture colorMS0;
    glBindTexture(GL_TEXTURE_2D, colorMS0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLTexture colorMS3;
    glBindTexture(GL_TEXTURE_2D, colorMS3);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS0, 0, 4);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D,
                                         colorMS3, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Setup program to render into attachments 0 and 3.
    constexpr bool kBuffersEnabled[8] = {true, false, false, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_NONE, GL_NONE,
                                       GL_COLOR_ATTACHMENT3};
    glDrawBuffers(4, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT3);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy from one of them.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Blend half-transparent green into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the textures are now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    glReadBuffer(GL_COLOR_ATTACHMENT0);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor expectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();

    glDeleteProgram(drawColor);
}

// Draw with depth buffer.  Uses EXT_multisampled_render_to_texture2.
// The test works with a 64x1 texture.  The first draw call will render geometry whose depth is
// different between top and bottom.  The second draw call will enable depth test and draw with the
// average of the two depths.  Only half of the samples will take the new color.  Once resolved, the
// expected color would be the average of the two draw colors.
TEST_P(MultisampledRenderToTextureES3Test, DepthStencilAttachment)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    constexpr GLsizei kWidth = 64;

    // Create multisampled framebuffer to draw into, with both color and depth attachments.
    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kWidth, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLTexture depthMS;
    glBindTexture(GL_TEXTURE_2D, depthMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, kWidth, 1, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8_OES, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                         depthMS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Setup draw program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);
    GLint positionLocation = glGetAttribLocation(drawColor, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, positionLocation);

    // Setup vertices such that depth is varied from top to bottom.
    std::array<Vector3, 6> quadVertices = {
        Vector3(-1.0f, 1.0f, 0.8f), Vector3(-1.0f, -1.0f, 0.2f), Vector3(1.0f, -1.0f, 0.2f),
        Vector3(-1.0f, 1.0f, 0.8f), Vector3(1.0f, -1.0f, 0.2f),  Vector3(1.0f, 1.0f, 0.8f),
    };
    GLBuffer quadVertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 6, quadVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(positionLocation);

    // Draw red into the framebuffer.
    glViewport(0, 0, kWidth, 1);
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Draw green such that half the samples of each pixel pass the depth test.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    glDepthFunc(GL_GREATER);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    const GLColor kExpected(127, 127, 0, 255);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kWidth - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kWidth / 2, 0, kExpected, 1);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Draw, copy, then blend.  The copy will make sure an implicit resolve happens.  Regardless, the
// following draw should retain the data written by the first draw command.
// Uses color attachments 0 and 1.  Attachment 0 is a normal multisampled texture, while attachment
// 1 is a multisampled-render-to-texture texture.
TEST_P(MultisampledRenderToTextureES31Test, MixedMultisampledAndMultisampledRenderToTexture)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    constexpr GLsizei kSize = 64;

    setupCopyTexProgram();

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture colorMS0;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorMS0);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, kSize, kSize, true);

    GLTexture colorMS1;
    glBindTexture(GL_TEXTURE_2D, colorMS1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           colorMS0, 0);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                         colorMS1, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Setup program to render into attachments 0 and 1.
    constexpr bool kBuffersEnabled[8] = {true, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create a texture and copy from one of them.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, kSize, kSize, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Blend half-transparent green into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 0.5f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the textures are now yellow
    const GLColor kExpected(127, 127, 0, 191);
    EXPECT_PIXEL_COLOR_NEAR(0, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, 0, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(0, kSize - 1, kExpected, 1);
    EXPECT_PIXEL_COLOR_NEAR(kSize - 1, kSize - 1, kExpected, 1);

    // For completeness, verify that the texture used as copy target is red.
    const GLColor expectedCopyResult(255, 0, 0, 255);
    verifyResults(texture, expectedCopyResult, kSize, 0, 0, kSize, kSize);

    ASSERT_GL_NO_ERROR();

    glDeleteProgram(drawColor);
}

// BlitFramebuffer functionality test with mixed color attachments where multisampled render to
// texture as attachment 1 and is the read buffer.  This test makes sure the fact that attachment 0
// is a true multisampled texture doesn't cause issues.
// Uses EXT_multisampled_render_to_texture2.
TEST_P(MultisampledRenderToTextureES31Test, BlitFramebufferAttachment1)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    constexpr GLsizei kSize = 16;

    // Create multisampled framebuffer to draw into, use color attachment 1
    GLTexture colorMS0;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorMS0);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, kSize, kSize, true);

    GLTexture colorMS1;
    glBindTexture(GL_TEXTURE_2D, colorMS1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           colorMS0, 0);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
                                         colorMS1, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Setup program to render into attachments 0 and 1.
    constexpr bool kBuffersEnabled[8] = {true, true};

    GLuint drawColor;
    setupUniformColorProgramMultiRenderTarget(kBuffersEnabled, &drawColor);
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    constexpr GLenum kDrawBuffers[] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    glDrawBuffers(2, kDrawBuffers);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffers.
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Create single sampled framebuffer to use as dest.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    GLTexture colorSS;
    glBindTexture(GL_TEXTURE_2D, colorSS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorSS, 0);
    ASSERT_GL_NO_ERROR();

    // Bind MS to READ as SS is already bound to DRAW.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMS);
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Bind SS to READ so we can readPixels from it
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);
    ASSERT_GL_NO_ERROR();
}

// BlitFramebuffer functionality test with mixed multisampled-render-to-texture color attachment and
// multisampled depth buffer.  This test makes sure that the color attachment is blitted, while
// the depth/stencil attachment is resolved.
TEST_P(MultisampledRenderToTextureES3Test, BlitFramebufferMixedColorAndDepth)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    constexpr GLsizei kSize = 16;

    // Create multisampled framebuffer to use as source.
    GLRenderbuffer depthMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthMS);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT24, kSize, kSize);

    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Clear depth to 0.5 and color to red.
    glClearDepthf(0.5f);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    ASSERT_GL_NO_ERROR();

    // Create single sampled framebuffer to use as dest.
    GLRenderbuffer depthSS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthSS);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kSize, kSize);

    GLTexture colorSS;
    glBindTexture(GL_TEXTURE_2D, colorSS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthSS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorSS, 0);
    ASSERT_GL_NO_ERROR();

    // Bind MS to READ as SS is already bound to DRAW.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboMS);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Bind SS to READ so we can readPixels from it
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::red);
    ASSERT_GL_NO_ERROR();

    // Use a small shader to verify depth.
    ANGLE_GL_PROGRAM(depthTestProgram, essl1_shaders::vs::Passthrough(), essl1_shaders::fs::Blue());
    ANGLE_GL_PROGRAM(depthTestProgramFail, essl1_shaders::vs::Passthrough(),
                     essl1_shaders::fs::Green());
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    drawQuad(depthTestProgram, essl1_shaders::PositionAttrib(), -0.01f);
    drawQuad(depthTestProgramFail, essl1_shaders::PositionAttrib(), 0.01f);
    glDisable(GL_DEPTH_TEST);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);
    ASSERT_GL_NO_ERROR();
}

// Draw non-multisampled, draw multisampled, repeat.  This tests the same texture being bound
// differently to two FBOs.
TEST_P(MultisampledRenderToTextureTest, DrawNonMultisampledThenMultisampled)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    // http://anglebug.com/4935
    ANGLE_SKIP_TEST_IF(IsD3D11());

    // Texture attachment to the two framebuffers.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Create singlesampled framebuffer.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    ASSERT_GL_NO_ERROR();

    // Create multisampled framebuffer.
    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);
    ASSERT_GL_NO_ERROR();

    // Draw red into the multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw green into the singlesampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glEnable(GL_SCISSOR_TEST);
    glScissor(kSize / 8, kSize / 8, 3 * kSize / 4, 3 * kSize / 4);
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw blue into the multisampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glScissor(kSize / 4, kSize / 4, kSize / 2, kSize / 2);
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is red on the border, blue in the middle and green in between.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboSS);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 13 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 13 * kSize / 16, GLColor::green);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);

    ASSERT_GL_NO_ERROR();
}

// Draw multisampled, draw multisampled with another sample count, repeat.  This tests the same
// texture being bound as multisampled-render-to-texture with different sample counts to two FBOs.
TEST_P(MultisampledRenderToTextureTest, DrawMultisampledDifferentSamples)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));
    constexpr GLsizei kSize = 64;

    GLsizei maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    ASSERT_GE(maxSamples, 4);

    // Texture attachment to the two framebuffers.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Create two multisampled framebuffers.
    GLFramebuffer fboMS1;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS1);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, 4);

    GLFramebuffer fboMS2;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS2);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color,
                                         0, maxSamples);
    ASSERT_GL_NO_ERROR();

    // Draw red into the first multisampled color buffer.
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw green into the second multisampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS1);
    glEnable(GL_SCISSOR_TEST);
    glScissor(kSize / 8, kSize / 8, 3 * kSize / 4, 3 * kSize / 4);
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Draw blue into the first multisampled color buffer
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS2);
    glScissor(kSize / 4, kSize / 4, kSize / 2, kSize / 2);
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Verify that the texture is red on the border, blue in the middle and green in between.
    GLFramebuffer fboSS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboSS);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize - 1, kSize - 1, GLColor::red);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 3 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 16, 13 * kSize / 16, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(13 * kSize / 16, 13 * kSize / 16, GLColor::green);

    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 3 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(3 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(5 * kSize / 8, 5 * kSize / 8, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::blue);

    ASSERT_GL_NO_ERROR();
}

ANGLE_INSTANTIATE_TEST_ES2_AND_ES3(MultisampledRenderToTextureTest);
ANGLE_INSTANTIATE_TEST_ES3(MultisampledRenderToTextureES3Test);
ANGLE_INSTANTIATE_TEST_ES31(MultisampledRenderToTextureES31Test);
}  // namespace
