//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RemoveUnreferencedVariables_test.cpp:
//   Tests for removing unreferenced variables from the AST.
//

#include "GLSLANG/ShaderLang.h"
#include "angle_gl.h"
#include "gtest/gtest.h"
#include "tests/test_utils/compiler_test.h"

using namespace sh;

class RemoveUnreferencedVariablesTest : public MatchOutputCodeTest
{
  public:
    RemoveUnreferencedVariablesTest() : MatchOutputCodeTest(GL_FRAGMENT_SHADER, 0, SH_ESSL_OUTPUT)
    {
    }
};

// Test that a simple unreferenced declaration is pruned.
TEST_F(RemoveUnreferencedVariablesTest, SimpleDeclaration)
{
    const std::string &shaderString =
        R"(precision mediump float;
        void main()
        {
            vec4 myUnreferencedVec;
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("myUnreferencedVec"));
}

// Test that a simple unreferenced global declaration is pruned.
TEST_F(RemoveUnreferencedVariablesTest, SimpleGlobalDeclaration)
{
    const std::string &shaderString =
        R"(precision mediump float;

        vec4 myUnreferencedVec;

        void main()
        {
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("myUnreferencedVec"));
}

// Test that a simple unreferenced variable with an initializer is pruned.
TEST_F(RemoveUnreferencedVariablesTest, SimpleInitializer)
{
    const std::string &shaderString =
        R"(precision mediump float;
        uniform vec4 uVec;
        void main()
        {
            vec4 myUnreferencedVec = uVec;
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("myUnreferencedVec"));
}

// Test that a user-defined function call inside an unreferenced variable initializer is retained.
TEST_F(RemoveUnreferencedVariablesTest, SideEffectInInitializer)
{
    const std::string &shaderString =
        R"(precision mediump float;
        vec4 sideEffect(int i)
        {
            gl_FragColor = vec4(0, i, 0, 1);
            return vec4(0);
        }
        void main()
        {
            vec4 myUnreferencedVec = sideEffect(1);
        })";
    compile(shaderString);

    // We're happy as long as the function with side effects is called.
    ASSERT_TRUE(foundInCode("sideEffect(1)"));
}

// Test that a modf call inside an unreferenced variable initializer is retained.
TEST_F(RemoveUnreferencedVariablesTest, BuiltInSideEffectInInitializer)
{
    const std::string &shaderString =
        R"(#version 300 es
        precision mediump float;
        uniform float uF;
        out vec4 my_FragColor;

        void main()
        {
            float iPart = 0.0;
            float myUnreferencedFloat = modf(uF, iPart);
            my_FragColor = vec4(0.0, iPart, 0.0, 1.0);
        })";
    compile(shaderString);

    // We're happy as long as the function with side effects is called.
    ASSERT_TRUE(foundInCode("modf("));
}

// Test that an imageStore call inside an unreferenced variable initializer is retained.
TEST_F(RemoveUnreferencedVariablesTest, ImageStoreSideEffectInInitializer)
{
    const std::string &shaderString =
        R"(#version 310 es
        precision highp float;
        layout(rgba32i) uniform highp writeonly iimage2D img;

        void main()
        {
            float myUnreferencedFloat = (imageStore(img, ivec2(0), ivec4(1)), 1.0);
        })";
    compile(shaderString);

    // We're happy as long as the function with side effects is called.
    ASSERT_TRUE(foundInCode("imageStore("));
}

// Test that multiple variables that are chained but otherwise are unreferenced are removed.
TEST_F(RemoveUnreferencedVariablesTest, MultipleVariablesChained)
{
    const std::string &shaderString =
        R"(precision mediump float;
        uniform vec4 uVec;
        void main()
        {
            vec4 myUnreferencedVec1 = uVec;
            vec4 myUnreferencedVec2 = myUnreferencedVec1 * 2.0;
            vec4 myUnreferencedVec3 = myUnreferencedVec2 + 1.0;
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("myUnreferencedVec3"));
    ASSERT_TRUE(notFoundInCode("myUnreferencedVec2"));
    ASSERT_TRUE(notFoundInCode("myUnreferencedVec1"));
}

