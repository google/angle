//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VulkanPipelineBinaryTest.cpp:
//   White box tests for pipeline creation through VK_KHR_pipeline_binary: cache hits for draws,
//   dispatches and links, across restarts and global key changes.  The storage format is tested in
//   vk_pipeline_binary_cache_unittest.cpp.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/angle_test_instantiate.h"
#include "test_utils/gl_raii.h"

#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Program.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/vk_pipeline_binary_cache.h"
#include "libANGLE/renderer/vulkan/vk_renderer.h"

#include <sstream>

using namespace angle;

namespace rx
{

class VulkanPipelineBinaryTest : public ANGLETest<>
{
  protected:
    VulkanPipelineBinaryTest()
    {
        // Some tests identify a pipeline by the exact color it draws, which RGB565 would quantize.
        // Without these, Android's EGL config sort puts it first.
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);

        // A program loaded from a program binary isn't warmed up at link time, so whether a link
        // creates pipelines would depend on what the program cache holds
        setContextProgramCacheEnabled(false);

        // Binaries are stored through the platform's blob cache
        setNeedsBlobCache(true);
    }

    void testTearDown() override
    {
        // ANGLE reuses the Display across tests, and the PipelineBinariesCache with it, so each
        // test starts counting from zero
        ContextVk *contextVk = getContextVk();
        if (contextVk != nullptr && contextVk->getRenderer() != nullptr)
        {
            contextVk->getRenderer()->getPipelineBinariesCache()->resetStats();
        }

        ANGLETest::testTearDown();
    }

    ContextVk *getContextVk() const
    {
        egl::Display *display   = static_cast<egl::Display *>(getEGLWindow()->getDisplay());
        gl::ContextID contextID = {
            static_cast<GLuint>(reinterpret_cast<uintptr_t>(getEGLWindow()->getContext()))};
        return GetImplAs<ContextVk>(display->getContextRef(contextID).get());
    }

    // Replaces the Display, Renderer and PipelineBinariesCache with new ones, as an application
    // restart would.  Returns whether the new device reports the same global pipeline key.  If it
    // doesn't, as some drivers do for a later device in the same process, nothing stored earlier is
    // found, and the test has nothing to check.
    [[nodiscard]] bool restartKeepsGlobalPipelineKey()
    {
        const VkPipelineBinaryKeyKHR globalKeyBefore = getGlobalPipelineKey();
        // Without this, recreateTestFixture() keeps the Display, and anything cached in memory
        forceNewDisplay();
        recreateTestFixture();
        return vk::PipelineBinaryKeysEqual(globalKeyBefore, getGlobalPipelineKey());
    }

    VkPipelineBinaryKeyKHR getGlobalPipelineKey() const
    {
        ContextVk *contextVk = getContextVk();
        if (contextVk == nullptr)
        {
            return {};
        }
        return contextVk->getRenderer()->getPipelineBinariesCache()->getGlobalKeyForTesting();
    }

    // Waits for the pipelines a link creates.
    void waitForPostLinkTasks(GLuint program)
    {
        egl::Display *display   = static_cast<egl::Display *>(getEGLWindow()->getDisplay());
        gl::ContextID contextID = {
            static_cast<GLuint>(reinterpret_cast<uintptr_t>(getEGLWindow()->getContext()))};
        egl::ScopedContextRef contextRef = display->getContextRef(contextID);
        gl::Program *programObject =
            contextRef.get()->getProgramResolveLink(gl::ShaderProgramID{program});
        programObject->waitForPostLinkTasks(contextRef.get());
    }

    bool isFeatureEnabled(Feature feature) { return getEGLWindow()->isFeatureEnabled(feature); }
};

// Compute pipelines need ES3.1.
class VulkanPipelineBinaryTestES31 : public VulkanPipelineBinaryTest
{};

// The color the pipeline with this index draws, distinct per pipeline, so that a pipeline created
// from another one's binaries draws the wrong color.
GLColor ExpectedColorForPipeline(size_t index)
{
    const GLubyte red  = static_cast<GLubyte>(index * 25);
    const GLubyte blue = static_cast<GLubyte>(255 - index * 25);
    return GLColor(red, 128, blue, 255);
}

