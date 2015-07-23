//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ParseESSLFunctions_test.cpp:
//   Tests for ESSL built-in functons.
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/PoolAlloc.h"
#include "compiler/translator/TranslatorESSL.h"

namespace
{

class ParseESSLFunctionsTest : public testing::Test
{
  public:
    ParseESSLFunctionsTest() {}

  protected:
    void SetUp()
    {
        mAllocator.push();
        SetGlobalPoolAllocator(&mAllocator);
        ShBuiltInResources resources;
        ShInitBuiltInResources(&resources);

        mTranslatorESSL = new TranslatorESSL(GL_VERTEX_SHADER, SH_GLES3_SPEC);
        ASSERT_TRUE(mTranslatorESSL->Init(resources));
    }

    void TearDown()
    {
        delete mTranslatorESSL;
        SetGlobalPoolAllocator(NULL);
        mAllocator.pop();
    }

    bool compile(const std::string& shaderString)
    {
        const char *shaderStrings[] = { shaderString.c_str() };
        return mTranslatorESSL->compileTreeForTesting(shaderStrings, 1, SH_OBJECT_CODE) != nullptr;
    }

  private:
    TranslatorESSL *mTranslatorESSL;
    TPoolAllocator mAllocator;
};

// Tests that cimpilation succeeds for built-in function overload in ESSL 1.00.
TEST_F(ParseESSLFunctionsTest, ESSL100BuiltInFunctionOverload)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "int sin(int x) {\n"
        "    return int(sin(float(x)));\n"
        "}\n"
        "void main() {\n"
        "   gl_Position = vec4(sin(1));"
        "}\n";
    EXPECT_TRUE(compile(shaderString));
}

// Tests that cimpilation fails for built-in function redefinition in ESSL 1.00.
TEST_F(ParseESSLFunctionsTest, ESSL100BuiltInFunctionRedefinition)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "float sin(float x) {\n"
        "    return sin(0.0);\n"
        "}\n"
        "void main() {\n"
        "   gl_Position = vec4(sin(1.0));\n"
        "}\n";
    EXPECT_FALSE(compile(shaderString));
}

} // namespace

