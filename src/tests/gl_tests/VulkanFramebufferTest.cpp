//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// VulkanFramebufferTest:
//   Tests to validate our Vulkan framebuffer image allocation.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/angle_test_instantiate.h"
// 'None' is defined as 'struct None {};' in
// third_party/googletest/src/googletest/include/gtest/internal/gtest-type-util.h.
// But 'None' is also defined as a numeric constant 0L in <X11/X.h>.
// So we need to include ANGLETest.h first to avoid this conflict.

#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/ProgramVk.h"
#include "libANGLE/renderer/vulkan/TextureVk.h"
#include "test_utils/gl_raii.h"
#include "util/EGLWindow.h"
#include "util/shader_utils.h"

using namespace angle;

namespace
{

class VulkanFramebufferTest : public ANGLETest<>
{
  protected:
    gl::Context *hackContext() const
    {
        egl::Display *display   = static_cast<egl::Display *>(getEGLWindow()->getDisplay());
        gl::ContextID contextID = {
            static_cast<GLuint>(reinterpret_cast<uintptr_t>(getEGLWindow()->getContext()))};
        return display->getContextRef(contextID).get();
    }

    rx::ContextVk *hackANGLE() const
    {
        // Hack the angle!
        return rx::GetImplAs<rx::ContextVk>(hackContext());
    }

    rx::TextureVk *hackTexture(GLuint handle) const
    {
        // Hack the angle!
        const gl::Context *context = hackContext();
        const gl::Texture *texture = context->getTexture({handle});
        return rx::vk::GetImpl(texture);
    }

    // Repeatedly copies the whole of source into the texture's level while the texture has
    // no storage, so every copy is staged.  Before any flush, the number of updates staged on that
    // level must stay bounded by the pruning threshold.  The copies are identical, so each one
    // supersedes all earlier ones.
    void testRepeatedCopiesArePrunedBeforeFlush(GLuint texture,
                                                GLint level,
                                                GLsizei size,
                                                GLuint source)
    {
        // Matches kUpdateCountThreshold in TextureVk::copySubImageImplWithTransfer.
        constexpr size_t kMaxStagedUpdates = 5;
        constexpr int kCopyCount           = 16;

        glBindFramebuffer(GL_FRAMEBUFFER, source);
        glBindTexture(GL_TEXTURE_2D, texture);
        glCopyTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, 0, 0, size, size, 0);
        for (int i = 0; i < kCopyCount; ++i)
        {
            glCopyTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, 0, 0, size, size);
        }
        ASSERT_GL_NO_ERROR();

        // Nothing has used the texture yet, so no flush has pruned the staged updates.  Without
        // pruning during the copies, kCopyCount + 1 updates would be staged here.
        rx::TextureVk *textureVk = hackTexture(texture);
        EXPECT_LE(
            textureVk->getLevelUpdateCountForTesting(gl::OwnerLevel(static_cast<uint32_t>(level))),
            kMaxStagedUpdates);
    }

    // Fills the color attachment of a new framebuffer with green, returning it in fboOut.
    void createGreenSourceFramebuffer(GLsizei size, GLTexture *colorOut, GLFramebuffer *fboOut)
    {
        glBindTexture(GL_TEXTURE_2D, *colorOut);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindFramebuffer(GL_FRAMEBUFFER, *fboOut);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colorOut, 0);
        ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void verifyTextureLevelIsGreen(GLuint texture, GLint level, GLsizei size)
    {
        GLFramebuffer readFbo;
        glBindFramebuffer(GL_FRAMEBUFFER, readFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, level);
        ASSERT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);
        EXPECT_PIXEL_RECT_EQ(0, 0, size, size, GLColor::green);
    }
};

// Test that framebuffer can be created from a mip-incomplete texture, and that its allocation only
// includes the framebuffer's attached mip.
TEST_P(VulkanFramebufferTest, TextureAttachmentMipIncomplete)
{
    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 100, 100, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA8, 5, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Set framebuffer to mip 0.  Framebuffer should be complete, and make the texture allocate
    // an image of only 1 level.
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    EXPECT_GL_FRAMEBUFFER_COMPLETE(GL_FRAMEBUFFER);

    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::black);

    rx::TextureVk *textureVk = hackTexture(texture);
    EXPECT_EQ(textureVk->getImage().getLevelCount(), 1u);
}

