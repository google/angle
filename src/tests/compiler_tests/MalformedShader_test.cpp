//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MalformedShader_test.cpp:
//   Tests that malformed shaders fail compilation.
//

#include "angle_gl.h"
#include "gtest/gtest.h"
#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/TranslatorESSL.h"

class MalformedShaderTest : public testing::Test
{
  public:
    MalformedShaderTest() {}

  protected:
    virtual void SetUp()
    {
        ShBuiltInResources resources;
        ShInitBuiltInResources(&resources);

        mTranslator = new TranslatorESSL(GL_FRAGMENT_SHADER, SH_GLES3_SPEC);
        ASSERT_TRUE(mTranslator->Init(resources));
    }

    virtual void TearDown()
    {
        delete mTranslator;
    }

    // Return true when compilation succeeds
    bool compile(const std::string& shaderString)
    {
        const char *shaderStrings[] = { shaderString.c_str() };
        bool compilationSuccess = mTranslator->compile(shaderStrings, 1, SH_INTERMEDIATE_TREE);
        TInfoSink &infoSink = mTranslator->getInfoSink();
        mInfoLog = infoSink.info.c_str();
        return compilationSuccess;
    }

  protected:
    std::string mInfoLog;

  private:
    TranslatorESSL *mTranslator;
};

// This is a test for a bug that used to exist in ANGLE:
// Calling a function with all parameters missing should not succeed.
TEST_F(MalformedShaderTest, FunctionParameterMismatch)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "float fun(float a) {\n"
        "   return a * 2.0;\n"
        "}\n"
        "void main() {\n"
        "   float ff = fun();\n"
        "   gl_FragColor = vec4(ff);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Functions can't be redeclared as variables in the same scope (ESSL 1.00 section 4.2.7)
TEST_F(MalformedShaderTest, RedeclaringFunctionAsVariable)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "float fun(float a) {\n"
        "   return a * 2.0;\n"
        "}\n"
        "float fun;\n"
        "void main() {\n"
        "   gl_FragColor = vec4(0.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Functions can't be redeclared as structs in the same scope (ESSL 1.00 section 4.2.7)
TEST_F(MalformedShaderTest, RedeclaringFunctionAsStruct)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "float fun(float a) {\n"
        "   return a * 2.0;\n"
        "}\n"
        "struct fun { float a; };\n"
        "void main() {\n"
        "   gl_FragColor = vec4(0.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Functions can't be redeclared with different qualifiers (ESSL 1.00 section 6.1.0)
TEST_F(MalformedShaderTest, RedeclaringFunctionWithDifferentQualifiers)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "float fun(out float a);\n"
        "float fun(float a) {\n"
        "   return a * 2.0;\n"
        "}\n"
        "void main() {\n"
        "   gl_FragColor = vec4(0.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Assignment and equality are undefined for structures containing arrays (ESSL 1.00 section 5.7)
TEST_F(MalformedShaderTest, CompareStructsContainingArrays)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "struct s { float a[3]; };\n"
        "void main() {\n"
        "   s a;\n"
        "   s b;\n"
        "   bool c = (a == b);\n"
        "   gl_FragColor = vec4(c ? 1.0 : 0.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Assignment and equality are undefined for structures containing arrays (ESSL 1.00 section 5.7)
TEST_F(MalformedShaderTest, AssignStructsContainingArrays)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "struct s { float a[3]; };\n"
        "void main() {\n"
        "   s a;\n"
        "   s b;\n"
        "   b.a[0] = 0.0;\n"
        "   a = b;\n"
        "   gl_FragColor = vec4(a.a[0]);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Assignment and equality are undefined for structures containing samplers (ESSL 1.00 sections 5.7 and 5.9)
TEST_F(MalformedShaderTest, CompareStructsContainingSamplers)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "struct s { sampler2D foo; };\n"
        "uniform s a;\n"
        "uniform s b;\n"
        "void main() {\n"
        "   bool c = (a == b);\n"
        "   gl_FragColor = vec4(c ? 1.0 : 0.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Samplers are not allowed as l-values (ESSL 3.00 section 4.1.7), our interpretation is that this
