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
    VulkanPerformanceCounterTest()
    {
        // Depth required for SwapShouldInvalidateDepthAfterClear.
        // Also RGBA8 is required to avoid the clear for emulated alpha.
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    const rx::vk::PerfCounters &hackANGLE() const
    {
        // Hack the angle!
        const gl::Context *context = static_cast<const gl::Context *>(getEGLWindow()->getContext());
        return rx::GetImplAs<const rx::ContextVk>(context)->getPerfCounters();
    }

    void setupClearAndDrawForInvalidateTest(GLProgram *program,
                                            GLFramebuffer *framebuffer,
                                            GLTexture *texture,
                                            GLRenderbuffer *renderbuffer)
    {
        glUseProgram(*program);

        // Setup to draw to color, depth, and stencil
        glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
        glBindTexture(GL_TEXTURE_2D, *texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, *renderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 16, 16);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                  *renderbuffer);
        ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

        // Clear and draw with depth and stencil buffer enabled
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_GEQUAL);
        glClearDepthf(0.99f);
        glEnable(GL_STENCIL_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawQuad(*program, essl1_shaders::PositionAttrib(), 0.5f);
        ASSERT_GL_NO_ERROR();
    }

    void setExpectedCountersForInvalidateTest(const rx::vk::PerfCounters &counters,
                                              uint32_t incrementalRenderPasses,
                                              uint32_t incrementalDepthClears,
                                              uint32_t incrementalDepthLoads,
                                              uint32_t incrementalDepthStores,
                                              uint32_t incrementalStencilClears,
                                              uint32_t incrementalStencilLoads,
                                              uint32_t incrementalStencilStores,
                                              rx::vk::PerfCounters *expected)
    {
        expected->renderPasses  = counters.renderPasses + incrementalRenderPasses;
        expected->depthClears   = counters.depthClears + incrementalDepthClears;
        expected->depthLoads    = counters.depthLoads + incrementalDepthLoads;
        expected->depthStores   = counters.depthStores + incrementalDepthStores;
        expected->stencilClears = counters.stencilClears + incrementalStencilClears;
        expected->stencilLoads  = counters.stencilLoads + incrementalStencilLoads;
        expected->stencilStores = counters.stencilStores + incrementalStencilStores;
    }

    void setAndIncrementLoadCountersForInvalidateTest(const rx::vk::PerfCounters &counters,
                                                      uint32_t incrementalDepthLoads,
                                                      uint32_t incrementalStencilLoads,
                                                      rx::vk::PerfCounters *expected)
    {
        expected->depthLoads   = counters.depthLoads + incrementalDepthLoads;
        expected->stencilLoads = counters.stencilLoads + incrementalStencilLoads;
    }

    void compareDepthStencilCountersForInvalidateTest(const rx::vk::PerfCounters &counters,
                                                      const rx::vk::PerfCounters &expected)
    {
        EXPECT_EQ(expected.depthClears, counters.depthClears);
        EXPECT_EQ(expected.depthLoads, counters.depthLoads);
        EXPECT_EQ(expected.depthStores, counters.depthStores);
        EXPECT_EQ(expected.stencilClears, counters.stencilClears);
        EXPECT_EQ(expected.stencilLoads, counters.stencilLoads);
        EXPECT_EQ(expected.stencilStores, counters.stencilStores);
    }

    void compareLoadCountersForInvalidateTest(const rx::vk::PerfCounters &counters,
                                              const rx::vk::PerfCounters &expected)
    {
        EXPECT_EQ(expected.depthLoads, counters.depthLoads);
        EXPECT_EQ(expected.stencilLoads, counters.stencilLoads);
    }
};

class VulkanPerformanceCounterTest_ES31 : public VulkanPerformanceCounterTest
{};

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