// A fragment shader that writes ExpectedColorForPipeline(index), so each index makes a pipeline
// of its own.
std::string MakeIdentifiableFragmentShader(size_t index)
{
    const GLColor color = ExpectedColorForPipeline(index);

    std::ostringstream fs;
    fs << "precision mediump float;\n"
       << "void main()\n"
       << "{\n"
       << "    gl_FragColor = vec4(" << static_cast<int>(color.R) << ".0 / 255.0,\n"
       << "                        " << static_cast<int>(color.G) << ".0 / 255.0,\n"
       << "                        " << static_cast<int>(color.B) << ".0 / 255.0,\n"
       << "                        1.0);\n"
       << "}\n";
    return fs.str();
}

// Draws with a program, then with a second program of the same shaders and state.  The second
// program's pipeline has the same pipeline key, so it is created from what the first one stored.
TEST_P(VulkanPipelineBinaryTest, WarmDrawHitsPipelineBinaryCache)
{
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));

    ContextVk *contextVk = getContextVk();

    vk::PipelineBinariesCache *cache = contextVk->getRenderer()->getPipelineBinariesCache();
    ASSERT_TRUE(cache->isInitialized());

    cache->resetStats();

    // Nothing is stored yet, so the pipeline is compiled
    ANGLE_GL_PROGRAM(program1, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(program1, essl1_shaders::PositionAttrib(), 0.5f);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_GE(cache->getCacheMisses(), 1u);

    glUseProgram(0);
    program1.reset();
    cache->resetStats();

    // The second program's pipeline is created from the first one's binaries
    ANGLE_GL_PROGRAM(program2, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(program2, essl1_shaders::PositionAttrib(), 0.5f);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    EXPECT_GE(cache->getCacheHits(), 1u);
    EXPECT_EQ(cache->getCacheMisses(), 0u);
}

// Dispatches into the storage buffer at binding 0, and checks that it holds |expectedValue|.
void DispatchAndVerify(GLuint expectedValue)
{
    glDispatchCompute(1, 1, 1);
    ASSERT_GL_NO_ERROR();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    const GLuint *mapped = static_cast<const GLuint *>(
        glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT));
    ASSERT_NE(mapped, nullptr);
    EXPECT_EQ(*mapped, expectedValue);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    ASSERT_GL_NO_ERROR();
}

// The same as WarmDrawHitsPipelineBinaryCache, for a compute pipeline.
TEST_P(VulkanPipelineBinaryTestES31, WarmDispatchHitsPipelineBinaryCache)
{
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));

    ContextVk *contextVk = getContextVk();

    vk::PipelineBinariesCache *cache = contextVk->getRenderer()->getPipelineBinariesCache();
    ASSERT_TRUE(cache->isInitialized());

    constexpr char kCS[] = R"(#version 310 es
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;
layout(std430, binding = 0) buffer Out
{
    uint value;
} outBuffer;
void main()
{
    outBuffer.value = 42u;
})";

    // The shader writes 42 over this, which shows that it ran
    constexpr GLuint kInitialValue = 0u;

    GLBuffer buffer;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(kInitialValue), &kInitialValue, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
    ASSERT_GL_NO_ERROR();

    cache->resetStats();

    // Nothing is stored yet, so the pipeline is compiled
    {
        ANGLE_GL_COMPUTE_PROGRAM(program, kCS);
        glUseProgram(program);
        DispatchAndVerify(42u);
    }
    EXPECT_GE(cache->getCacheMisses(), 1u);

    // Clear the result, so the second dispatch has to write it again
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(kInitialValue), &kInitialValue);
    ASSERT_GL_NO_ERROR();
    cache->resetStats();

    // The second program's pipeline is created from the first one's binaries
    {
        ANGLE_GL_COMPUTE_PROGRAM(program, kCS);
        glUseProgram(program);
        DispatchAndVerify(42u);
    }
    EXPECT_GE(cache->getCacheHits(), 1u);
    EXPECT_EQ(cache->getCacheMisses(), 0u);
}

