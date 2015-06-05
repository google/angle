//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BuiltInFunctionEmulator_test.cpp:
//   Tests for writing the code for built-in function emulation.
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/TranslatorGLSL.h"

namespace
{

// Test for the SH_EMULATE_BUILT_IN_FUNCTIONS flag.
class EmulateBuiltInFunctionsTest : public testing::Test
{
  public:
    EmulateBuiltInFunctionsTest() {}

  protected:
    void SetUp() override
    {
        ShBuiltInResources resources;
        ShInitBuiltInResources(&resources);

        mTranslatorGLSL = new TranslatorGLSL(GL_VERTEX_SHADER, SH_GLES2_SPEC, SH_GLSL_COMPATIBILITY_OUTPUT);
        ASSERT_TRUE(mTranslatorGLSL->Init(resources));
    }

    void TearDown() override
    {
        SafeDelete(mTranslatorGLSL);
    }

    void compile(const std::string &shaderString)
    {
        const char *shaderStrings[] = { shaderString.c_str() };

        bool compilationSuccess = mTranslatorGLSL->compile(shaderStrings, 1,
                                                           SH_OBJECT_CODE | SH_EMULATE_BUILT_IN_FUNCTIONS);
        TInfoSink &infoSink = mTranslatorGLSL->getInfoSink();
        mGLSLCode = infoSink.obj.c_str();
        if (!compilationSuccess)
        {
            FAIL() << "Shader compilation into GLSL failed " << infoSink.info.c_str();
        }
    }

    bool foundInCode(const char *stringToFind)
    {
        return mGLSLCode.find(stringToFind) != std::string::npos;
    }

  private:
    TranslatorGLSL *mTranslatorGLSL;
    std::string mGLSLCode;
};

TEST_F(EmulateBuiltInFunctionsTest, DotEmulated)
{
    const std::string shaderString =
        "precision mediump float;\n"
        "uniform float u;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(dot(u, 1.0), 1.0, 1.0, 1.0);\n"
        "}\n";
    compile(shaderString);
    ASSERT_TRUE(foundInCode("webgl_dot_emu("));
}

} // namespace