// Tests that RGB texture should not break renderpass.
TEST_P(VulkanPerformanceCounterTest, SampleFromRGBTextureDoesNotBreakRenderPass)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Texture2D(), essl1_shaders::fs::Texture2D());
    glUseProgram(program);
    GLint textureLoc = glGetUniformLocation(program, essl1_shaders::Texture2DUniform());
    ASSERT_NE(-1, textureLoc);

    GLTexture textureRGBA;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureRGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLTexture textureRGB;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureRGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    uint32_t expectedRenderPassCount = counters.renderPasses + 1;

    // First draw with textureRGBA which should start the renderpass
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniform1i(textureLoc, 0);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Next draw with textureRGB which should not end the renderpass
    glUniform1i(textureLoc, 1);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    uint32_t actualRenderPassCount = counters.renderPasses;
    EXPECT_EQ(expectedRenderPassCount, actualRenderPassCount);
}

// Tests that RGB texture should not break renderpass.
TEST_P(VulkanPerformanceCounterTest, renderToRGBTextureDoesNotBreakRenderPass)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Passthrough(), essl1_shaders::fs::UniformColor());
    glUseProgram(program);
    GLint colorUniformLocation =
        glGetUniformLocation(program, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(-1, colorUniformLocation);
    ASSERT_GL_NO_ERROR();

    GLTexture textureRGB;
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureRGB);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureRGB, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    ASSERT_GL_NO_ERROR();

    uint32_t expectedRenderPassCount = counters.renderPasses + 1;

    // Draw into FBO
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);  // clear to green
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, 256, 256);
    glUniform4fv(colorUniformLocation, 1, GLColor::blue.toNormalizedVector().data());
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);

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

// Test resolving a multisampled texture with blit doesn't break the render pass so a subpass can be
// used
TEST_P(VulkanPerformanceCounterTest_ES31, MultisampleResolveWithBlit)
{
    constexpr int kSize = 16;
    glViewport(0, 0, kSize, kSize);

    GLFramebuffer msaaFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO.get());

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture.get());
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, kSize, kSize, false);
    ASSERT_GL_NO_ERROR();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           texture.get(), 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    ANGLE_GL_PROGRAM(gradientProgram, essl31_shaders::vs::Passthrough(),
                     essl31_shaders::fs::RedGreenGradient());
    drawQuad(gradientProgram, essl31_shaders::PositionAttrib(), 0.5f, 1.0f, true);
    ASSERT_GL_NO_ERROR();

    // Create another FBO to resolve the multisample buffer into.
    GLTexture resolveTexture;
    GLFramebuffer resolveFBO;
    glBindTexture(GL_TEXTURE_2D, resolveTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveTexture, 0);
    EXPECT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));

    glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
    glBlitFramebuffer(0, 0, kSize, kSize, 0, 0, kSize, kSize, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    ASSERT_GL_NO_ERROR();

    const rx::vk::PerfCounters &counters = hackANGLE();
    EXPECT_EQ(counters.resolveImageCommands, 0u);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, resolveFBO);
    EXPECT_PIXEL_NEAR(0, 0, 0, 0, 0, 255, 1.0);                      // Black
    EXPECT_PIXEL_NEAR(kSize - 1, 1, 239, 0, 0, 255, 1.0);            // Red
    EXPECT_PIXEL_NEAR(0, kSize - 1, 0, 239, 0, 255, 1.0);            // Green
    EXPECT_PIXEL_NEAR(kSize - 1, kSize - 1, 239, 239, 0, 255, 1.0);  // Yellow
}

// Ensures a read-only depth-stencil feedback loop works in a single RenderPass.
TEST_P(VulkanPerformanceCounterTest, ReadOnlyDepthStencilFeedbackLoopUsesSingleRenderPass)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    constexpr GLsizei kSize = 4;

    ANGLE_GL_PROGRAM(redProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    ANGLE_GL_PROGRAM(texProgram, essl1_shaders::vs::Texture2D(), essl1_shaders::fs::Texture2D());

    GLTexture colorTexture;
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    setupQuadVertexBuffer(0.5f, 1.0f);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // Set up a depth texture and fill it with an arbitrary initial value.
    GLTexture depthTexture;
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, kSize, kSize, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLFramebuffer depthAndColorFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, depthAndColorFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    GLFramebuffer depthOnlyFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Draw to a first FBO to initialize the depth buffer.
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFBO);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(redProgram);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    uint32_t expectedRenderPassCount = counters.renderPasses + 1;

    // Start new RenderPass with depth write disabled and no loop.
    glBindFramebuffer(GL_FRAMEBUFFER, depthAndColorFBO);
    glDepthMask(false);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Now set up the read-only feedback loop.
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glUseProgram(texProgram);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Tweak the bits to keep it read-only.
    glEnable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Render with just the depth attachment.
    glUseProgram(redProgram);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Rebind the depth texture.
    glUseProgram(texProgram);
    glDepthMask(GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    uint32_t actualRenderPassCount = counters.renderPasses;
    EXPECT_EQ(expectedRenderPassCount, actualRenderPassCount);

    // Do a final write to depth to make sure we can switch out of read-only mode.
    glBindTexture(GL_TEXTURE_2D, 0);
    glDepthMask(GL_TRUE);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();
}

