//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BindGeneratesResourceTest.cpp : Tests of the GL_CHROMIUM_bind_generates_resource extension.

#include "test_utils/ANGLETest.h"

namespace angle
{
class ContextLostTest : public ANGLETest
{
  protected:
    ContextLostTest() { setContextResetStrategy(EGL_LOSE_CONTEXT_ON_RESET_EXT); }
};

// GL_CHROMIUM_lose_context is implemented in the frontend
TEST_P(ContextLostTest, ExtensionStringExposed)
{
    EXPECT_TRUE(ensureExtensionEnabled("GL_CHROMIUM_lose_context"));
}

// Use GL_CHROMIUM_lose_context to lose a context and verify
TEST_P(ContextLostTest, BasicUsage)
{
    ANGLE_SKIP_TEST_IF(!ensureExtensionEnabled("GL_CHROMIUM_lose_context"));
    ANGLE_SKIP_TEST_IF(!ensureExtensionEnabled("GL_EXT_robustness"));

    glLoseContextCHROMIUM(GL_GUILTY_CONTEXT_RESET, GL_INNOCENT_CONTEXT_RESET);
    EXPECT_GL_NO_ERROR();
    EXPECT_GLENUM_EQ(glGetGraphicsResetStatusEXT(), GL_GUILTY_CONTEXT_RESET);

    glBindTexture(GL_TEXTURE_2D, 0);
    EXPECT_GL_ERROR(GL_OUT_OF_MEMORY);
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST(ContextLostTest,
                       ES2_NULL(),
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_VULKAN(),
                       ES3_VULKAN());

}  // namespace angle
