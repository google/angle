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
        "#version 300 es;\n"
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