// extends to structs containing samplers. ESSL 1.00 spec is clearer about this.
TEST_F(MalformedShaderTest, AssignStructsContainingSamplers)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "struct s { sampler2D foo; };\n"
        "uniform s a;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   s b;\n"
        "   b = a;\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// This is a regression test for a particular bug that was in ANGLE.
// It also verifies that ESSL3 functionality doesn't leak to ESSL1.
TEST_F(MalformedShaderTest, ArrayWithNoSizeInInitializerList)
{
    const std::string &shaderString =
        "precision mediump float;\n"
        "void main() {\n"
        "   float a[2], b[];\n"
        "   gl_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Const variables need an initializer.
TEST_F(MalformedShaderTest, ConstVarNotInitialized)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   const float a;\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Const variables need an initializer. In ESSL1 const structs containing
// arrays are not allowed at all since it's impossible to initialize them.
// Even though this test is for ESSL3 the only thing that's critical for
// ESSL1 is the non-initialization check that's used for both language versions.
// Whether ESSL1 compilation generates the most helpful error messages is a
// secondary concern.
TEST_F(MalformedShaderTest, ConstStructNotInitialized)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "struct S {\n"
        "   float a[3];\n"
        "};\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   const S b;\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Const variables need an initializer. In ESSL1 const arrays are not allowed
// at all since it's impossible to initialize them.
// Even though this test is for ESSL3 the only thing that's critical for
// ESSL1 is the non-initialization check that's used for both language versions.
// Whether ESSL1 compilation generates the most helpful error messages is a
// secondary concern.
TEST_F(MalformedShaderTest, ConstArrayNotInitialized)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   const float a[3];\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Block layout qualifiers can't be used on non-block uniforms (ESSL 3.00 section 4.3.8.3)
TEST_F(MalformedShaderTest, BlockLayoutQualifierOnRegularUniform)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(packed) uniform mat2 x;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Block layout qualifiers can't be used on non-block uniforms (ESSL 3.00 section 4.3.8.3)
TEST_F(MalformedShaderTest, BlockLayoutQualifierOnUniformWithEmptyDecl)
{
    // Yes, the comma in the declaration below is not a typo.
    // Empty declarations are allowed in GLSL.
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "layout(packed) uniform mat2, x;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Arrays of arrays are not allowed (ESSL 3.00 section 4.1.9)
TEST_F(MalformedShaderTest, ArraysOfArrays1)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   float[5] a[3];\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Arrays of arrays are not allowed (ESSL 3.00 section 4.1.9)
TEST_F(MalformedShaderTest, ArraysOfArrays2)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   float[2] a, b[3];\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Implicitly sized arrays need to be initialized (ESSL 3.00 section 4.1.9)
TEST_F(MalformedShaderTest, UninitializedImplicitArraySize)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   float[] a;\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// An operator can only form a constant expression if all the operands are constant expressions
// - even operands of ternary operator that are never evaluated. (ESSL 3.00 section 4.3.3)
TEST_F(MalformedShaderTest, TernaryOperatorNotConstantExpression)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "uniform bool u;\n"
        "void main() {\n"
        "   const bool a = true ? true : u;\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Ternary operator can't operate on arrays (ESSL 3.00 section 5.7)
TEST_F(MalformedShaderTest, TernaryOperatorOnArrays)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   float[1] a = float[1](0.0);\n"
        "   float[1] b = float[1](1.0);\n"
        "   float[1] c = true ? a : b;\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Ternary operator can't operate on structs (ESSL 3.00 section 5.7)
TEST_F(MalformedShaderTest, TernaryOperatorOnStructs)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "struct S { float foo; };\n"
        "void main() {\n"
        "   S a = S(0.0);\n"
        "   S b = S(1.0);\n"
        "   S c = true ? a : b;\n"
        "   my_FragColor = vec4(1.0);\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}

// Array length() returns a constant signed integral expression (ESSL 3.00 section 4.1.9)
// Assigning it to unsigned should result in an error.
TEST_F(MalformedShaderTest, AssignArrayLengthToUnsigned)
{
    const std::string &shaderString =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "void main() {\n"
        "   int[1] arr;\n"
        "   uint l = arr.length();\n"
        "   my_FragColor = vec4(float(l));\n"
        "}\n";
    if (compile(shaderString))
    {
        FAIL() << "Shader compilation succeeded, expecting failure " << mInfoLog;
    }
}
