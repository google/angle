//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VulkanPerformanceCounterTest:
//   Validates specific GL call patterns with ANGLE performance counters.
//   For example we can verify a certain call set doesn't break the RenderPass.
//
// TODO(jmadill): Move to a GL extension. http://anglebug.com/4918

#include "test_utils/ANGLETest.h"
#include "test_utils/angle_test_instantiate.h"
// 'None' is defined as 'struct None {};' in
// third_party/googletest/src/googletest/include/gtest/internal/gtest-type-util.h.
// But 'None' is also defined as a numeric constant 0L in <X11/X.h>.
// So we need to include ANGLETest.h first to avoid this conflict.

#include "libANGLE/Context.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{
class VulkanPerformanceCounterTest : public ANGLETest
{
  protected:
    const rx::vk::PerfCounters &hackANGLE() const
    {
        // Hack the angle!
        const gl::Context *context = static_cast<gl::Context *>(getEGLWindow()->getContext());
        return rx::GetImplAs<rx::ContextVk>(context)->getPerfCounters();
    }
};

// Tests that texture updates to unused textures don't break the RP.
TEST_P(VulkanPerformanceCounterTest, NewTextureDoesNotBreakRenderPass)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    GLColor kInitialData[4] = {GLColor::red, GLColor::blue, GLColor::green, GLColor::yellow};

    // Step 1: Set up a simple 2D Texture rendering loop.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, kInitialData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    auto quadVerts = GetQuadVertices();

    GLBuffer vertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, quadVerts.size() * sizeof(quadVerts[0]), quadVerts.data(),
                 GL_STATIC_DRAW);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Texture2D(), essl1_shaders::fs::Texture2D());
    glUseProgram(program);

    GLint posLoc = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, posLoc);

    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(posLoc);
    ASSERT_GL_NO_ERROR();

    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();
    uint32_t expectedRenderPassCount = counters.renderPasses;

    // Step 2: Introduce a new 2D Texture with the same Program and Framebuffer.
    GLTexture newTexture;
    glBindTexture(GL_TEXTURE_2D, newTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, kInitialData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    uint32_t actualRenderPassCount = counters.renderPasses;
    EXPECT_EQ(expectedRenderPassCount, actualRenderPassCount);
}

// Tests that changing a Texture's max level hits the descriptor set cache.
TEST_P(VulkanPerformanceCounterTest, ChangingMaxLevelHitsDescriptorCache)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    GLColor kInitialData[4] = {GLColor::red, GLColor::blue, GLColor::green, GLColor::yellow};

    // Step 1: Set up a simple mipped 2D Texture rendering loop.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, kInitialData);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, kInitialData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);

    auto quadVerts = GetQuadVertices();

    GLBuffer vertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, quadVerts.size() * sizeof(quadVerts[0]), quadVerts.data(),
                 GL_STATIC_DRAW);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Texture2D(), essl1_shaders::fs::Texture2D());
    glUseProgram(program);

    GLint posLoc = glGetAttribLocation(program, essl1_shaders::PositionAttrib());
    ASSERT_NE(-1, posLoc);

    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(posLoc);
    ASSERT_GL_NO_ERROR();

    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Step 2: Change max level and draw.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    uint32_t expectedWriteDescriptorSetCount = counters.writeDescriptorSets;

    // Step 3: Change max level back to original value and verify we hit the cache.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    uint32_t actualWriteDescriptorSetCount = counters.writeDescriptorSets;
    EXPECT_EQ(expectedWriteDescriptorSetCount, actualWriteDescriptorSetCount);
}

// Tests that two glCopyBufferSubData commands can share a barrier.
TEST_P(VulkanPerformanceCounterTest, IndependentBufferCopiesShareSingleBarrier)
{
    constexpr GLint srcDataA[] = {1, 2, 3, 4};
    constexpr GLint srcDataB[] = {5, 6, 7, 8};

    // Step 1: Set up four buffers for two copies.
    GLBuffer srcA;
    glBindBuffer(GL_COPY_READ_BUFFER, srcA);
    glBufferData(GL_COPY_READ_BUFFER, sizeof(srcDataA), srcDataA, GL_STATIC_COPY);

    GLBuffer dstA;
    glBindBuffer(GL_COPY_WRITE_BUFFER, dstA);
    glBufferData(GL_COPY_WRITE_BUFFER, sizeof(srcDataA[0]) * 2, nullptr, GL_STATIC_COPY);

    GLBuffer srcB;
    glBindBuffer(GL_COPY_READ_BUFFER, srcB);
    glBufferData(GL_COPY_READ_BUFFER, sizeof(srcDataB), srcDataB, GL_STATIC_COPY);

    GLBuffer dstB;
    glBindBuffer(GL_COPY_WRITE_BUFFER, dstB);
    glBufferData(GL_COPY_WRITE_BUFFER, sizeof(srcDataB[0]) * 2, nullptr, GL_STATIC_COPY);

    // We expect that ANGLE generate zero additional command buffers.
    const rx::vk::PerfCounters &counters = hackANGLE();
    uint32_t expectedFlushCount          = counters.flushedOutsideRenderPassCommandBuffers;

    // Step 2: Do the two copies.
    glBindBuffer(GL_COPY_READ_BUFFER, srcA);
    glBindBuffer(GL_COPY_WRITE_BUFFER, dstA);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, sizeof(srcDataB[0]), 0,
                        sizeof(srcDataA[0]) * 2);

    glBindBuffer(GL_COPY_READ_BUFFER, srcB);
    glBindBuffer(GL_COPY_WRITE_BUFFER, dstB);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, sizeof(srcDataB[0]), 0,
                        sizeof(srcDataB[0]) * 2);

    ASSERT_GL_NO_ERROR();

    uint32_t actualFlushCount = counters.flushedOutsideRenderPassCommandBuffers;
    EXPECT_EQ(expectedFlushCount, actualFlushCount);
}

ANGLE_INSTANTIATE_TEST(VulkanPerformanceCounterTest, ES3_VULKAN());

}  // anonymous namespace
