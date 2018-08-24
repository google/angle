//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureMultisampleTest: Tests of multisampled texture

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{
// Sample positions of d3d standard pattern. Some of the sample positions might not the same as
// opengl.
using SamplePositionsArray                                            = std::array<float, 32>;
static constexpr std::array<SamplePositionsArray, 5> kSamplePositions = {
    {{{0.5f, 0.5f}},
     {{0.75f, 0.75f, 0.25f, 0.25f}},
     {{0.375f, 0.125f, 0.875f, 0.375f, 0.125f, 0.625f, 0.625f, 0.875f}},
     {{0.5625f, 0.3125f, 0.4375f, 0.6875f, 0.8125f, 0.5625f, 0.3125f, 0.1875f, 0.1875f, 0.8125f,
       0.0625f, 0.4375f, 0.6875f, 0.9375f, 0.9375f, 0.0625f}},
     {{0.5625f, 0.5625f, 0.4375f, 0.3125f, 0.3125f, 0.625f,  0.75f,   0.4375f,
       0.1875f, 0.375f,  0.625f,  0.8125f, 0.8125f, 0.6875f, 0.6875f, 0.1875f,
       0.375f,  0.875f,  0.5f,    0.0625f, 0.25f,   0.125f,  0.125f,  0.75f,
       0.0f,    0.5f,    0.9375f, 0.25f,   0.875f,  0.9375f, 0.0625f, 0.0f}}}};

class TextureMultisampleTest : public ANGLETest
{
  protected:
    TextureMultisampleTest()
    {
        setWindowWidth(64);
        setWindowHeight(64);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        glGenFramebuffers(1, &mFramebuffer);
        glGenTextures(1, &mTexture);

        ASSERT_GL_NO_ERROR();
    }

    void TearDown() override
    {
        glDeleteFramebuffers(1, &mFramebuffer);
        mFramebuffer = 0;
        glDeleteTextures(1, &mTexture);
        mTexture = 0;

        ANGLETest::TearDown();
    }

    GLuint mFramebuffer = 0;
    GLuint mTexture     = 0;
};

class TextureMultisampleTestES31 : public TextureMultisampleTest
{
  protected:
    TextureMultisampleTestES31() : TextureMultisampleTest() {}
};

class TextureMultisampleArrayWebGLTest : public TextureMultisampleTest
{
  protected:
    TextureMultisampleArrayWebGLTest() : TextureMultisampleTest()
    {
        // These tests run in WebGL mode so we can test with both extension off and on.
        setWebGLCompatibilityEnabled(true);
    }

    // Requests the ANGLE_texture_multisample_array extension and returns true if the operation
    // succeeds.
    bool requestArrayExtension()
    {
        if (extensionRequestable("GL_ANGLE_texture_multisample_array"))
        {
            glRequestExtensionANGLE("GL_ANGLE_texture_multisample_array");
        }

        if (!extensionEnabled("GL_ANGLE_texture_multisample_array"))
        {
            return false;
        }
        return true;
    }
};

// Tests that if es version < 3.1, GL_TEXTURE_2D_MULTISAMPLE is not supported in
// GetInternalformativ. Checks that the number of samples returned is valid in case of ES >= 3.1.
TEST_P(TextureMultisampleTest, MultisampleTargetGetInternalFormativBase)
{
    // This query returns supported sample counts in descending order. If only one sample count is
    // queried, it should be the maximum one.
    GLint maxSamplesR8 = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_R8, GL_SAMPLES, 1, &maxSamplesR8);
    if (getClientMajorVersion() < 3 || getClientMinorVersion() < 1)
    {
        ASSERT_GL_ERROR(GL_INVALID_ENUM);
    }
    else
    {
        ASSERT_GL_NO_ERROR();

        // GLES 3.1 section 19.3.1 specifies the required minimum of how many samples are supported.
        GLint maxColorTextureSamples;
        glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxColorTextureSamples);
        GLint maxSamples;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        GLint maxSamplesR8Required = std::min(maxColorTextureSamples, maxSamples);

        EXPECT_GE(maxSamplesR8, maxSamplesR8Required);
    }
}

