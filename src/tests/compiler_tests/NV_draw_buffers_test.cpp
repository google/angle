//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// NV_draw_buffers_test.cpp:
//   Test for NV_draw_buffers setting
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/TranslatorESSL.h"

class NVDrawBuffersTest : public testing::Test
{
  public:
    NVDrawBuffersTest() {}

  protected:
    virtual void SetUp()
    {
        ShBuiltInResources resources;
        ShInitBuiltInResources(&resources);
        resources.MaxDrawBuffers = 8;
        resources.EXT_draw_buffers = 1;
        resources.NV_draw_buffers = 1;

        mTranslator = new TranslatorESSL(GL_FRAGMENT_SHADER, SH_GLES2_SPEC);
        ASSERT_TRUE(mTranslator->Init(resources));
    }

    virtual void TearDown()
    {
        delete mTranslator;
    }

    TranslatorESSL *mTranslator;
};

TEST_F(NVDrawBuffersTest, NVDrawBuffers)
{
    const std::string &shaderString =
        "#extension GL_EXT_draw_buffers : require\n"
        "precision mediump float;\n"
        "void main() {\n"
        "   gl_FragData[0] = vec4(1.0);\n"
        "   gl_FragData[1] = vec4(0.0);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_OBJECT_CODE));

    TInfoSink& infoSink = mTranslator->getInfoSink();
    std::string objCode(infoSink.obj.c_str());
    size_t nv_draw_buffers_ind = objCode.find("GL_NV_draw_buffers");
    EXPECT_NE(std::string::npos, nv_draw_buffers_ind);
    size_t ext_draw_buffers_ind = objCode.find("GL_EXT_draw_buffers");
    EXPECT_EQ(std::string::npos, ext_draw_buffers_ind);
}