// Draws, restarts as an application would, and draws again.  Nothing survives the restart but
// what was stored on disk, and the second draw's pipelines are all created from it.
TEST_P(VulkanPipelineBinaryTest, WarmRestartHitsPipelineBinaryCache)
{
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));

    ContextVk *contextVk = getContextVk();

    vk::PipelineBinariesCache *coldCache = contextVk->getRenderer()->getPipelineBinariesCache();
    ASSERT_TRUE(coldCache->isInitialized());

    // Nothing is stored yet, so the pipelines are compiled and stored
    ANGLE_GL_PROGRAM(program1, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(program1, essl1_shaders::PositionAttrib(), 0.5f);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    const bool keptGlobalKey = restartKeepsGlobalPipelineKey();

    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));
    ANGLE_SKIP_TEST_IF(!keptGlobalKey);
    contextVk = getContextVk();
    ASSERT_NE(contextVk, nullptr);

    vk::PipelineBinariesCache *warmCache = contextVk->getRenderer()->getPipelineBinariesCache();
    ASSERT_TRUE(warmCache->isInitialized());

    // The new Display creates internal pipelines of its own, which aren't under test
    warmCache->resetStats();

    ANGLE_GL_PROGRAM(program2, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(program2, essl1_shaders::PositionAttrib(), 0.5f);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);

    EXPECT_GE(warmCache->getCacheHits(), 1u);
    EXPECT_EQ(warmCache->getCacheMisses(), 0u);
}

// The same as WarmRestartHitsPipelineBinaryCache, with several pipelines that each draw their own
// color, so that a lost entry, or a pipeline created from another one's binaries, shows up.
TEST_P(VulkanPipelineBinaryTest, WarmRestartWithManyPipelines)
{
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));

    ContextVk *contextVk = getContextVk();

    vk::PipelineBinariesCache *coldCache = contextVk->getRenderer()->getPipelineBinariesCache();
    ASSERT_TRUE(coldCache->isInitialized());

    constexpr size_t kPipelineCount = 10;

    // Nothing is stored yet, so the pipelines are compiled and stored
    for (size_t i = 0; i < kPipelineCount; ++i)
    {
        const std::string fs = MakeIdentifiableFragmentShader(i);
        ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), fs.c_str());
        drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
        EXPECT_PIXEL_COLOR_NEAR(0, 0, ExpectedColorForPipeline(i), 1);
    }

    const bool keptGlobalKey = restartKeepsGlobalPipelineKey();

    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));
    ANGLE_SKIP_TEST_IF(!keptGlobalKey);
    contextVk = getContextVk();
    ASSERT_NE(contextVk, nullptr);

    vk::PipelineBinariesCache *warmCache = contextVk->getRenderer()->getPipelineBinariesCache();
    ASSERT_TRUE(warmCache->isInitialized());

    // The new Display creates internal pipelines of its own, which aren't under test
    warmCache->resetStats();

    for (size_t i = 0; i < kPipelineCount; ++i)
    {
        const std::string fs = MakeIdentifiableFragmentShader(i);
        ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), fs.c_str());
        drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
        EXPECT_PIXEL_COLOR_NEAR(0, 0, ExpectedColorForPipeline(i), 1);
    }

    EXPECT_GE(warmCache->getCacheHits(), kPipelineCount);
    EXPECT_EQ(warmCache->getCacheMisses(), 0u);
}

// Sets the renderer's global pipeline key, as a driver update would, and puts the real one back
// when it goes out of scope.
class ScopedGlobalPipelineKey : angle::NonCopyable
{
  public:
    ScopedGlobalPipelineKey(vk::PipelineBinariesCache *cache, const VkPipelineBinaryKeyKHR &key)
        : mCache(cache), mSavedKey(cache->getGlobalKeyForTesting())
    {
        mCache->setGlobalKeyForTesting(key);
    }
    ~ScopedGlobalPipelineKey() { mCache->setGlobalKeyForTesting(mSavedKey); }

  private:
    vk::PipelineBinariesCache *mCache;
    const VkPipelineBinaryKeyKHR mSavedKey;
};