// Tests that common PUBG MOBILE case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, disable, draw
TEST_P(VulkanPerformanceCounterTest, InvalidateDisableDraw)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+0), stencil(Clears+0, Load+1, Stores+0)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 0, 0, 1, 0, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    // Draw (since disabled, shouldn't change result)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 0, 0, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that alternative PUBG MOBILE case does not break render pass, and that counts are correct:
//
// - Scenario: disable, invalidate, draw
TEST_P(VulkanPerformanceCounterTest, DisableInvalidateDraw)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+0), stencil(Clears+0, Load+1, Stores+0)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 0, 0, 1, 0, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    // Invalidate (storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Draw (since disabled, shouldn't change result)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 0, 0, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: disable, draw, invalidate, enable
TEST_P(VulkanPerformanceCounterTest, DisableDrawInvalidateEnable)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+0), stencil(Clears+0, Load+1, Stores+0)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 0, 0, 1, 0, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Note: setupClearAndDrawForInvalidateTest() did an enable and draw

    // Disable (since not invalidated, shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    // Draw (since not invalidated, shouldn't change result)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Enable (shouldn't change result)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    // Note: The above enable calls will be ignored, since no drawing was done to force the enable
    // dirty bit to be processed

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Break the render pass by reading back a pixel.
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 0, 0, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that common TRex case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate
TEST_P(VulkanPerformanceCounterTest, Invalidate)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+0), stencil(Clears+0, Load+1, Stores+0)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 0, 0, 1, 0, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 0, 0, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, draw
TEST_P(VulkanPerformanceCounterTest, InvalidateDraw)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 1, 0, 1, 1, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    // TODO: Fix ANGLE to correct set mContentDefined for this scenario.  At this point,
    // mContentDefined will remain false since we don't do record anything at draw-time, and since
    // we don't set mContentDefined at endRP().
    // https://issuetracker.google.com/issues/167275320

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 1, 1, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    // TODO: After fixing ANGLE per https://issuetracker.google.com/issues/167275320, uncomment:
    // compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, draw, disable
