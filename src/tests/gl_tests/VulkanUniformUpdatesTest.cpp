//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VulkanUniformUpdatesTest:
//   Tests to validate our Vulkan dynamic uniform updates are working as expected.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/angle_test_instantiate.h"
// 'None' is defined as 'struct None {};' in
// third_party/googletest/src/googletest/include/gtest/internal/gtest-type-util.h.
// But 'None' is also defined as a numeric constant 0L in <X11/X.h>.
// So we need to include ANGLETest.h first to avoid this conflict.

#include "libANGLE/Context.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/ProgramVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

class VulkanUniformUpdatesTest : public ANGLETest
{
  protected:
    rx::ContextVk *hackANGLE()
    {
        // Hack the angle!
        const gl::Context *context = reinterpret_cast<gl::Context *>(getEGLWindow()->getContext());
        return rx::GetImplAs<rx::ContextVk>(context);
    }
};

// This test updates a uniform until a new buffer is allocated and then make sure the uniform
// updates still work.
TEST_P(VulkanUniformUpdatesTest, UpdateUntilNewBufferIsAllocated)
{
    ASSERT_TRUE(IsVulkan());

    constexpr char kPositionUniformVertexShader[] = R"(attribute vec2 position;
uniform vec2 uniPosModifier;
void main()
{
    gl_Position = vec4(position + uniPosModifier, 0, 1);
})";

    constexpr char kColorUniformFragmentShader[] = R"(precision mediump float;
uniform vec4 uniColor;
void main()
{
    gl_FragColor = uniColor;
})";

    ANGLE_GL_PROGRAM(program, kPositionUniformVertexShader, kColorUniformFragmentShader);
    glUseProgram(program);

    const gl::State &state   = hackANGLE()->getGLState();
    rx::ProgramVk *programVk = rx::vk::GetImpl(state.getProgram());

    // Set a really small min size so that uniform updates often allocates a new buffer.
    programVk->setDefaultUniformBlocksMinSizeForTesting(128);

    GLint posUniformLocation = glGetUniformLocation(program, "uniPosModifier");
    ASSERT_NE(posUniformLocation, -1);
    GLint colorUniformLocation = glGetUniformLocation(program, "uniColor");
    ASSERT_NE(colorUniformLocation, -1);

    // Sets both uniforms 10 times, it should certainly trigger new buffers creations by the
    // underlying StreamingBuffer.
    for (int i = 0; i < 100; i++)
    {
        glUniform2f(posUniformLocation, -0.5, 0.0);
        glUniform4f(colorUniformLocation, 1.0, 0.0, 0.0, 1.0);
        drawQuad(program, "position", 0.5f, 1.0f);
        swapBuffers();
        ASSERT_GL_NO_ERROR();
    }
}

// Force uniform updates until the dynamic descriptor pool wraps into a new pool allocation.
TEST_P(VulkanUniformUpdatesTest, DescriptorPoolUpdates)
{
    ASSERT_TRUE(IsVulkan());

    // Force a small limit on the max sets per pool to more easily trigger a new allocation.
    constexpr uint32_t kMaxSetsForTesting                = 32;
    rx::vk::DynamicDescriptorPool *dynamicDescriptorPool = hackANGLE()->getDynamicDescriptorPool();
    dynamicDescriptorPool->setMaxSetsPerPoolForTesting(kMaxSetsForTesting);

    // Initialize texture program.
    GLuint program = get2DTexturedQuadProgram();
    ASSERT_NE(0u, program);
    glUseProgram(program);

    GLint texLoc = glGetUniformLocation(program, "tex");
    ASSERT_NE(-1, texLoc);

    // Initialize basic red texture.
    const std::vector<GLColor> redColors(4, GLColor::red);
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, redColors.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    // Draw multiple times, each iteration will create a new descriptor set.
    for (uint32_t iteration = 0; iteration < kMaxSetsForTesting * 8; ++iteration)
    {
        glUniform1i(texLoc, 0);
        drawQuad(program, "position", 0.5f, 1.0f, true);
        swapBuffers();
        ASSERT_GL_NO_ERROR();
    }
}

ANGLE_INSTANTIATE_TEST(VulkanUniformUpdatesTest, ES2_VULKAN());

}  // anonymous namespace
