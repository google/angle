//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CollectVariables_test.cpp:
//   Some tests for shader inspection
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/TranslatorGLSL.h"

#define EXPECT_GLENUM_EQ(expected, actual) \
    EXPECT_EQ(static_cast<GLenum>(expected), static_cast<GLenum>(actual))

class CollectVariablesTest : public testing::Test
{
  public:
    CollectVariablesTest(GLenum shaderType)
        : mShaderType(shaderType)
    {}

  protected:
    virtual void SetUp()
    {
        ShBuiltInResources resources;
        ShInitBuiltInResources(&resources);
        resources.MaxDrawBuffers = 8;

        mTranslator = new TranslatorGLSL(
            mShaderType, SH_GLES3_SPEC, SH_GLSL_COMPATIBILITY_OUTPUT);
        ASSERT_TRUE(mTranslator->Init(resources));
    }

    virtual void TearDown()
    {
        delete mTranslator;
    }

    GLenum mShaderType;
    TranslatorGLSL *mTranslator;
};

class CollectVertexVariablesTest : public CollectVariablesTest
{
  public:
    CollectVertexVariablesTest() : CollectVariablesTest(GL_VERTEX_SHADER) {}
};

class CollectFragmentVariablesTest : public CollectVariablesTest
{
  public:
      CollectFragmentVariablesTest() : CollectVariablesTest(GL_FRAGMENT_SHADER) {}
};

TEST_F(CollectFragmentVariablesTest, SimpleOutputVar)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 out_fragColor;\n"
        "void main() {\n"
        "   out_fragColor = vec4(1.0);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::Attribute> &outputVariables = mTranslator->getOutputVariables();
    ASSERT_EQ(1u, outputVariables.size());

    const sh::Attribute &outputVariable = outputVariables[0];

    EXPECT_EQ(0u, outputVariable.arraySize);
    EXPECT_EQ(-1, outputVariable.location);
    EXPECT_GLENUM_EQ(GL_MEDIUM_FLOAT, outputVariable.precision);
    EXPECT_TRUE(outputVariable.staticUse);
    EXPECT_GLENUM_EQ(GL_FLOAT_VEC4, outputVariable.type);
    EXPECT_EQ("out_fragColor", outputVariable.name);
}

TEST_F(CollectFragmentVariablesTest, LocationOutputVar)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(location=5) out vec4 out_fragColor;\n"
        "void main() {\n"
        "   out_fragColor = vec4(1.0);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::Attribute> &outputVariables = mTranslator->getOutputVariables();
    ASSERT_EQ(1u, outputVariables.size());

    const sh::Attribute &outputVariable = outputVariables[0];

    EXPECT_EQ(0u, outputVariable.arraySize);
    EXPECT_EQ(5, outputVariable.location);
    EXPECT_GLENUM_EQ(GL_MEDIUM_FLOAT, outputVariable.precision);
    EXPECT_TRUE(outputVariable.staticUse);
    EXPECT_GLENUM_EQ(GL_FLOAT_VEC4, outputVariable.type);
    EXPECT_EQ("out_fragColor", outputVariable.name);
}

TEST_F(CollectVertexVariablesTest, LocationAttribute)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "layout(location=5) in vec4 in_Position;\n"
        "void main() {\n"
        "   gl_Position = in_Position;\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::Attribute> &attributes = mTranslator->getAttributes();
    ASSERT_EQ(1u, attributes.size());

    const sh::Attribute &attribute = attributes[0];

    EXPECT_EQ(0u, attribute.arraySize);
    EXPECT_EQ(5, attribute.location);
    EXPECT_GLENUM_EQ(GL_HIGH_FLOAT, attribute.precision);
    EXPECT_TRUE(attribute.staticUse);
    EXPECT_GLENUM_EQ(GL_FLOAT_VEC4, attribute.type);
    EXPECT_EQ("in_Position", attribute.name);
}

