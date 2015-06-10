//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Pack_Unpack_test.cpp:
//   Tests for the emulating pack_unpack functions for GLSL 4.1.
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/TranslatorGLSL.h"

namespace
{

class PackUnpackTest : public testing::Test
{
  public:
    PackUnpackTest() {}

  protected:
    void SetUp() override
    {
        ShBuiltInResources resources;
        ShInitBuiltInResources(&resources);

        mTranslator = new TranslatorGLSL(
        GL_FRAGMENT_SHADER, SH_GLES3_SPEC, SH_GLSL_410_CORE_OUTPUT);
        ASSERT_TRUE(mTranslator->Init(resources));
    }

    void TearDown() override
    {
        SafeDelete(mTranslator);
    }

    void compile(const std::string& shaderString)
    {
        const char *shaderStrings[] = { shaderString.c_str() };

        bool compilationSuccess = mTranslator->compile(shaderStrings, 1, SH_OBJECT_CODE);
        TInfoSink &infoSink = mTranslator->getInfoSink();
        mGLSLCode = infoSink.obj.c_str();
        if (!compilationSuccess)
        {
            FAIL() << "Shader compilation into GLSL 4.1 failed " << infoSink.info.c_str();
        }
    }

    bool foundInGLSLCode(const char* stringToFind)
    {
        return mGLSLCode.find(stringToFind) != std::string::npos;
    }

  private:
    TranslatorGLSL *mTranslator;
    std::string mGLSLCode;
};

//Check if PackSnorm2x16 Emulation for GLSL 4.1 compile correctly.
TEST_F(PackUnpackTest, PackSnorm2x16Emulation)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = 0) out mediump vec4 fragColor;"
        "void main() {\n"
        "   vec2 v;\n"
        "   uint u = packSnorm2x16(v);\n"
        "   fragColor = vec4(0.0);\n"
        "}\n";
    compile(shaderString);
    ASSERT_TRUE(foundInGLSLCode("uint webgl_packSnorm2x16_emu(vec2 v)"));
}

//Check if UnpackSnorm2x16 Emulation for GLSL 4.1 compile correctly.
TEST_F(PackUnpackTest, UnpackSnorm2x16Emulation)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = 0) out mediump vec4 fragColor;"
        "void main() {\n"
        "   uint u;\n"
        "   vec2 v=unpackSnorm2x16(u);\n"
        "   fragColor = vec4(0.0);\n"
        "}\n";
    compile(shaderString);
    ASSERT_TRUE(foundInGLSLCode("vec2 webgl_unpackSnorm2x16_emu(uint u)"));
}

//Check if PackHalf2x16 Emulation for GLSL 4.1 compile correctly.
TEST_F(PackUnpackTest, PackHalf2x16Emulation)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = 0) out mediump vec4 fragColor;"
        "void main() {\n"
        "   vec2 v;\n"
        "   uint u=packHalf2x16(v);\n"
        "   fragColor = vec4(0.0);\n"
        "}\n";
    compile(shaderString);
    ASSERT_TRUE(foundInGLSLCode("uint webgl_packHalf2x16_emu(vec2 v)"));
}

//Check if UnpackHalf2x16 Emulation for GLSL 4.1 compile correctly.
TEST_F(PackUnpackTest, UnpackHalf2x16Emulation)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location = 0) out mediump vec4 fragColor;"
        "void main() {\n"
        "   uint u;\n"
        "   vec2 v=unpackHalf2x16(u);\n"
        "   fragColor = vec4(0.0);\n"
        "}\n";
    compile(shaderString);
    ASSERT_TRUE(foundInGLSLCode("vec2 webgl_unpackHalf2x16_emu(uint u)"));
}

}