// Test that multiple variables that are chained with the last one being referenced are kept.
TEST_F(RemoveUnreferencedVariablesTest, MultipleVariablesChainedReferenced)
{
    const std::string &shaderString =
        R"(precision mediump float;
        uniform vec4 uVec;
        void main()
        {
            vec4 myReferencedVec1 = uVec;
            vec4 myReferencedVec2 = myReferencedVec1 * 2.0;
            vec4 myReferencedVec3 = myReferencedVec2 + 1.0;
            gl_FragColor = myReferencedVec3;
        })";
    compile(shaderString);

    ASSERT_TRUE(foundInCode("myReferencedVec3"));
    ASSERT_TRUE(foundInCode("myReferencedVec2"));
    ASSERT_TRUE(foundInCode("myReferencedVec1"));
}

// Test that multiple variables that are chained within two scopes but otherwise are unreferenced
// are removed.
TEST_F(RemoveUnreferencedVariablesTest, MultipleVariablesChainedTwoScopes)
{
    const std::string &shaderString =
        R"(precision mediump float;
        uniform vec4 uVec;
        void main()
        {
            vec4 myUnreferencedVec1 = uVec;
            vec4 myUnreferencedVec2 = myUnreferencedVec1 * 2.0;
            if (uVec.x > 0.0)
            {
                vec4 myUnreferencedVec3 = myUnreferencedVec2 + 1.0;
            }
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("myUnreferencedVec3"));
    ASSERT_TRUE(notFoundInCode("myUnreferencedVec2"));
    ASSERT_TRUE(notFoundInCode("myUnreferencedVec1"));
}

// Test that multiple variables that are chained with the last one being referenced in an inner
// scope are kept.
TEST_F(RemoveUnreferencedVariablesTest, VariableReferencedInAnotherScope)
{
    const std::string &shaderString =
        R"(precision mediump float;
        uniform vec4 uVec;
        void main()
        {
            vec4 myReferencedVec1 = uVec;
            vec4 myReferencedVec2 = myReferencedVec1 * 2.0;
            if (uVec.x > 0.0)
            {
                vec4 myReferencedVec3 = myReferencedVec2 + 1.0;
                gl_FragColor = myReferencedVec3;
            }
        })";
    compile(shaderString);

    ASSERT_TRUE(foundInCode("myReferencedVec3"));
    ASSERT_TRUE(foundInCode("myReferencedVec2"));
    ASSERT_TRUE(foundInCode("myReferencedVec1"));
}

// Test that if there are two variables with the same name, one of them can be removed and another
// one kept.
TEST_F(RemoveUnreferencedVariablesTest, TwoVariablesWithSameNameInDifferentScopes)
{
    const std::string &shaderString =
        R"(precision mediump float;
        uniform vec4 uVec;
        void main()
        {
            vec4 myVec = uVec;  // This one is unreferenced.
            if (uVec.x > 0.0)
            {
                vec4 myVec = uVec * 2.0;  // This one is referenced.
                gl_FragColor = myVec;
            }
            vec4 myUnreferencedVec = myVec;
        })";
    compile(shaderString);

    ASSERT_TRUE(foundInCode("myVec", 2));
}

// Test that an unreferenced variable declared in a for loop header is removed.
TEST_F(RemoveUnreferencedVariablesTest, UnreferencedVariableDeclaredInForLoopHeader)
{
    const std::string &shaderString =
        R"(#version 300 es
        precision highp float;
        uniform int ui;

        out vec4 my_FragColor;

        void main()
        {
            my_FragColor = vec4(0.0);
            int index = 0;
            for (int unreferencedInt = ui; index < 10; ++index)
            {
                my_FragColor += vec4(0.0, float(index) * 0.01, 0.0, 0.0);
            }
        })";
    compile(shaderString);

    ASSERT_TRUE(foundInCode("index"));
    ASSERT_TRUE(notFoundInCode("unreferencedInt"));
}