TEST_F(CollectVertexVariablesTest, SimpleInterfaceBlock)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "uniform b {\n"
        "  float f;\n"
        "};"
        "void main() {\n"
        "   gl_Position = vec4(f, 0.0, 0.0, 1.0);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::InterfaceBlock> &interfaceBlocks = mTranslator->getInterfaceBlocks();
    ASSERT_EQ(1u, interfaceBlocks.size());

    const sh::InterfaceBlock &interfaceBlock = interfaceBlocks[0];

    EXPECT_EQ(0u, interfaceBlock.arraySize);
    EXPECT_FALSE(interfaceBlock.isRowMajorLayout);
    EXPECT_EQ(sh::BLOCKLAYOUT_SHARED, interfaceBlock.layout);
    EXPECT_EQ("b", interfaceBlock.name);
    EXPECT_TRUE(interfaceBlock.staticUse);

    ASSERT_EQ(1u, interfaceBlock.fields.size());

    const sh::InterfaceBlockField &field = interfaceBlock.fields[0];

    EXPECT_GLENUM_EQ(GL_HIGH_FLOAT, field.precision);
    EXPECT_TRUE(field.staticUse);
    EXPECT_GLENUM_EQ(GL_FLOAT, field.type);
    EXPECT_EQ("f", field.name);
    EXPECT_FALSE(field.isRowMajorLayout);
    EXPECT_TRUE(field.fields.empty());
}

TEST_F(CollectVertexVariablesTest, SimpleInstancedInterfaceBlock)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "uniform b {\n"
        "  float f;\n"
        "} blockInstance;"
        "void main() {\n"
        "   gl_Position = vec4(blockInstance.f, 0.0, 0.0, 1.0);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::InterfaceBlock> &interfaceBlocks = mTranslator->getInterfaceBlocks();
    ASSERT_EQ(1u, interfaceBlocks.size());

    const sh::InterfaceBlock &interfaceBlock = interfaceBlocks[0];

    EXPECT_EQ(0u, interfaceBlock.arraySize);
    EXPECT_FALSE(interfaceBlock.isRowMajorLayout);
    EXPECT_EQ(sh::BLOCKLAYOUT_SHARED, interfaceBlock.layout);
    EXPECT_EQ("b", interfaceBlock.name);
    EXPECT_TRUE(interfaceBlock.staticUse);

    ASSERT_EQ(1u, interfaceBlock.fields.size());

    const sh::InterfaceBlockField &field = interfaceBlock.fields[0];

    EXPECT_GLENUM_EQ(GL_HIGH_FLOAT, field.precision);
    EXPECT_TRUE(field.staticUse);
    EXPECT_GLENUM_EQ(GL_FLOAT, field.type);
    EXPECT_EQ("b.f", field.name);
    EXPECT_FALSE(field.isRowMajorLayout);
    EXPECT_TRUE(field.fields.empty());
}

TEST_F(CollectVertexVariablesTest, StructInterfaceBlock)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "struct st { float f; };"
        "uniform b {\n"
        "  st s;\n"
        "};"
        "void main() {\n"
        "   gl_Position = vec4(s.f, 0.0, 0.0, 1.0);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::InterfaceBlock> &interfaceBlocks = mTranslator->getInterfaceBlocks();
    ASSERT_EQ(1u, interfaceBlocks.size());

    const sh::InterfaceBlock &interfaceBlock = interfaceBlocks[0];

    EXPECT_EQ(0u, interfaceBlock.arraySize);
    EXPECT_FALSE(interfaceBlock.isRowMajorLayout);
    EXPECT_EQ(sh::BLOCKLAYOUT_SHARED, interfaceBlock.layout);
    EXPECT_EQ("b", interfaceBlock.name);
    EXPECT_TRUE(interfaceBlock.staticUse);

    ASSERT_EQ(1u, interfaceBlock.fields.size());

    const sh::InterfaceBlockField &field = interfaceBlock.fields[0];

    EXPECT_TRUE(field.isStruct());
    EXPECT_TRUE(field.staticUse);
    EXPECT_EQ("s", field.name);
    EXPECT_FALSE(field.isRowMajorLayout);

    const sh::ShaderVariable &member = field.fields[0];

    // NOTE: we don't currently mark struct members as statically used or not
    EXPECT_FALSE(member.isStruct());
    EXPECT_EQ("f", member.name);
    EXPECT_GLENUM_EQ(GL_FLOAT, member.type);
    EXPECT_GLENUM_EQ(GL_HIGH_FLOAT, member.precision);
}