// Test ensure that a R4G4B4A4 format sample only will actually uses R4G4B4A4 format instead of
// R8G8B8A8.
TEST_P(VulkanFramebufferTest, R4G4B4A4TextureSampleOnlyActuallyUses444Format)
{
    rx::ContextVk *contextVk   = hackANGLE();
    rx::vk::Renderer *renderer = contextVk->getRenderer();
    angle::FormatID formatID   = angle::FormatID::R4G4B4A4_UNORM;

    // Check if R4G4B4A4_UNORM is supported format.
    bool isTexturable = renderer->hasImageFormatFeatureBits(
        formatID,
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    ANGLE_SKIP_TEST_IF(!isTexturable);

    static constexpr GLsizei kTexWidth  = 100;
    static constexpr GLsizei kTexHeight = 100;
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);

    const GLushort u16Color = 0xF00F;  // red
    std::vector<GLushort> pixels(kTexWidth * kTexHeight, u16Color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA4, kTexWidth, kTexHeight, 0, GL_RGBA,
                 GL_UNSIGNED_SHORT_4_4_4_4, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ANGLE_GL_PROGRAM(program, essl3_shaders::vs::Texture2DLod(), essl3_shaders::fs::Texture2DLod());
    glUseProgram(program);
    GLint textureLocation = glGetUniformLocation(program, essl3_shaders::Texture2DUniform());
    ASSERT_NE(-1, textureLocation);
    GLint lodLocation = glGetUniformLocation(program, essl3_shaders::LodUniform());
    ASSERT_NE(-1, lodLocation);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glClearColor(0, 1, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glUniform1f(lodLocation, 0);
    drawQuad(program, essl3_shaders::PositionAttrib(), 0.5f);
    EXPECT_PIXEL_EQ(getWindowWidth() / 2, getWindowHeight() / 2, 255, 0, 0, 255);
    ASSERT_GL_NO_ERROR();

    rx::TextureVk *textureVk = hackTexture(texture);
    EXPECT_EQ(textureVk->getImage().getActualFormatID(), formatID);
}

// Test that identical glCopyTexSubImage2D calls staged to level 0 of a texture without storage are
// pruned as they are staged, rather than accumulating until the texture is flushed.
TEST_P(VulkanFramebufferTest, RepeatedCopyTexSubImagePrunedLevel0)
{
    constexpr GLsizei kSize = 16;

    GLTexture sourceColor;
    GLFramebuffer sourceFbo;
    createGreenSourceFramebuffer(kSize, &sourceColor, &sourceFbo);

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    testRepeatedCopiesArePrunedBeforeFlush(texture, 0, kSize, sourceFbo);
    verifyTextureLevelIsGreen(texture, 0, kSize);
}

// Same as RepeatedCopyTexSubImagePrunedLevel0, but copying to a non-zero level, to make sure the
// level being copied to is the one that is pruned.
TEST_P(VulkanFramebufferTest, RepeatedCopyTexSubImagePrunedLevel1)
{
    constexpr GLsizei kSize = 16;

    GLTexture sourceColor;
    GLFramebuffer sourceFbo;
    createGreenSourceFramebuffer(kSize, &sourceColor, &sourceFbo);

    // Define level 0 without data so that level 1 has a consistent size, but the texture still
    // has no storage.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, kSize * 2, kSize * 2, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    testRepeatedCopiesArePrunedBeforeFlush(texture, 1, kSize, sourceFbo);

    // Restrict the texture to level 1 so it is complete for the readback attachment.
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
    verifyTextureLevelIsGreen(texture, 1, kSize);
}

ANGLE_INSTANTIATE_TEST(VulkanFramebufferTest, ES3_VULKAN());
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(VulkanFramebufferTest);

}  // anonymous namespace