// Test that a loop condition is kept even if it declares an unreferenced variable.
TEST_F(RemoveUnreferencedVariablesTest, UnreferencedVariableDeclaredInWhileLoopCondition)
{
    const std::string &shaderString =
        R"(#version 300 es
        precision highp float;
        uniform int ui;

        out vec4 my_FragColor;

        void main()
        {
            my_FragColor = vec4(0.0);
            int index = 0;
            while (bool b = (index < 10))
            {
                my_FragColor += vec4(0.0, float(index) * 0.01, 0.0, 0.0);
                ++index;
            }
        })";
    compile(shaderString);

    ASSERT_TRUE(foundInCode("index < 10"));
}

// Test that a variable declared in a for loop header that is only referenced in an unreferenced
// variable initializer is removed.
TEST_F(RemoveUnreferencedVariablesTest,
       VariableDeclaredInForLoopHeaderAccessedInUnreferencedVariableInitializer)
{
    const std::string &shaderString =
        R"(#version 300 es
        precision highp float;
        uniform int ui;

        out vec4 my_FragColor;

        void main()
        {
            my_FragColor = vec4(0.0);
            int index = 0;
            for (int unreferencedInt1 = ui; index < 10; ++index)
            {
                int unreferencedInt2 = unreferencedInt1;
                my_FragColor += vec4(0.0, float(index) * 0.01, 0.0, 0.0);
            }
        })";
    compile(shaderString);

    ASSERT_TRUE(foundInCode("index"));
    ASSERT_TRUE(notFoundInCode("unreferencedInt2"));
    ASSERT_TRUE(notFoundInCode("unreferencedInt1"));
}

// Test that a user-defined type (struct) declaration that's used is not removed, but that the
// variable that's declared in the same declaration is removed.
TEST_F(RemoveUnreferencedVariablesTest, UserDefinedTypeReferencedAndVariableNotReferenced)
{
    const std::string &shaderString =
        R"(#version 300 es
        precision highp float;
        uniform float uF;

        out vec4 my_FragColor;

        void main()
        {
            struct myStruct { float member; } unreferencedStruct;
            myStruct usedStruct = myStruct(uF);
            my_FragColor = vec4(usedStruct.member);
        })";
    compile(shaderString);

    ASSERT_TRUE(foundInCode("myStruct"));
    ASSERT_TRUE(foundInCode("usedStruct"));
    ASSERT_TRUE(notFoundInCode("unreferencedStruct"));
}

// Test that a nameless user-defined type (struct) declaration is removed entirely.
TEST_F(RemoveUnreferencedVariablesTest, NamelessUserDefinedTypeUnreferenced)
{
    const std::string &shaderString =
        R"(#version 300 es
        precision highp float;
        void main()
        {
            struct { float member; } unreferencedStruct;
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("unreferencedStruct"));
    ASSERT_TRUE(notFoundInCode("member"));
}

// Test that a variable that's only referenced in a unused function is removed.
TEST_F(RemoveUnreferencedVariablesTest, VariableOnlyReferencedInUnusedFunction)
{
    const std::string &shaderString =
        R"(
        int onlyReferencedInUnusedFunction = 0;
        void unusedFunc() {
            onlyReferencedInUnusedFunction++;
        }

        void main()
        {
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("onlyReferencedInUnusedFunction"));
}

// Test that a variable that's only referenced in an array length() method call is removed.
TEST_F(RemoveUnreferencedVariablesTest, VariableOnlyReferencedInLengthMethod)
{
    const std::string &shaderString =
        R"(#version 300 es
        precision highp float;

        out vec4 my_FragColor;

        void main()
        {
            float onlyReferencedInLengthMethodCall[1];
            int len = onlyReferencedInLengthMethodCall.length();
            my_FragColor = vec4(0, len, 0, 1);
        })";
    compile(shaderString);

    ASSERT_TRUE(notFoundInCode("onlyReferencedInLengthMethodCall"));
}