TEST_F(CollectVertexVariablesTest, StructInstancedInterfaceBlock)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "struct st { float f; };"
        "uniform b {\n"
        "  st s;\n"
        "} instanceName;"
        "void main() {\n"
        "   gl_Position = vec4(instanceName.s.f, 0.0, 0.0, 1.0);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::InterfaceBlock> &interfaceBlocks = mTranslator->getInterfaceBlocks();
    ASSERT_EQ(1u, interfaceBlocks.size());

    const sh::InterfaceBlock &interfaceBlock = interfaceBlocks[0];

    EXPECT_EQ(0u, interfaceBlock.arraySize);
    EXPECT_FALSE(interfaceBlock.isRowMajorLayout);
    EXPECT_EQ(sh::BLOCKLAYOUT_SHARED, interfaceBlock.layout);
    EXPECT_EQ("b", interfaceBlock.name);
    EXPECT_TRUE(interfaceBlock.staticUse);

    ASSERT_EQ(1u, interfaceBlock.fields.size());

    const sh::InterfaceBlockField &field = interfaceBlock.fields[0];

    EXPECT_TRUE(field.isStruct());
    EXPECT_TRUE(field.staticUse);
    EXPECT_EQ("b.s", field.name);
    EXPECT_FALSE(field.isRowMajorLayout);

    const sh::ShaderVariable &member = field.fields[0];

    // NOTE: we don't currently mark struct members as statically used or not
    EXPECT_FALSE(member.isStruct());
    EXPECT_EQ("f", member.name);
    EXPECT_GLENUM_EQ(GL_FLOAT, member.type);
    EXPECT_GLENUM_EQ(GL_HIGH_FLOAT, member.precision);
}

TEST_F(CollectVertexVariablesTest, NestedStructRowMajorInterfaceBlock)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "struct st { mat2 m; };"
        "layout(row_major) uniform b {\n"
        "  st s;\n"
        "};"
        "void main() {\n"
        "   gl_Position = vec4(s.m);\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::InterfaceBlock> &interfaceBlocks = mTranslator->getInterfaceBlocks();
    ASSERT_EQ(1u, interfaceBlocks.size());

    const sh::InterfaceBlock &interfaceBlock = interfaceBlocks[0];

    EXPECT_EQ(0u, interfaceBlock.arraySize);
    EXPECT_TRUE(interfaceBlock.isRowMajorLayout);
    EXPECT_EQ(sh::BLOCKLAYOUT_SHARED, interfaceBlock.layout);
    EXPECT_EQ("b", interfaceBlock.name);
    EXPECT_TRUE(interfaceBlock.staticUse);

    ASSERT_EQ(1u, interfaceBlock.fields.size());

    const sh::InterfaceBlockField &field = interfaceBlock.fields[0];

    EXPECT_TRUE(field.isStruct());
    EXPECT_TRUE(field.staticUse);
    EXPECT_EQ("s", field.name);
    EXPECT_TRUE(field.isRowMajorLayout);

    const sh::ShaderVariable &member = field.fields[0];

    // NOTE: we don't currently mark struct members as statically used or not
    EXPECT_FALSE(member.isStruct());
    EXPECT_EQ("m", member.name);
    EXPECT_GLENUM_EQ(GL_FLOAT_MAT2, member.type);
    EXPECT_GLENUM_EQ(GL_HIGH_FLOAT, member.precision);
}

TEST_F(CollectVertexVariablesTest, VaryingInterpolation)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "centroid out float vary;\n"
        "void main() {\n"
        "   gl_Position = vec4(1.0);\n"
        "   vary = 1.0;\n"
        "}\n";

    const char *shaderStrings[] = { shaderString.c_str() };
    ASSERT_TRUE(mTranslator->compile(shaderStrings, 1, SH_VARIABLES));

    const std::vector<sh::Varying> &varyings = mTranslator->getVaryings();
    ASSERT_EQ(2u, varyings.size());

    const sh::Varying *varying = &varyings[0];

    if (varying->name == "gl_Position")
    {
        varying = &varyings[1];
    }

    EXPECT_EQ(0u, varying->arraySize);
    EXPECT_GLENUM_EQ(GL_MEDIUM_FLOAT, varying->precision);
    EXPECT_TRUE(varying->staticUse);
    EXPECT_GLENUM_EQ(GL_FLOAT, varying->type);
    EXPECT_EQ("vary", varying->name);
    EXPECT_EQ(sh::INTERPOLATION_CENTROID, varying->interpolation);
}
