//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WebGLFramebufferTest.cpp : Framebuffer tests for GL_ANGLE_webgl_compatibility.
// Based on WebGL 1 test renderbuffers/framebuffer-object-attachment.

#include "test_utils/ANGLETest.h"

#include "test_utils/gl_raii.h"

namespace angle
{

class WebGLFramebufferTest : public ANGLETest
{
  protected:
    WebGLFramebufferTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setWebGLCompatibilityEnabled(true);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();
        glRequestExtensionANGLE = reinterpret_cast<PFNGLREQUESTEXTENSIONANGLEPROC>(
            eglGetProcAddress("glRequestExtensionANGLE"));
    }

    void TearDown() override { ANGLETest::TearDown(); }

    PFNGLREQUESTEXTENSIONANGLEPROC glRequestExtensionANGLE = nullptr;
};

constexpr GLint ALLOW_COMPLETE              = 0x1;
constexpr GLint ALLOW_UNSUPPORTED           = 0x2;
constexpr GLint ALLOW_INCOMPLETE_ATTACHMENT = 0x4;

void checkFramebufferForAllowedStatuses(GLbitfield allowedStatuses)
{
    // If the framebuffer is in an error state for multiple reasons,
    // we can't guarantee which one will be reported.
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    bool statusAllowed =
        ((allowedStatuses & ALLOW_COMPLETE) && (status == GL_FRAMEBUFFER_COMPLETE)) ||
        ((allowedStatuses & ALLOW_UNSUPPORTED) && (status == GL_FRAMEBUFFER_UNSUPPORTED)) ||
        ((allowedStatuses & ALLOW_INCOMPLETE_ATTACHMENT) &&
         (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT));
    EXPECT_TRUE(statusAllowed);
}

void checkBufferBits(GLenum attachment)
{
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;

    bool haveDepthBuffer =
        attachment == GL_DEPTH_ATTACHMENT || attachment == GL_DEPTH_STENCIL_ATTACHMENT;
    bool haveStencilBuffer =
        attachment == GL_STENCIL_ATTACHMENT || attachment == GL_DEPTH_STENCIL_ATTACHMENT;

    GLint redBits     = 0;
    GLint greenBits   = 0;
    GLint blueBits    = 0;
    GLint alphaBits   = 0;
    GLint depthBits   = 0;
    GLint stencilBits = 0;
    glGetIntegerv(GL_RED_BITS, &redBits);
    glGetIntegerv(GL_GREEN_BITS, &greenBits);
    glGetIntegerv(GL_BLUE_BITS, &blueBits);
    glGetIntegerv(GL_ALPHA_BITS, &alphaBits);
    glGetIntegerv(GL_DEPTH_BITS, &depthBits);
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

    EXPECT_GE(redBits + greenBits + blueBits + alphaBits, 16);

    if (haveDepthBuffer)
        EXPECT_GE(depthBits, 16);
    else
        EXPECT_EQ(0, depthBits);

    if (haveStencilBuffer)
        EXPECT_GE(stencilBits, 8);
    else
        EXPECT_EQ(0, stencilBits);
}

// Tests that certain required combinations work in WebGL compatiblity.
TEST_P(WebGLFramebufferTest, TestFramebufferRequiredCombinations)
{
    // Per discussion with the OpenGL ES working group, the following framebuffer attachment
    // combinations are required to work in all WebGL implementations:
    // 1. COLOR_ATTACHMENT0 = RGBA/UNSIGNED_BYTE texture
    // 2. COLOR_ATTACHMENT0 = RGBA/UNSIGNED_BYTE texture + DEPTH_ATTACHMENT = DEPTH_COMPONENT16
    // renderbuffer
    // 3. COLOR_ATTACHMENT0 = RGBA/UNSIGNED_BYTE texture + DEPTH_STENCIL_ATTACHMENT = DEPTH_STENCIL
    // renderbuffer

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    constexpr int width  = 64;
    constexpr int height = 64;

    // 1. COLOR_ATTACHMENT0 = RGBA/UNSIGNED_BYTE texture
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    EXPECT_GL_NO_ERROR();
    checkFramebufferForAllowedStatuses(ALLOW_COMPLETE);
    checkBufferBits(GL_COLOR_ATTACHMENT0);

    // 2. COLOR_ATTACHMENT0 = RGBA/UNSIGNED_BYTE texture + DEPTH_ATTACHMENT = DEPTH_COMPONENT16
    // renderbuffer
    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
    EXPECT_GL_NO_ERROR();
    checkFramebufferForAllowedStatuses(ALLOW_COMPLETE);
    checkBufferBits(GL_DEPTH_ATTACHMENT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);

    // 3. COLOR_ATTACHMENT0 = RGBA/UNSIGNED_BYTE texture + DEPTH_STENCIL_ATTACHMENT = DEPTH_STENCIL
    // renderbuffer
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              renderbuffer);
    EXPECT_GL_NO_ERROR();
    checkFramebufferForAllowedStatuses(ALLOW_COMPLETE);
    checkBufferBits(GL_DEPTH_STENCIL_ATTACHMENT);
}

// Only run against WebGL 1 validation, since much was changed in 2.
ANGLE_INSTANTIATE_TEST(WebGLFramebufferTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES());

}  // namespace
