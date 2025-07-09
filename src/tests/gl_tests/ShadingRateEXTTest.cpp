//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShadingRateEXTTest.cpp : Tests of the GL_EXT_fragment_shading_rate extension.

#include "test_utils/ANGLETest.h"

#include "test_utils/gl_raii.h"

namespace angle
{

class ShadingRateEXTTest : public ANGLETest<>
{
  protected:
    ShadingRateEXTTest()
    {
        setWindowWidth(256);
        setWindowHeight(256);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

const char *simpleShadingRateVS()
{
    return R"(#version 310 es
in vec4 a_position;
void main()
{
    gl_Position = a_position;
})";
}

const char *simpleShadingRateFS()
{
    return R"(#version 310 es
#extension GL_EXT_fragment_shading_rate : enable
precision highp float;
uniform mediump vec4 u_color;
layout(location = 0) out vec4 fragColor;
void main(void)
{
    // Emit red color if ShadingRateEXT == GL_SHADING_RATE_2X2_PIXELS_EXT
    if (gl_ShadingRateEXT == 5) {
        fragColor = vec4(1.0, 0.0, 0.0, 1.0); // red
    } else {
        fragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
})";
}

// Test basic functionality of EXT_fragment_shading_rate
TEST_P(ShadingRateEXTTest, Basic)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_fragment_shading_rate"));

    // Verify validate shading rate.
    GLsizei count    = 0;
    const int maxNum = 9;
    GLenum shadingRates[maxNum];
    glGetFragmentShadingRatesEXT(1, maxNum, &count, shadingRates);
    ASSERT_GL_NO_ERROR();

    for (int i = 0; i < count; i++)
    {
        glShadingRateEXT(shadingRates[i]);
    }
    ASSERT_GL_NO_ERROR();

    glShadingRateEXT(GL_SHADING_RATE_1X1_PIXELS_EXT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ANGLE_GL_PROGRAM(uniformColorProgram, simpleShadingRateVS(), simpleShadingRateFS());
    glUseProgram(uniformColorProgram);

    // Set and query shading rate.
    glShadingRateEXT(GL_SHADING_RATE_2X2_PIXELS_EXT);
    GLint shadingRate = 0;
    glGetIntegerv(GL_SHADING_RATE_EXT, &shadingRate);
    ASSERT(shadingRate == GL_SHADING_RATE_2X2_PIXELS_EXT);

    // Verify draw call with 2x2 shading rate.
    drawQuad(uniformColorProgram, essl1_shaders::PositionAttrib(), 0.5f);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
}

// The negative test of EXT_fragment_shading_rate
TEST_P(ShadingRateEXTTest, Error)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_fragment_shading_rate"));

    glShadingRateEXT(GL_SAMPLE_SHADING);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    glShadingRateCombinerOpsEXT(GL_SHADING_RATE_EXT,
                                GL_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_EXT);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
    glShadingRateCombinerOpsEXT(GL_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_EXT,
                                GL_MIN_FRAGMENT_SHADING_RATE_ATTACHMENT_TEXEL_WIDTH_EXT);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    GLboolean supportNonTrivialCombiner = false;
    glGetBooleanv(GL_FRAGMENT_SHADING_RATE_NON_TRIVIAL_COMBINERS_SUPPORTED_EXT,
                  &supportNonTrivialCombiner);

    if (!supportNonTrivialCombiner)
    {
        glShadingRateCombinerOpsEXT(GL_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_EXT,
                                    GL_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_EXT);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);

        glShadingRateCombinerOpsEXT(GL_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_EXT,
                                    GL_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_EXT);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }

    if (!IsGLExtensionEnabled("GL_EXT_fragment_shading_rate_primitive"))
    {
        glShadingRateCombinerOpsEXT(GL_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_EXT,
                                    GL_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_EXT);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }

    if (!IsGLExtensionEnabled("GL_EXT_fragment_shading_rate_attachment"))
    {
        glShadingRateCombinerOpsEXT(GL_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_EXT,
                                    GL_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_EXT);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(ShadingRateEXTTest);
ANGLE_INSTANTIATE_TEST_ES31(ShadingRateEXTTest);

}  // namespace angle
