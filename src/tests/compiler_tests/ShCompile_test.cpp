//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShCompile_test.cpp
//   Test the sh::Compile interface with different parameters.
//

#include <clocale>
#include "GLSLANG/ShaderLang.h"
#include "angle_gl.h"
#include "common/platform.h"
#include "gtest/gtest.h"

class ShCompileTest : public testing::Test
{
  public:
    ShCompileTest() {}

  protected:
    void SetUp() override
    {
        sh::InitBuiltInResources(&mResources);
        mCompiler = sh::ConstructCompiler(GL_FRAGMENT_SHADER, SH_WEBGL_SPEC,
                                          SH_GLSL_COMPATIBILITY_OUTPUT, &mResources);
        ASSERT_TRUE(mCompiler != nullptr) << "Compiler could not be constructed.";
    }

    void TearDown() override
    {
        if (mCompiler)
        {
            sh::Destruct(mCompiler);
            mCompiler = nullptr;
        }
    }

    void testCompile(const char **shaderStrings, int stringCount, bool expectation)
    {
        ShCompileOptions options      = SH_OBJECT_CODE | SH_VARIABLES | SH_INIT_OUTPUT_VARIABLES;
        bool success                  = sh::Compile(mCompiler, shaderStrings, stringCount, options);
        const std::string &compileLog = sh::GetInfoLog(mCompiler);
        EXPECT_EQ(expectation, success) << compileLog;
    }

    ShBuiltInResources mResources;

  public:
    ShHandle mCompiler;
};

class ShCompileComputeTest : public ShCompileTest
{
  public:
    ShCompileComputeTest() {}

  protected:
    void SetUp() override
    {
        sh::InitBuiltInResources(&mResources);
        mCompiler = sh::ConstructCompiler(GL_COMPUTE_SHADER, SH_WEBGL3_SPEC,
                                          SH_GLSL_COMPATIBILITY_OUTPUT, &mResources);
        ASSERT_TRUE(mCompiler != nullptr) << "Compiler could not be constructed.";
    }
};

// Test calling sh::Compile with compute shader source string.
TEST_F(ShCompileComputeTest, ComputeShaderString)
{
    constexpr char kComputeShaderString[] =
        R"(#version 310 es
        layout(local_size_x=1) in;
        void main()
        {
        })";

    const char *shaderStrings[] = {kComputeShaderString};

    testCompile(shaderStrings, 1, true);
}

// Test calling sh::Compile with more than one shader source string.
TEST_F(ShCompileTest, MultipleShaderStrings)
{
    const std::string &shaderString1 =
        "precision mediump float;\n"
        "void main() {\n";
    const std::string &shaderString2 =
        "    gl_FragColor = vec4(0.0);\n"
        "}";

    const char *shaderStrings[] = {shaderString1.c_str(), shaderString2.c_str()};

    testCompile(shaderStrings, 2, true);
}

// Test calling sh::Compile with a tokens split into different shader source strings.
TEST_F(ShCompileTest, TokensSplitInShaderStrings)
{
    const std::string &shaderString1 =
        "precision mediump float;\n"
        "void ma";
    const std::string &shaderString2 =
        "in() {\n"
        "#i";
    const std::string &shaderString3 =
        "f 1\n"
        "    gl_FragColor = vec4(0.0);\n"
        "#endif\n"
        "}";

    const char *shaderStrings[] = {shaderString1.c_str(), shaderString2.c_str(),
                                   shaderString3.c_str()};

    testCompile(shaderStrings, 3, true);
}

// Parsing floats in shaders can run afoul of locale settings.
// In de_DE, `strtof("1.9")` will yield `1.0f`. (It's expecting "1,9")
TEST_F(ShCompileTest, DecimalSepLocale)
{
    const auto defaultLocale = setlocale(LC_NUMERIC, nullptr);

    const auto fnSetLocale = [](const char *const name) {
        return bool(setlocale(LC_NUMERIC, name));
    };

    const bool setLocaleToDe =
        fnSetLocale("de_DE") || fnSetLocale("de-DE");  // Windows doesn't like de_DE.

// These configs don't support de_DE: android_angle_vk[32,64]_rel_ng, linux_angle_rel_ng
// Just allow those platforms to quietly fail, but require other platforms to succeed.
#if defined(ANGLE_PLATFORM_ANDROID) || defined(ANGLE_PLATFORM_LINUX)
    if (!setLocaleToDe)
    {
        return;
    }
#endif
    ASSERT_TRUE(setLocaleToDe);

    const char kSource[] = R"(
        void main()
        {
            gl_FragColor = vec4(1.9);
        })";
    const char *parts[]  = {kSource};
    testCompile(parts, 1, true);

    const auto &translated = sh::GetObjectCode(mCompiler);
    // printf("%s\n", translated.data());
    EXPECT_NE(translated.find("1.9"), std::string::npos);

    fnSetLocale(defaultLocale);
}
