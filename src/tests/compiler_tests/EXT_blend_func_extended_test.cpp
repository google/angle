//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EXT_blend_func_extended.cpp:
//   Test for EXT_blend_func_extended_test
//

#include "tests/test_utils/ShaderExtensionTest.h"

namespace
{
const char EXTBFEPragma[] = "#extension GL_EXT_blend_func_extended : require\n";

// Shader that writes to SecondaryFragColor and SecondaryFragData does not compile.
const char ESSL100_ColorAndDataWriteFailureShader1[] =
    "precision mediump float;\n"
    "void main() {\n"
    "    gl_SecondaryFragColorEXT = vec4(1.0);\n"
    "    gl_SecondaryFragDataEXT[gl_MaxDualSourceDrawBuffersEXT] = vec4(0.1);\n"
    "}\n";

// Shader that writes to FragColor and SecondaryFragData does not compile.
const char ESSL100_ColorAndDataWriteFailureShader2[] =
    "precision mediump float;\n"
    "void main() {\n"
    "    gl_FragColor = vec4(1.0);\n"
    "    gl_SecondaryFragDataEXT[gl_MaxDualSourceDrawBuffersEXT] = vec4(0.1);\n"
    "}\n";

// Shader that writes to FragData and SecondaryFragColor.
const char ESSL100_ColorAndDataWriteFailureShader3[] =
    "#extension GL_EXT_draw_buffers : require\n"
    "precision mediump float;\n"
    "void main() {\n"
    "    gl_SecondaryFragColorEXT = vec4(1.0);\n"
    "    gl_FragData[gl_MaxDrawBuffers] = vec4(0.1);\n"
    "}\n";

// Dynamic indexing of SecondaryFragData is not allowed in WebGL 2.0.
const char ESSL100_IndexSecondaryFragDataWithNonConstantShader[] =
    "precision mediump float;\n"
    "void main() {\n"
    "    for (int i = 0; i < 2; ++i) {\n"
    "        gl_SecondaryFragDataEXT[true ? 0 : i] = vec4(0.0);\n"
    "    }\n"
    "}\n";

// Shader that specifies index layout qualifier but not location fails to compile.
const char ESSL300_LocationIndexFailureShader[] =
    R"(precision mediump float;
layout(index = 0) out vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
})";

// Shader that specifies index layout qualifier multiple times fails to compile.
const char ESSL300_DoubleIndexFailureShader[] =
    R"(precision mediump float;
layout(index = 0, location = 0, index = 1) out vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
})";

// Shader that specifies an output with out-of-bounds location
// for index 0 when another output uses index 1 is invalid.
const char ESSL300_Index0OutOfBoundsFailureShader[] =
    R"(precision mediump float;
layout(location = 1, index = 0) out mediump vec4 fragColor;
layout(location = 0, index = 1) out mediump vec4 secondaryFragColor;
void main() {
    fragColor = vec4(1);
    secondaryFragColor = vec4(1);
})";

// Shader that specifies an output with out-of-bounds location for index 1 is invalid.
const char ESSL300_Index1OutOfBoundsFailureShader[] =
    R"(precision mediump float;
layout(location = 1, index = 1) out mediump vec4 secondaryFragColor;
void main() {
    secondaryFragColor = vec4(1);
})";

// Shader that specifies two outputs with the same location
// but different indices and different base types is invalid.
const char ESSL300_IndexTypeMismatchFailureShader[] =
    R"(precision mediump float;
layout(location = 0, index = 0) out mediump vec4 fragColor;
layout(location = 0, index = 1) out mediump ivec4 secondaryFragColor;
void main() {
    fragColor = vec4(1);
    secondaryFragColor = ivec4(1);
})";

// Global index layout qualifier fails.
const char ESSL300_GlobalIndexFailureShader[] =
    R"(precision mediump float;
layout(index = 0);
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
})";

// Index layout qualifier on a non-output variable fails.
const char ESSL300_IndexOnUniformVariableFailureShader[] =
    R"(precision mediump float;
layout(index = 0) uniform vec4 u;
out vec4 fragColor;
void main() {
    fragColor = u;
})";

// Index layout qualifier on a struct fails.
const char ESSL300_IndexOnStructFailureShader[] =
    R"(precision mediump float;
layout(index = 0) struct S {
    vec4 field;
};
out vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
})";

// Index layout qualifier on a struct member fails.
const char ESSL300_IndexOnStructFieldFailureShader[] =
    R"(precision mediump float;
struct S {
    layout(index = 0) vec4 field;
};
out mediump vec4 fragColor;
void main() {
    fragColor = vec4(1.0);
})";

class EXTBlendFuncExtendedCompileFailureTest : public sh::ShaderExtensionTest
{
  protected:
    void SetUp() override
    {
        sh::ShaderExtensionTest::SetUp();
        // EXT_draw_buffers is used in some of the shaders for test purposes.
        mResources.EXT_draw_buffers = 1;
        mResources.NV_draw_buffers  = 2;
    }
};

TEST_P(EXTBlendFuncExtendedCompileFailureTest, CompileFails)
{
    // Expect compile failure due to shader error, with shader having correct pragma.
    mResources.EXT_blend_func_extended  = 1;
    mResources.MaxDualSourceDrawBuffers = 1;
    InitializeCompiler();
    EXPECT_FALSE(TestShaderCompile(EXTBFEPragma));
}

// Incorrect #version 100 shaders fail.
INSTANTIATE_TEST_SUITE_P(IncorrectESSL100Shaders,
                         EXTBlendFuncExtendedCompileFailureTest,
                         Combine(Values(SH_GLES2_SPEC),
                                 Values(sh::ESSLVersion100),
                                 Values(ESSL100_ColorAndDataWriteFailureShader1,
                                        ESSL100_ColorAndDataWriteFailureShader2,
                                        ESSL100_ColorAndDataWriteFailureShader3)));

// Correct #version 100 shaders that are incorrect in WebGL 2.0.
INSTANTIATE_TEST_SUITE_P(IncorrectESSL100ShadersWebGL2,
                         EXTBlendFuncExtendedCompileFailureTest,
                         Combine(Values(SH_WEBGL2_SPEC),
                                 Values(sh::ESSLVersion100),
                                 Values(ESSL100_IndexSecondaryFragDataWithNonConstantShader)));

// Incorrect #version 300 es shaders always fail.
INSTANTIATE_TEST_SUITE_P(IncorrectESSL300Shaders,
                         EXTBlendFuncExtendedCompileFailureTest,
                         Combine(Values(SH_GLES3_1_SPEC),
                                 Values(sh::ESSLVersion300, sh::ESSLVersion310),
                                 Values(ESSL300_LocationIndexFailureShader,
                                        ESSL300_DoubleIndexFailureShader,
                                        ESSL300_Index0OutOfBoundsFailureShader,
                                        ESSL300_Index1OutOfBoundsFailureShader,
                                        ESSL300_IndexTypeMismatchFailureShader,
                                        ESSL300_GlobalIndexFailureShader,
                                        ESSL300_IndexOnUniformVariableFailureShader,
                                        ESSL300_IndexOnStructFailureShader,
                                        ESSL300_IndexOnStructFieldFailureShader)));

}  // namespace