TEST_P(VulkanPerformanceCounterTest, InvalidateDrawDisable)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 1, 0, 1, 1, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    // Note: this draw is just so that the disable dirty bits will be processed
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 1, 1, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, disable, draw, enable
TEST_P(VulkanPerformanceCounterTest, InvalidateDisableDrawEnable)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+0), stencil(Clears+0, Load+1, Stores+0)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 0, 0, 1, 0, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    // Draw (since disabled, shouldn't change result)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Enable (shouldn't change result)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    // Note: The above enable calls will be ignored, since no drawing was done to force the enable
    // dirty bit to be processed

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 0, 0, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, disable, draw, enable, draw
TEST_P(VulkanPerformanceCounterTest, InvalidateDisableDrawEnableDraw)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 1, 0, 1, 1, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    // Draw (since disabled, shouldn't change result)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Enable (shouldn't change result)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 1, 1, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, draw, disable, enable
TEST_P(VulkanPerformanceCounterTest, InvalidateDrawDisableEnable)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 1, 0, 1, 1, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    // Note: this draw is just so that the disable dirty bits will be processed
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Enable (shouldn't change result)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    // Note: The above enable calls will be ignored, since no drawing was done to force the enable
    // dirty bit to be processed

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 1, 1, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, draw, disable, enable, invalidate
TEST_P(VulkanPerformanceCounterTest, InvalidateDrawDisableEnableInvalidate)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+0), stencil(Clears+0, Load+1, Stores+0)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 0, 0, 1, 0, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    // Note: this draw is just so that the disable dirty bits will be processed
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Enable (shouldn't change result)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 0, 0, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, draw, disable, enable, invalidate, draw
TEST_P(VulkanPerformanceCounterTest, InvalidateDrawDisableEnableInvalidateDraw)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 1, 0, 1, 1, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    // Note: this draw is just so that the disable dirty bits will be processed
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Enable (shouldn't change result)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 1, 1, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that another common (dEQP) case does not break render pass, and that counts are correct:
//
// - Scenario: invalidate, disable, enable, draw
TEST_P(VulkanPerformanceCounterTest, InvalidateDisableEnableDraw)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 1, 0, 1, 1, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    GLRenderbuffer renderbuffer;
    setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

    // Execute the scenario that this test is for:

    // Invalidate (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Disable (shouldn't change result)
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    // Note: this draw is just so that the disable dirty bits will be processed
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Enable (shouldn't change result)
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Ensure that the render pass wasn't broken
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 1, 1, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests whether depth-stencil ContentDefined will be correct when:
//
// - Scenario: invalidate, detach D/S texture and modify it, attach D/S texture, draw with blend
TEST_P(VulkanPerformanceCounterTest, InvalidateDetachModifyTexAttachDrawWithBlend)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 0, 0, 1, 0, &expected);

    ANGLE_GL_PROGRAM(redProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    ANGLE_GL_PROGRAM(greenProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLTexture colorTexture;
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

    GLTexture depthStencilTexture;
    glBindTexture(GL_TEXTURE_2D, depthStencilTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 2, 2, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                           depthStencilTexture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Clear and draw with depth-stencil enabled
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glClearDepthf(0.99f);
    glEnable(GL_STENCIL_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawQuad(redProgram, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Invalidate depth & stencil (should result: in storeOp = DONT_CARE; mContentDefined = false)
    const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
    ASSERT_GL_NO_ERROR();

    // Check for the expected number of render passes, expected color, and other expected counters
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Detach depth-stencil attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Modify depth-stencil
    constexpr uint32_t kDepthStencilInitialValue = 0xafffff00;
    uint32_t depthStencilData[4] = {kDepthStencilInitialValue, kDepthStencilInitialValue,
                                    kDepthStencilInitialValue, kDepthStencilInitialValue};
    glBindTexture(GL_TEXTURE_2D, depthStencilTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 2, 2, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8, depthStencilData);

    // Re-attach depth-stencil
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                           depthStencilTexture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Draw again, showing that the modified depth-stencil value prevents a new color value
    //
    // Expect rpCount+1, depth(Clears+0, Loads+1, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 0, 1, 1, 0, 1, 1, &expected);
    drawQuad(greenProgram, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    // Check for the expected number of render passes, expected color, and other expected counters
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Draw again, using a different depth value, so that the drawing takes place
    //
    // Expect rpCount+1, depth(Clears+0, Loads+1, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 0, 1, 1, 0, 1, 1, &expected);
    drawQuad(greenProgram, essl1_shaders::PositionAttrib(), 0.2f);
    ASSERT_GL_NO_ERROR();
    // Check for the expected number of render passes, expected color, and other expected counters
    EXPECT_EQ(expected.renderPasses, counters.renderPasses);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);
    compareDepthStencilCountersForInvalidateTest(counters, expected);
}

// Tests that a GLRenderbuffer can be deleted before the render pass ends, and that everything
// still works.
//
// - Scenario: invalidate
TEST_P(VulkanPerformanceCounterTest, InvalidateDrawAndDeleteRenderbuffer)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    rx::vk::PerfCounters expected;

    // Expect rpCount+1, depth(Clears+1, Loads+0, Stores+1), stencil(Clears+0, Load+1, Stores+1)
    setExpectedCountersForInvalidateTest(counters, 1, 1, 0, 1, 0, 1, 1, &expected);

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    GLFramebuffer framebuffer;
    GLTexture texture;
    {
        // Declare the RAII-based GLRenderbuffer object within this set of curly braces, so that it
        // will be deleted early (at the close-curly-brace)
        GLRenderbuffer renderbuffer;
        setupClearAndDrawForInvalidateTest(&program, &framebuffer, &texture, &renderbuffer);

        // Invalidate (storeOp = DONT_CARE; mContentDefined = false)
        const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
        glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, discards);
        ASSERT_GL_NO_ERROR();

        // Draw (since enabled, should result: in storeOp = STORE; mContentDefined = true)
        drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
        ASSERT_GL_NO_ERROR();

        // Ensure that the render pass wasn't broken
        EXPECT_EQ(expected.renderPasses, counters.renderPasses);
    }

    // The renderbuffer should now be deleted.

    // Use swapBuffers and then check how many loads and stores were actually done
    swapBuffers();
    compareDepthStencilCountersForInvalidateTest(counters, expected);

    // Start and end another render pass, to check that the load ops are as expected
    setAndIncrementLoadCountersForInvalidateTest(counters, 0, 0, &expected);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    swapBuffers();
    compareLoadCountersForInvalidateTest(counters, expected);
}

// Tests that even if the app clears depth, it should be invalidated if there is no read.
TEST_P(VulkanPerformanceCounterTest, SwapShouldInvalidateDepthAfterClear)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    ANGLE_GL_PROGRAM(redProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());

    // Clear depth.
    glClear(GL_DEPTH_BUFFER_BIT);

    // Ensure we never read from depth.
    glDisable(GL_DEPTH_TEST);

    // Do one draw, then swap.
    drawQuad(redProgram, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    uint32_t expectedDepthClears = counters.depthClears;

    swapBuffers();

    uint32_t actualDepthClears = counters.depthClears;
    EXPECT_EQ(expectedDepthClears, actualDepthClears);
}

// Tests that masked color clears don't break the RP.
TEST_P(VulkanPerformanceCounterTest, MaskedClearDoesNotBreakRenderPass)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    ASSERT_GL_NO_ERROR();

    uint32_t expectedRenderPassCount = counters.renderPasses + 1;

    // Mask color channels and clear the framebuffer multiple times.
    glClearColor(0.25f, 0.25f, 0.25f, 0.25f);
    glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_FALSE);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);
    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(0.75f, 0.75f, 0.75f, 0.75f);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);

    uint32_t actualRenderPassCount = counters.renderPasses;
    EXPECT_EQ(expectedRenderPassCount, actualRenderPassCount);

    EXPECT_PIXEL_NEAR(0, 0, 63, 127, 255, 191, 1);
}