// Tests that if es version < 3.1, GL_TEXTURE_2D_MULTISAMPLE is not supported in
// FramebufferTexture2D.
TEST_P(TextureMultisampleTest, MultisampleTargetFramebufferTexture2D)
{
    GLint samples = 1;
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mTexture);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA8, 64, 64, GL_FALSE);
    if (getClientMajorVersion() < 3 || getClientMinorVersion() < 1)
    {
        ASSERT_GL_ERROR(GL_INVALID_ENUM);
    }
    else
    {
        ASSERT_GL_NO_ERROR();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                           mTexture, 0);
    if (getClientMajorVersion() < 3 || getClientMinorVersion() < 1)
    {
        ASSERT_GL_ERROR(GL_INVALID_OPERATION);
    }
    else
    {
        ASSERT_GL_NO_ERROR();
    }
}

// Tests basic functionality of glTexStorage2DMultisample.
TEST_P(TextureMultisampleTestES31, ValidateTextureStorageMultisampleParameters)
{
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mTexture);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_NO_ERROR();

    GLint params = 0;
    glGetTexParameteriv(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_IMMUTABLE_FORMAT, &params);
    EXPECT_EQ(1, params);

    glTexStorage2DMultisample(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);

    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 0, 0, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, maxSize + 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    GLint maxSamples = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE, GL_R8, GL_SAMPLES, 1, &maxSamples);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, maxSamples + 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_OPERATION);

    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 0, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA, 0, 0, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_VALUE);

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 1, 1, GL_FALSE);
    ASSERT_GL_ERROR(GL_INVALID_OPERATION);
}

// Tests the value of MAX_INTEGER_SAMPLES is no less than 1.
// [OpenGL ES 3.1 SPEC Table 20.40]
TEST_P(TextureMultisampleTestES31, MaxIntegerSamples)
{
    GLint maxIntegerSamples;
    glGetIntegerv(GL_MAX_INTEGER_SAMPLES, &maxIntegerSamples);
    EXPECT_GE(maxIntegerSamples, 1);
    EXPECT_NE(std::numeric_limits<GLint>::max(), maxIntegerSamples);
}

// Tests the value of MAX_COLOR_TEXTURE_SAMPLES is no less than 1.
// [OpenGL ES 3.1 SPEC Table 20.40]
TEST_P(TextureMultisampleTestES31, MaxColorTextureSamples)
{
    GLint maxColorTextureSamples;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxColorTextureSamples);
    EXPECT_GE(maxColorTextureSamples, 1);
    EXPECT_NE(std::numeric_limits<GLint>::max(), maxColorTextureSamples);
}

// Tests the value of MAX_DEPTH_TEXTURE_SAMPLES is no less than 1.
// [OpenGL ES 3.1 SPEC Table 20.40]
TEST_P(TextureMultisampleTestES31, MaxDepthTextureSamples)
{
    GLint maxDepthTextureSamples;
    glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &maxDepthTextureSamples);
    EXPECT_GE(maxDepthTextureSamples, 1);
    EXPECT_NE(std::numeric_limits<GLint>::max(), maxDepthTextureSamples);
}

// The value of sample position should be equal to standard pattern on D3D.
TEST_P(TextureMultisampleTestES31, CheckSamplePositions)
{
    ANGLE_SKIP_TEST_IF(!IsD3D11());

    GLsizei maxSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);

    GLfloat samplePosition[2];

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFramebuffer);

    for (int sampleCount = 1; sampleCount <= maxSamples; sampleCount++)
    {
        GLTexture texture;
        size_t indexKey = static_cast<size_t>(ceil(log2(sampleCount)));
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);
        glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sampleCount, GL_RGBA8, 1, 1, GL_TRUE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE,
                               texture, 0);
        EXPECT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));
        ASSERT_GL_NO_ERROR();

        for (int sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++)
        {
            glGetMultisamplefv(GL_SAMPLE_POSITION, sampleIndex, samplePosition);
            EXPECT_EQ(samplePosition[0], kSamplePositions[indexKey][2 * sampleIndex]);
            EXPECT_EQ(samplePosition[1], kSamplePositions[indexKey][2 * sampleIndex + 1]);
        }
    }

    ASSERT_GL_NO_ERROR();
}

