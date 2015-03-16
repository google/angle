//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ConstantFolding_test.cpp:
//   Tests for constant folding
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/PoolAlloc.h"
#include "compiler/translator/TranslatorESSL.h"

class ConstantFinder : public TIntermTraverser
{
  public:
    ConstantFinder(ConstantUnion constToFind)
        : mConstToFind(constToFind),
          mFound(false)
    {}

    virtual void visitConstantUnion(TIntermConstantUnion *node)
    {
        if (node->getUnionArrayPointer()[0] == mConstToFind)
        {
            mFound = true;
        }
    }

    bool found() const { return mFound; }

  private:
    ConstantUnion mConstToFind;
    bool mFound;
};

class ConstantFoldingTest : public testing::Test
{
  public:
    ConstantFoldingTest() {}

  protected:
    virtual void SetUp()
    {
        allocator.push();
        SetGlobalPoolAllocator(&allocator);
        ShBuiltInResources resources;
        ShInitBuiltInResources(&resources);

        mTranslatorESSL = new TranslatorESSL(GL_FRAGMENT_SHADER, SH_GLES3_SPEC);
        ASSERT_TRUE(mTranslatorESSL->Init(resources));
    }

    virtual void TearDown()
    {
        delete mTranslatorESSL;
        SetGlobalPoolAllocator(NULL);
        allocator.pop();
    }

    void compile(const std::string& shaderString)
    {
        const char *shaderStrings[] = { shaderString.c_str() };

        mASTRoot = mTranslatorESSL->compileTreeForTesting(shaderStrings, 1, SH_OBJECT_CODE);
        if (!mASTRoot)
        {
            TInfoSink &infoSink = mTranslatorESSL->getInfoSink();
            FAIL() << "Shader compilation into ESSL failed " << infoSink.info.c_str();
        }
    }

    bool constantFoundInAST(ConstantUnion c)
    {
        ConstantFinder finder(c);
        mASTRoot->traverse(&finder);
        return finder.found();
    }

    bool constantFoundInAST(int i)
    {
        ConstantUnion c;
        c.setIConst(i);
        return constantFoundInAST(c);
    }

  private:
    TranslatorESSL *mTranslatorESSL;
    TIntermNode *mASTRoot;

    TPoolAllocator allocator;
};

TEST_F(ConstantFoldingTest, FoldIntegerAdd)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out int my_Int;\n"
        "void main() {\n"
        "   const int i = 1124 + 5;\n"
        "   my_Int = i;\n"
        "}\n";
    compile(shaderString);
    ASSERT_FALSE(constantFoundInAST(1124));
    ASSERT_FALSE(constantFoundInAST(5));
    ASSERT_TRUE(constantFoundInAST(1129));
};

TEST_F(ConstantFoldingTest, FoldIntegerSub)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out int my_Int;\n"
        "void main() {\n"
        "   const int i = 1124 - 5;\n"
        "   my_Int = i;\n"
        "}\n";
    compile(shaderString);
    ASSERT_FALSE(constantFoundInAST(1124));
    ASSERT_FALSE(constantFoundInAST(5));
    ASSERT_TRUE(constantFoundInAST(1119));
};

TEST_F(ConstantFoldingTest, FoldIntegerMul)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out int my_Int;\n"
        "void main() {\n"
        "   const int i = 1124 * 5;\n"
        "   my_Int = i;\n"
        "}\n";
    compile(shaderString);
    ASSERT_FALSE(constantFoundInAST(1124));
    ASSERT_FALSE(constantFoundInAST(5));
    ASSERT_TRUE(constantFoundInAST(5620));
};

TEST_F(ConstantFoldingTest, FoldIntegerDiv)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out int my_Int;\n"
        "void main() {\n"
        "   const int i = 1124 / 5;\n"
        "   my_Int = i;\n"
        "}\n";
    compile(shaderString);
    ASSERT_FALSE(constantFoundInAST(1124));
    ASSERT_FALSE(constantFoundInAST(5));
    // Rounding mode of division is undefined in the spec but ANGLE can be expected to round down.
    ASSERT_TRUE(constantFoundInAST(224));
};

TEST_F(ConstantFoldingTest, FoldIntegerModulus)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out int my_Int;\n"
        "void main() {\n"
        "   const int i = 1124 % 5;\n"
        "   my_Int = i;\n"
        "}\n";
    compile(shaderString);
    ASSERT_FALSE(constantFoundInAST(1124));
    ASSERT_FALSE(constantFoundInAST(5));
    ASSERT_TRUE(constantFoundInAST(4));
};