// Tests that draw buffer change with all color channel mask off should not break renderpass
TEST_P(VulkanPerformanceCounterTest, DrawbufferChangeWithAllColorMaskDisabled)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Passthrough(), essl1_shaders::fs::UniformColor());
    glUseProgram(program);
    GLint colorUniformLocation =
        glGetUniformLocation(program, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(-1, colorUniformLocation);
    ASSERT_GL_NO_ERROR();

    GLTexture textureRGBA;
    glBindTexture(GL_TEXTURE_2D, textureRGBA);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    GLTexture textureDepth;
    glBindTexture(GL_TEXTURE_2D, textureDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 64, 64, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureRGBA, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureDepth, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    ASSERT_GL_NO_ERROR();

    uint32_t expectedRenderPassCount = counters.renderPasses + 1;

    // Draw into FBO
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);  // clear to green
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, 256, 256);
    glUniform4fv(colorUniformLocation, 1, GLColor::blue.toNormalizedVector().data());
    GLenum glDrawBuffers_bufs_1[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, glDrawBuffers_bufs_1);
    glEnable(GL_DEPTH_TEST);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    // Change draw buffer state and color mask
    GLenum glDrawBuffers_bufs_0[] = {GL_NONE};
    glDrawBuffers(1, glDrawBuffers_bufs_0);
    glColorMask(false, false, false, false);
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.6f);
    // Change back draw buffer state and color mask
    glDrawBuffers(1, glDrawBuffers_bufs_1);
    glColorMask(true, true, true, true);
    glUniform4fv(colorUniformLocation, 1, GLColor::red.toNormalizedVector().data());
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.7f);

    uint32_t actualRenderPassCount = counters.renderPasses;
    EXPECT_EQ(expectedRenderPassCount, actualRenderPassCount);
}