// Links the same shaders into three programs: before, during and after a change of the driver's
// global key, as a driver update would make.  Binaries stored under one global key aren't found
// under another, and are found again once the key changes back.
TEST_P(VulkanPipelineBinaryTest, StoredBinariesDependOnGlobalKey)
{
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));

    ContextVk *contextVk = getContextVk();

    vk::PipelineBinariesCache *cache = contextVk->getRenderer()->getPipelineBinariesCache();
    ASSERT_TRUE(cache->isInitialized());

    ANGLE_GL_PROGRAM(program1, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(program1, essl1_shaders::PositionAttrib(), 0.5f);
    waitForPostLinkTasks(program1);

    {
        VkPipelineBinaryKeyKHR updatedGlobalKey = cache->getGlobalKeyForTesting();
        ASSERT_GT(updatedGlobalKey.keySize, 0u);
        angle::Span(updatedGlobalKey.key)[0] ^= 0xFF;
        ScopedGlobalPipelineKey scopedGlobalKey(cache, updatedGlobalKey);

        // Nothing is stored under the new global key yet, so the first pipeline misses.  A later
        // one may share its pipeline key, and find what it stored.
        cache->resetStats();
        ANGLE_GL_PROGRAM(program2, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
        waitForPostLinkTasks(program2);
        drawQuad(program2, essl1_shaders::PositionAttrib(), 0.5f);
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
        EXPECT_GE(cache->getCacheMisses(), 1u);
    }

    // With the real key back, what the first program stored is found again
    cache->resetStats();
    ANGLE_GL_PROGRAM(program3, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    drawQuad(program3, essl1_shaders::PositionAttrib(), 0.5f);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_GE(cache->getCacheHits(), 1u);
    EXPECT_EQ(cache->getCacheMisses(), 0u);
}

// With graphics pipeline libraries, a program's shaders library is made when it links, and stored
// like any other pipeline.  Draws with a program, restarts, and links the same shaders again: the
// library is created from what was stored, before anything is drawn.
TEST_P(VulkanPipelineBinaryTest, WarmLinkHitsPipelineBinaryCacheForLibrary)
{
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::SupportsGraphicsPipelineLibrary));

    {
        ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
        drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    }

    const bool keptGlobalKey = restartKeepsGlobalPipelineKey();
    ANGLE_SKIP_TEST_IF(!isFeatureEnabled(Feature::PreferPipelineBinaries));
    ANGLE_SKIP_TEST_IF(!keptGlobalKey);
    ContextVk *contextVk = getContextVk();
    ASSERT_NE(contextVk, nullptr);

    // The new Display creates internal pipelines of its own, which aren't under test
    vk::PipelineBinariesCache *cache = contextVk->getRenderer()->getPipelineBinariesCache();
    cache->resetStats();

    // Linking makes the shaders library, from the stored binaries
    ANGLE_GL_PROGRAM(program, essl1_shaders::vs::Simple(), essl1_shaders::fs::Red());
    waitForPostLinkTasks(program);
    EXPECT_GE(cache->getCacheHits(), 1u);
    EXPECT_EQ(cache->getCacheMisses(), 0u);

    // The draw links a pipeline from the library, which is looked up too where linked pipelines are
    // stored
    drawQuad(program, essl1_shaders::PositionAttrib(), 0.5f);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    EXPECT_EQ(cache->getCacheMisses(), 0u);
}

// Graphics pipeline libraries are left at the device's default, and turned off in one
// configuration so that complete pipelines are tested where the device uses libraries.
ANGLE_INSTANTIATE_TEST(VulkanPipelineBinaryTest,
                       ES2_VULKAN().enable(Feature::AllowPipelineBinariesForTesting),
                       ES3_VULKAN().enable(Feature::AllowPipelineBinariesForTesting),
                       ES3_VULKAN()
                           .enable(Feature::AllowPipelineBinariesForTesting)
                           .disable(Feature::SupportsGraphicsPipelineLibrary));
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VulkanPipelineBinaryTest);

ANGLE_INSTANTIATE_TEST(VulkanPipelineBinaryTestES31,
                       ES31_VULKAN().enable(Feature::AllowPipelineBinariesForTesting));
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VulkanPipelineBinaryTestES31);

}  // namespace rx
