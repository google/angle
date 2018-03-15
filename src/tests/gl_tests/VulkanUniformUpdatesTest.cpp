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
};

// This test updates a uniform until a new buffer is allocated and then make sure the uniform
// updates still work.
TEST_P(VulkanUniformUpdatesTest, UpdateUniformUntilNewBufferIsAllocated)
{
    ASSERT_TRUE(IsVulkan());

    constexpr char kPositionUniformVertexShader[] = R"(
precision mediump float;
attribute vec2 position;
uniform vec2 uniPosModifier;
void main()
{
    gl_Position = vec4(position + uniPosModifier, 0, 1);
})";

    constexpr char kColorUniformFragmentShader[] = R"(
precision mediump float;
uniform vec4 uniColor;
void main()
{
    gl_FragColor = uniColor;
})";

    // Hack the angle!
    const gl::Context *context = reinterpret_cast<gl::Context *>(getEGLWindow()->getContext());
    auto *contextVk            = rx::GetImplAs<rx::ContextVk>(context);

    ANGLE_GL_PROGRAM(program, kPositionUniformVertexShader, kColorUniformFragmentShader);
    glUseProgram(program);

    const gl::State &state   = contextVk->getGLState();
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

ANGLE_INSTANTIATE_TEST(VulkanUniformUpdatesTest, ES2_VULKAN());

}  // anonymous namespace