// Tests the optimization that a glFlush call issued inside a renderpass will be skipped.
TEST_P(VulkanPerformanceCounterTest, InRenderpassFlushShouldNotBreakRenderpass)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    uint32_t expectedRenderPassCount     = counters.renderPasses + 1;

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
    ASSERT_GL_NO_ERROR();

    ANGLE_GL_PROGRAM(redProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(redProgram, essl1_shaders::PositionAttrib(), 0.5f);
    glFlush();
    ANGLE_GL_PROGRAM(greenProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Green());
    drawQuad(greenProgram, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    uint32_t actualRenderPassCount = counters.renderPasses;
    EXPECT_EQ(expectedRenderPassCount, actualRenderPassCount);
}

// Tests that depth/stencil texture clear/load works correctly.
TEST_P(VulkanPerformanceCounterTest, DepthStencilTextureClearAndLoad)
{
    const rx::vk::PerfCounters &counters = hackANGLE();
    uint32_t expectedDepthClearCount     = counters.depthClears + 1;
    uint32_t expectedDepthLoadCount      = counters.depthLoads + 3;
    uint32_t expectedStencilClearCount   = counters.stencilClears + 1;
    uint32_t expectedStencilLoadCount    = counters.stencilLoads + 3;

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    constexpr GLsizei kSize = 6;

    // Create framebuffer to draw into, with both color and depth attachments.
    GLTexture color;
    glBindTexture(GL_TEXTURE_2D, color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLTexture depth;
    glBindTexture(GL_TEXTURE_2D, depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, kSize, kSize, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8_OES, nullptr);

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
    ASSERT_GL_NO_ERROR();

    // Set up texture for copy operation that breaks the render pass
    GLTexture copyTex;
    glBindTexture(GL_TEXTURE_2D, copyTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set viewport and clear depth/stencil
    glViewport(0, 0, kSize, kSize);
    glClearDepthf(1);
    glClearStencil(0x55);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // If depth is not cleared to 1, rendering would fail.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);

    // If stencil is not clear to 0x55, rendering would fail.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw green
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, kSize / 2, 0, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw blue
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, kSize / 2, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw yellow
    glUniform4f(colorUniformLocation, 1.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, kSize / 2, kSize / 2, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Verify the counters
    EXPECT_EQ(counters.depthClears, expectedDepthClearCount);
    EXPECT_EQ(counters.depthLoads, expectedDepthLoadCount);
    EXPECT_EQ(counters.stencilClears, expectedStencilClearCount);
    EXPECT_EQ(counters.stencilLoads, expectedStencilLoadCount);

    // Verify that copies were done correctly.
    GLFramebuffer verifyFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, verifyFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copyTex, 0);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, 0, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(0, kSize / 2, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::yellow);
}

// Tests that multisampled-render-to-texture depth/stencil textures don't ever load data.
TEST_P(VulkanPerformanceCounterTest, RenderToTextureDepthStencilTextureShouldNotLoad)
{
    // http://anglebug.com/5083
    ANGLE_SKIP_TEST_IF(IsWindows() && IsAMD() && IsVulkan());

    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture2"));

    const rx::vk::PerfCounters &counters = hackANGLE();
    uint32_t expectedDepthClearCount     = counters.depthClears + 1;
    uint32_t expectedDepthLoadCount      = counters.depthLoads;
    uint32_t expectedStencilClearCount   = counters.stencilClears + 1;
    uint32_t expectedStencilLoadCount    = counters.stencilLoads;

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    constexpr GLsizei kSize = 6;

    // Create multisampled framebuffer to draw into, with both color and depth attachments.
    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLTexture depthMS;
    glBindTexture(GL_TEXTURE_2D, depthMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, kSize, kSize, 0, GL_DEPTH_STENCIL,
                 GL_UNSIGNED_INT_24_8_OES, nullptr);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                         depthMS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Set up texture for copy operation that breaks the render pass
    GLTexture copyTex;
    glBindTexture(GL_TEXTURE_2D, copyTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set viewport and clear depth
    glViewport(0, 0, kSize, kSize);
    glClearDepthf(1);
    glClearStencil(0x55);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // If depth is not cleared to 1, rendering would fail.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // If stencil is not clear to 0x55, rendering would fail.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw green
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, kSize / 2, 0, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw blue
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, kSize / 2, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw yellow
    glUniform4f(colorUniformLocation, 1.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, kSize / 2, kSize / 2, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Verify the counters
    EXPECT_EQ(counters.depthClears, expectedDepthClearCount);
    EXPECT_EQ(counters.depthLoads, expectedDepthLoadCount);
    EXPECT_EQ(counters.stencilClears, expectedStencilClearCount);
    EXPECT_EQ(counters.stencilLoads, expectedStencilLoadCount);

    // Verify that copies were done correctly.  Only the first copy can be verified because the
    // contents of the depth/stencil buffer is undefined after the first render pass break, meaning
    // it is unknown whether the three subsequent draw calls passed the depth or stencil tests.
    GLFramebuffer verifyFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, verifyFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copyTex, 0);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2 - 1, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(0, kSize / 2 - 1, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2 - 1, kSize / 2 - 1, GLColor::red);
}

// Tests that multisampled-render-to-texture depth/stencil renderbuffers don't ever load depth data.
// Stencil data may still be loaded if VK_EXT_shader_stencil_export is not supported.
TEST_P(VulkanPerformanceCounterTest, RenderToTextureDepthStencilRenderbufferShouldNotLoad)
{
    // http://anglebug.com/5083
    ANGLE_SKIP_TEST_IF(IsWindows() && IsAMD() && IsVulkan());

    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_multisampled_render_to_texture"));

    const rx::vk::PerfCounters &counters = hackANGLE();
    uint32_t expectedDepthClearCount     = counters.depthClears + 1;
    uint32_t expectedDepthLoadCount      = counters.depthLoads;
    uint32_t expectedStencilClearCount   = counters.stencilClears + 1;
    uint32_t expectedStencilLoadCountMin = counters.stencilLoads;
    uint32_t expectedStencilLoadCountMax = counters.stencilLoads + 4;

    GLFramebuffer FBO;
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);

    constexpr GLsizei kSize = 6;

    // Create multisampled framebuffer to draw into, with both color and depth attachments.
    GLTexture colorMS;
    glBindTexture(GL_TEXTURE_2D, colorMS);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    GLRenderbuffer depthStencilMS;
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencilMS);
    glRenderbufferStorageMultisampleEXT(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, kSize, kSize);

    GLFramebuffer fboMS;
    glBindFramebuffer(GL_FRAMEBUFFER, fboMS);
    glFramebufferTexture2DMultisampleEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                         colorMS, 0, 4);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              depthStencilMS);
    ASSERT_GL_NO_ERROR();

    // Set up texture for copy operation that breaks the render pass
    GLTexture copyTex;
    glBindTexture(GL_TEXTURE_2D, copyTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set viewport and clear depth
    glViewport(0, 0, kSize, kSize);
    glClearDepthf(1);
    glClearStencil(0x55);
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // If depth is not cleared to 1, rendering would fail.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // If stencil is not clear to 0x55, rendering would fail.
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 0x55, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0xFF);

    // Set up program
    ANGLE_GL_PROGRAM(drawColor, essl1_shaders::vs::Simple(), essl1_shaders::fs::UniformColor());
    glUseProgram(drawColor);
    GLint colorUniformLocation =
        glGetUniformLocation(drawColor, angle::essl1_shaders::ColorUniform());
    ASSERT_NE(colorUniformLocation, -1);

    // Draw red
    glUniform4f(colorUniformLocation, 1.0f, 0.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.75f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw green
    glUniform4f(colorUniformLocation, 0.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, kSize / 2, 0, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw blue
    glUniform4f(colorUniformLocation, 0.0f, 0.0f, 1.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.25f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, kSize / 2, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Draw yellow
    glUniform4f(colorUniformLocation, 1.0f, 1.0f, 0.0f, 1.0f);
    drawQuad(drawColor, essl1_shaders::PositionAttrib(), 0.0f);
    ASSERT_GL_NO_ERROR();

    // Break the render pass
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, kSize / 2, kSize / 2, 0, 0, kSize / 2, kSize / 2);
    ASSERT_GL_NO_ERROR();

    // Verify the counters
    EXPECT_EQ(counters.depthClears, expectedDepthClearCount);
    EXPECT_EQ(counters.depthLoads, expectedDepthLoadCount);
    EXPECT_EQ(counters.stencilClears, expectedStencilClearCount);
    EXPECT_GE(counters.stencilLoads, expectedStencilLoadCountMin);
    EXPECT_LE(counters.stencilLoads, expectedStencilLoadCountMax);

    // Verify that copies were done correctly.
    GLFramebuffer verifyFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, verifyFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, copyTex, 0);

    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, 0, GLColor::green);
    EXPECT_PIXEL_COLOR_EQ(0, kSize / 2, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(kSize / 2, kSize / 2, GLColor::yellow);
}

// Ensures we use read-only depth layout when there is no write
TEST_P(VulkanPerformanceCounterTest, ReadOnlyDepthBufferLayout)
{
    const rx::vk::PerfCounters &counters = hackANGLE();

    constexpr GLsizei kSize = 64;

    // Create depth only FBO and fill depth texture to leftHalf=0.0 and rightHalf=1.0. This should
    // use writeable layout
    uint32_t expectedReadOnlyDepthStencilCount = counters.readOnlyDepthStencilRenderPasses;
    GLTexture depthTexture;
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, kSize, kSize, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLFramebuffer depthOnlyFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, depthOnlyFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    ANGLE_GL_PROGRAM(redProgram, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    glViewport(0, 0, kSize / 2, kSize);
    drawQuad(redProgram, essl1_shaders::PositionAttrib(), 0.0f);
    glViewport(kSize / 2, 0, kSize / 2, kSize);
    drawQuad(redProgram, essl1_shaders::PositionAttrib(), 1.0f);
    glViewport(0, 0, kSize, kSize);
    ASSERT_GL_NO_ERROR();

    // Because the layout counter is updated at end of renderpass, we need to issue a finish call
    // here to end the renderpass.
    glFinish();

    uint32_t actualReadOnlyDepthStencilCount = counters.readOnlyDepthStencilRenderPasses;
    EXPECT_EQ(expectedReadOnlyDepthStencilCount, actualReadOnlyDepthStencilCount);

    // Create a color+depth FBO and use depth as read only. This should use read only layout
    expectedReadOnlyDepthStencilCount = counters.readOnlyDepthStencilRenderPasses + 1;
    GLTexture colorTexture;
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSize, kSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GLFramebuffer depthAndColorFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, depthAndColorFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);
    ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    // Clear color to blue and draw a green quad with depth=0.5
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);
    GLfloat *clearColor = GLColor::blue.toNormalizedVector().data();
    glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT);
    drawQuad(redProgram, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    // The pixel check will end renderpass.
    EXPECT_PIXEL_COLOR_EQ(1, 1, GLColor::blue);
    EXPECT_PIXEL_COLOR_EQ(1 + kSize / 2, 1, GLColor::red);
    actualReadOnlyDepthStencilCount = counters.readOnlyDepthStencilRenderPasses;
    EXPECT_EQ(expectedReadOnlyDepthStencilCount, actualReadOnlyDepthStencilCount);
}

ANGLE_INSTANTIATE_TEST(VulkanPerformanceCounterTest, ES3_VULKAN());
ANGLE_INSTANTIATE_TEST(VulkanPerformanceCounterTest_ES31, ES31_VULKAN());

}  // anonymous namespace