// Tests that GL_TEXTURE_2D_MULTISAMPLE_ARRAY is not supported in GetInternalformativ when the
// extension is not supported.
TEST_P(TextureMultisampleArrayWebGLTest, MultisampleArrayTargetGetInternalFormativWithoutExtension)
{
    GLint maxSamples = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_RGBA8, GL_SAMPLES, 1,
                          &maxSamples);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);
}

// Attempt to bind a texture to multisample array binding point when extension is not supported.
TEST_P(TextureMultisampleArrayWebGLTest, BindMultisampleArrayTextureWithoutExtension)
{
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_ERROR(GL_INVALID_ENUM);
}

// Tests that GL_TEXTURE_2D_MULTISAMPLE_ARRAY is supported in GetInternalformativ.
TEST_P(TextureMultisampleArrayWebGLTest, MultisampleArrayTargetGetInternalFormativ)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    // This query returns supported sample counts in descending order. If only one sample count is
    // queried, it should be the maximum one.
    GLint maxSamplesRGBA8 = 0;
    glGetInternalformativ(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_RGBA8, GL_SAMPLES, 1,
                          &maxSamplesRGBA8);
    ASSERT_GL_NO_ERROR();

    // GLES 3.1 section 19.3.1 specifies the required minimum of how many samples are supported.
    GLint maxColorTextureSamples;
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &maxColorTextureSamples);
    GLint maxSamples;
    glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    GLint maxSamplesRGBA8Required = std::min(maxColorTextureSamples, maxSamples);

    EXPECT_GE(maxSamplesRGBA8, maxSamplesRGBA8Required);
}

// Tests that TexImage3D call cannot be used for GL_TEXTURE_2D_MULTISAMPLE_ARRAY.
TEST_P(TextureMultisampleArrayWebGLTest, MultiSampleArrayTexImage)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_NO_ERROR();

    glTexImage3D(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_RGBA8, 1, 1, 1, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
}

// Tests passing invalid parameters to TexStorage3DMultisample.
TEST_P(TextureMultisampleArrayWebGLTest, InvalidTexStorage3DMultisample)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_NO_ERROR();

    // Invalid target
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE, 2, GL_RGBA8, 1, 1, 1, GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    // Samples 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 0, GL_RGBA8, 1, 1, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    // Unsized internalformat
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA, 1, 1, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    // Width 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA8, 0, 1, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    // Height 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA8, 1, 0, 1,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);

    // Depth 0
    glTexStorage3DMultisampleANGLE(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, 2, GL_RGBA8, 1, 1, 0,
                                   GL_TRUE);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

// Tests passing invalid parameters to TexParameteri.
TEST_P(TextureMultisampleArrayWebGLTest, InvalidTexParameteri)
{
    ANGLE_SKIP_TEST_IF(!requestArrayExtension());

    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, mTexture);
    ASSERT_GL_NO_ERROR();

    // None of the sampler parameters can be set on GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE.
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MIN_LOD, 0);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_MAX_LOD, 0);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_COMPARE_MODE, GL_NONE);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    // Only valid base level on GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE is 0.
    glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE_ARRAY_ANGLE, GL_TEXTURE_BASE_LEVEL, 1);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

ANGLE_INSTANTIATE_TEST(TextureMultisampleTest,
                       ES31_D3D11(),
                       ES3_OPENGL(),
                       ES3_OPENGLES(),
                       ES31_OPENGL(),
                       ES31_OPENGLES());
ANGLE_INSTANTIATE_TEST(TextureMultisampleTestES31, ES31_D3D11(), ES31_OPENGL(), ES31_OPENGLES());
ANGLE_INSTANTIATE_TEST(TextureMultisampleArrayWebGLTest,
                       ES31_D3D11(),
                       ES31_OPENGL(),
                       ES31_OPENGLES());

}  // anonymous namespace
