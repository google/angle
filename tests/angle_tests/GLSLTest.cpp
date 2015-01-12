#include "ANGLETest.h"

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(GLSLTest, ES2_D3D9, ES2_D3D11);

template<typename T>
class GLSLTest : public ANGLETest
{
protected:
    GLSLTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        mSimpleVSSource = SHADER_SOURCE
        (
            attribute vec4 inputAttribute;
            void main()
            {
                gl_Position = inputAttribute;
            }
        );
    }

    std::string GenerateVaryingType(GLint vectorSize)
    {
        char varyingType[10];

        if (vectorSize == 1)
        {
            sprintf(varyingType, "float");
        }
        else
        {
            sprintf(varyingType, "vec%d", vectorSize);
        }

        return std::string(varyingType);
    }

    std::string GenerateVectorVaryingDeclaration(GLint vectorSize, GLint arraySize, GLint id)
    {
        char buff[100];

        if (arraySize == 1)
        {
            sprintf(buff, "varying %s v%d;\n", GenerateVaryingType(vectorSize).c_str(), id);
        }
        else
        {
            sprintf(buff, "varying %s v%d[%d];\n", GenerateVaryingType(vectorSize).c_str(), id, arraySize);
        }

        return std::string(buff);
    }

    std::string GenerateVectorVaryingSettingCode(GLint vectorSize, GLint arraySize, GLint id)
    {
        std::string returnString;
        char buff[100];

        if (arraySize == 1)
        {
            sprintf(buff, "\t v%d = %s(1.0);\n", id, GenerateVaryingType(vectorSize).c_str());
            returnString += buff;
        }
        else
        {
            for (int i = 0; i < arraySize; i++)
            {
                sprintf(buff, "\t v%d[%d] = %s(1.0);\n", id, i, GenerateVaryingType(vectorSize).c_str());
                returnString += buff;
            }
        }

        return returnString;
    }

    std::string GenerateVectorVaryingUseCode(GLint arraySize, GLint id)
    {
        if (arraySize == 1)
        {
            char buff[100];
            sprintf(buff, "v%d + ", id);
            return std::string(buff);
        }
        else
        {
            std::string returnString;
            for (int i = 0; i < arraySize; i++)
            {
                char buff[100];
                sprintf(buff, "v%d[%d] + ", id, i);
                returnString += buff;
            }
            return returnString;
        }
    }

    void GenerateGLSLWithVaryings(GLint floatCount, GLint floatArrayCount, GLint vec2Count, GLint vec2ArrayCount, GLint vec3Count, GLint vec3ArrayCount, std::string* fragmentShader, std::string* vertexShader)
    {
        // Generate a string declaring the varyings, to share between the fragment shader and the vertex shader.
        std::string varyingDeclaration;

        unsigned int varyingCount = 0;

        for (GLint i = 0; i < floatCount; i++)
        {
            varyingDeclaration += GenerateVectorVaryingDeclaration(1, 1, varyingCount);
            varyingCount += 1;
        }

        for (GLint i = 0; i < floatArrayCount; i++)
        {
            varyingDeclaration += GenerateVectorVaryingDeclaration(1, 2, varyingCount);
            varyingCount += 1;
        }

        for (GLint i = 0; i < vec2Count; i++)
        {
            varyingDeclaration += GenerateVectorVaryingDeclaration(2, 1, varyingCount);
            varyingCount += 1;
        }

        for (GLint i = 0; i < vec2ArrayCount; i++)
        {
            varyingDeclaration += GenerateVectorVaryingDeclaration(2, 2, varyingCount);
            varyingCount += 1;
        }

        for (GLint i = 0; i < vec3Count; i++)
        {
            varyingDeclaration += GenerateVectorVaryingDeclaration(3, 1, varyingCount);
            varyingCount += 1;
        }

        for (GLint i = 0; i < vec3ArrayCount; i++)
        {
            varyingDeclaration += GenerateVectorVaryingDeclaration(3, 2, varyingCount);
            varyingCount += 1;
        }

        // Generate the vertex shader
        vertexShader->clear();
        vertexShader->append(varyingDeclaration);
        vertexShader->append("\nvoid main()\n{\n");

        unsigned int currentVSVarying = 0;

        for (GLint i = 0; i < floatCount; i++)
        {
            vertexShader->append(GenerateVectorVaryingSettingCode(1, 1, currentVSVarying));
            currentVSVarying += 1;
        }

        for (GLint i = 0; i < floatArrayCount; i++)
        {
            vertexShader->append(GenerateVectorVaryingSettingCode(1, 2, currentVSVarying));
            currentVSVarying += 1;
        }

        for (GLint i = 0; i < vec2Count; i++)
        {
            vertexShader->append(GenerateVectorVaryingSettingCode(2, 1, currentVSVarying));
            currentVSVarying += 1;
        }

        for (GLint i = 0; i < vec2ArrayCount; i++)
        {
            vertexShader->append(GenerateVectorVaryingSettingCode(2, 2, currentVSVarying));
            currentVSVarying += 1;
        }

        for (GLint i = 0; i < vec3Count; i++)
        {
            vertexShader->append(GenerateVectorVaryingSettingCode(3, 1, currentVSVarying));
            currentVSVarying += 1;
        }

        for (GLint i = 0; i < vec3ArrayCount; i++)
        {
            vertexShader->append(GenerateVectorVaryingSettingCode(3, 2, currentVSVarying));
            currentVSVarying += 1;
        }

        vertexShader->append("}\n");

        // Generate the fragment shader
        fragmentShader->clear();
        fragmentShader->append("precision highp float;\n");
        fragmentShader->append(varyingDeclaration);
        fragmentShader->append("\nvoid main() \n{ \n\tvec4 retColor = vec4(0,0,0,0);\n");

        unsigned int currentFSVarying = 0;

        // Make use of the float varyings
        fragmentShader->append("\tretColor += vec4(");

        for (GLint i = 0; i < floatCount; i++)
        {
            fragmentShader->append(GenerateVectorVaryingUseCode(1, currentFSVarying));
            currentFSVarying += 1;
        }

        for (GLint i = 0; i < floatArrayCount; i++)
        {
            fragmentShader->append(GenerateVectorVaryingUseCode(2, currentFSVarying));
            currentFSVarying += 1;
        }

        fragmentShader->append("0.0, 0.0, 0.0, 0.0);\n");

        // Make use of the vec2 varyings
        fragmentShader->append("\tretColor += vec4(");

        for (GLint i = 0; i < vec2Count; i++)
        {
            fragmentShader->append(GenerateVectorVaryingUseCode(1, currentFSVarying));
            currentFSVarying += 1;
        }

        for (GLint i = 0; i < vec2ArrayCount; i++)
        {
            fragmentShader->append(GenerateVectorVaryingUseCode(2, currentFSVarying));
            currentFSVarying += 1;
        }

        fragmentShader->append("vec2(0.0, 0.0), 0.0, 0.0);\n");

        // Make use of the vec3 varyings
        fragmentShader->append("\tretColor += vec4(");

        for (GLint i = 0; i < vec3Count; i++)
        {
            fragmentShader->append(GenerateVectorVaryingUseCode(1, currentFSVarying));
            currentFSVarying += 1;
        }

        for (GLint i = 0; i < vec3ArrayCount; i++)
        {
            fragmentShader->append(GenerateVectorVaryingUseCode(2, currentFSVarying));
            currentFSVarying += 1;
        }

        fragmentShader->append("vec3(0.0, 0.0, 0.0), 0.0);\n");
        fragmentShader->append("\tgl_FragColor = retColor;\n}");
    }

    std::string mSimpleVSSource;
};

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(GLSLTest_ES3, ES3_D3D11);

template<typename T>
class GLSLTest_ES3 : public GLSLTest<T>
{
};

TYPED_TEST(GLSLTest, NamelessScopedStructs)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;

        void main()
        {
            struct
            {
                float q;
            } b;

            gl_FragColor = vec4(1, 0, 0, 1);
            gl_FragColor.a += b.q;
        }
    );

    GLuint program = CompileProgram(mSimpleVSSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, ScopedStructsOrderBug)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;

        struct T
        {
            float f;
        };

        void main()
        {
            T a;

            struct T
            {
                float q;
            };

            T b;

            gl_FragColor = vec4(1, 0, 0, 1);
            gl_FragColor.a += a.f;
            gl_FragColor.a += b.q;
        }
    );

    GLuint program = CompileProgram(mSimpleVSSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, ScopedStructsBug)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;

        struct T_0
        {
            float f;
        };

        void main()
        {
            gl_FragColor = vec4(1, 0, 0, 1);

            struct T
            {
                vec2 v;
            };

            T_0 a;
            T b;

            gl_FragColor.a += a.f;
            gl_FragColor.a += b.v.x;
        }
    );

    GLuint program = CompileProgram(mSimpleVSSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, DxPositionBug)
{
    const std::string &vertexShaderSource = SHADER_SOURCE
    (
        attribute vec4 inputAttribute;
        varying float dx_Position;
        void main()
        {
            gl_Position = vec4(inputAttribute);
            dx_Position = 0.0;
        }
    );

    const std::string &fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;

        varying float dx_Position;

        void main()
        {
            gl_FragColor = vec4(dx_Position, 0, 0, 1);
        }
    );

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, ElseIfRewriting)
{
    const std::string &vertexShaderSource =
        "attribute vec4 a_position;\n"
        "varying float v;\n"
        "void main() {\n"
        "  gl_Position = a_position;\n"
        "  v = 1.0;\n"
        "  if (a_position.x <= 0.5) {\n"
        "    v = 0.0;\n"
        "  } else if (a_position.x >= 0.5) {\n"
        "    v = 2.0;\n"
        "  }\n"
        "}\n";

    const std::string &fragmentShaderSource =
        "precision highp float;\n"
        "varying float v;\n"
        "void main() {\n"
        "  vec4 color = vec4(1.0, 0.0, 0.0, 1.0);\n"
        "  if (v >= 1.0) color = vec4(0.0, 1.0, 0.0, 1.0);\n"
        "  if (v >= 2.0) color = vec4(0.0, 0.0, 1.0, 1.0);\n"
        "  gl_FragColor = color;\n"
        "}\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    ASSERT_NE(0u, program);

    drawQuad(program, "a_position", 0.5f);
    swapBuffers();

    EXPECT_PIXEL_EQ(0, 0, 255, 0, 0, 255);
    EXPECT_PIXEL_EQ(getWindowWidth()-1, 0, 0, 255, 0, 255);
}

TYPED_TEST(GLSLTest, TwoElseIfRewriting)
{
    const std::string &vertexShaderSource =
        "attribute vec4 a_position;\n"
        "varying float v;\n"
        "void main() {\n"
        "  gl_Position = a_position;\n"
        "  if (a_position.x == 0.0) {\n"
        "    v = 1.0;\n"
        "  } else if (a_position.x > 0.5) {\n"
        "    v = 0.0;\n"
        "  } else if (a_position.x > 0.75) {\n"
        "    v = 0.5;\n"
        "  }\n"
        "}\n";

    const std::string &fragmentShaderSource =
        "precision highp float;\n"
        "varying float v;\n"
        "void main() {\n"
        "  gl_FragColor = vec4(v, 0.0, 0.0, 1.0);\n"
        "}\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, InvariantVaryingOut)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;
        varying float v_varying;
        void main() { gl_FragColor = vec4(v_varying, 0, 0, 1.0); }
    );

    const std::string vertexShaderSource = SHADER_SOURCE
    (
        attribute vec4 a_position;
        invariant varying float v_varying;
        void main() { v_varying = a_position.x; gl_Position = a_position; }
    );

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, FrontFacingAndVarying)
{
    const std::string vertexShaderSource = SHADER_SOURCE
    (
        attribute vec4 a_position;
        varying float v_varying;
        void main()
        {
            v_varying = a_position.x;
            gl_Position = a_position;
        }
    );

    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;
        varying float v_varying;
        void main()
        {
            vec4 c;

            if (gl_FrontFacing)
            {
                c = vec4(v_varying, 0, 0, 1.0);
            }
            else
            {
                c = vec4(0, v_varying, 0, 1.0);
            }
            gl_FragColor = c;
        }
    );

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, InvariantVaryingIn)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;
        invariant varying float v_varying;
        void main() { gl_FragColor = vec4(v_varying, 0, 0, 1.0); }
    );

    const std::string vertexShaderSource = SHADER_SOURCE
    (
        attribute vec4 a_position;
        varying float v_varying;
        void main() { v_varying = a_position.x; gl_Position = a_position; }
    );

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, InvariantVaryingBoth)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;
        invariant varying float v_varying;
        void main() { gl_FragColor = vec4(v_varying, 0, 0, 1.0); }
    );

    const std::string vertexShaderSource = SHADER_SOURCE
    (
        attribute vec4 a_position;
        invariant varying float v_varying;
        void main() { v_varying = a_position.x; gl_Position = a_position; }
    );

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, InvariantGLPosition)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;
        varying float v_varying;
        void main() { gl_FragColor = vec4(v_varying, 0, 0, 1.0); }
    );

    const std::string vertexShaderSource = SHADER_SOURCE
    (
        attribute vec4 a_position;
        invariant gl_Position;
        varying float v_varying;
        void main() { v_varying = a_position.x; gl_Position = a_position; }
    );

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, InvariantAll)
{
    const std::string fragmentShaderSource = SHADER_SOURCE
    (
        precision mediump float;
        varying float v_varying;
        void main() { gl_FragColor = vec4(v_varying, 0, 0, 1.0); }
    );

    const std::string vertexShaderSource =
        "#pragma STDGL invariant(all)\n"
        "attribute vec4 a_position;\n"
        "varying float v_varying;\n"
        "void main() { v_varying = a_position.x; gl_Position = a_position; }\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, MaxVaryingVec3)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 0, 0, maxVaryings, 0, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, MaxVaryingVec3Array)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 0, 0, 0, maxVaryings / 2, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

// Disabled because of a failure in D3D9
TYPED_TEST(GLSLTest, DISABLED_MaxVaryingVec3AndOneFloat)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(1, 0, 0, 0, maxVaryings, 0, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

// Disabled because of a failure in D3D9
TYPED_TEST(GLSLTest, DISABLED_MaxVaryingVec3ArrayAndOneFloatArray)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 1, 0, 0, 0, maxVaryings / 2, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

// Disabled because of a failure in D3D9
TYPED_TEST(GLSLTest, DISABLED_TwiceMaxVaryingVec2)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 2 * maxVaryings, 0, 0, 0, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

// Disabled because of a failure in D3D9
TYPED_TEST(GLSLTest, DISABLED_MaxVaryingVec2Arrays)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 0, maxVaryings, 0, 0, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}

TYPED_TEST(GLSLTest, MaxPlusOneVaryingVec3)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 0, 0, maxVaryings + 1, 0, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

TYPED_TEST(GLSLTest, MaxPlusOneVaryingVec3Array)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 0, 0, 0, maxVaryings / 2 + 1, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

TYPED_TEST(GLSLTest, MaxVaryingVec3AndOneVec2)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 1, 0, maxVaryings, 0, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

TYPED_TEST(GLSLTest, MaxPlusOneVaryingVec2)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, 0, 2 * maxVaryings + 1, 0, 0, 0, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

TYPED_TEST(GLSLTest, MaxVaryingVec3ArrayAndMaxPlusOneFloatArray)
{
    GLint maxVaryings = 0;
    glGetIntegerv(GL_MAX_VARYING_VECTORS, &maxVaryings);

    std::string fragmentShaderSource;
    std::string vertexShaderSource;

    GenerateGLSLWithVaryings(0, maxVaryings / 2 + 1, 0, 0, 0, maxVaryings / 2, &fragmentShaderSource, &vertexShaderSource);

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_EQ(0u, program);
}

// Verify shader source with a fixed length that is less than the null-terminated length will compile.
TYPED_TEST(GLSLTest, FixedShaderLength)
{
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

    const std::string appendGarbage = "abcasdfasdfasdfasdfasdf";
    const std::string source = "void main() { gl_FragColor = vec4(0, 0, 0, 0); }" + appendGarbage;
    const char *sourceArray[1] = { source.c_str() };
    GLint lengths[1] = { source.length() - appendGarbage.length() };
    glShaderSource(shader, ArraySize(sourceArray), sourceArray, lengths);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
    EXPECT_NE(compileResult, 0);
}

// Verify that a negative shader source length is treated as a null-terminated length.
TYPED_TEST(GLSLTest, NegativeShaderLength)
{
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

    const char *sourceArray[1] = { "void main() { gl_FragColor = vec4(0, 0, 0, 0); }" };
    GLint lengths[1] = { -10 };
    glShaderSource(shader, ArraySize(sourceArray), sourceArray, lengths);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
    EXPECT_NE(compileResult, 0);
}

// Verify that a length array with mixed positive and negative values compiles.
TYPED_TEST(GLSLTest, MixedShaderLengths)
{
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

    const char *sourceArray[] =
    {
        "void main()",
        "{",
        "    gl_FragColor = vec4(0, 0, 0, 0);",
        "}",
    };
    GLint lengths[] =
    {
        -10,
        1,
        std::strlen(sourceArray[2]),
        -1,
    };
    ASSERT_EQ(ArraySize(sourceArray), ArraySize(lengths));

    glShaderSource(shader, ArraySize(sourceArray), sourceArray, lengths);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
    EXPECT_NE(compileResult, 0);
}

// Verify that zero-length shader source does not affect shader compilation.
TYPED_TEST(GLSLTest, ZeroShaderLength)
{
    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

    const char *sourceArray[] =
    {
        "adfasdf",
        "34534",
        "void main() { gl_FragColor = vec4(0, 0, 0, 0); }",
        "",
        "asdfasdfsdsdf",
    };
    GLint lengths[] =
    {
        0,
        0,
        -1,
        0,
        0,
    };
    ASSERT_EQ(ArraySize(sourceArray), ArraySize(lengths));

    glShaderSource(shader, ArraySize(sourceArray), sourceArray, lengths);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);
    EXPECT_NE(compileResult, 0);
}

// Tests that bad index expressions don't crash ANGLE's translator.
// https://code.google.com/p/angleproject/issues/detail?id=857
TYPED_TEST(GLSLTest, BadIndexBug)
{
    const std::string &fragmentShaderSourceVec =
        "precision mediump float;\n"
        "uniform vec4 uniformVec;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(uniformVec[int()]);\n"
        "}";

    GLuint shader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSourceVec);
    EXPECT_EQ(0u, shader);

    if (shader != 0)
    {
        glDeleteShader(shader);
    }

    const std::string &fragmentShaderSourceMat =
        "precision mediump float;\n"
        "uniform mat4 uniformMat;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(uniformMat[int()]);\n"
        "}";

    shader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSourceMat);
    EXPECT_EQ(0u, shader);

    if (shader != 0)
    {
        glDeleteShader(shader);
    }

    const std::string &fragmentShaderSourceArray =
        "precision mediump float;\n"
        "uniform vec4 uniformArray;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(uniformArray[int()]);\n"
        "}";

    shader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSourceArray);
    EXPECT_EQ(0u, shader);

    if (shader != 0)
    {
        glDeleteShader(shader);
    }
}

// Tests that using a global static initialized from a varying works as expected.
// See: https://code.google.com/p/angleproject/issues/detail?id=878
TYPED_TEST(GLSLTest, GlobalStaticAndVarying)
{
    const std::string &vertexShaderSource =
        "attribute vec4 a_position;\n"
        "varying float v;\n"
        "void main() {\n"
        "  gl_Position = a_position;\n"
        "  v = 1.0;\n"
        "}\n";

    const std::string &fragmentShaderSource =
        "precision highp float;\n"
        "varying float v;\n"
        "float x = v;"
        "float global_v = x;"
        "void main() {\n"
        "  gl_FragColor = vec4(global_v, 0.0, 0.0, 1.0);\n"
        "}\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    ASSERT_NE(0u, program);

    drawQuad(program, "a_position", 0.5f);
    swapBuffers();

    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(0, 0, 255, 0, 0, 255);
}

// Tests that using a global static initialized from gl_InstanceID works as expected.
TYPED_TEST(GLSLTest_ES3, GlobalStaticAndInstanceID)
{
    const std::string &vertexShaderSource =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec4 a_position;\n"
        "out vec4 vColour;"
        "int x = gl_InstanceID;"
        "int global_v = x;"
        "void main() {\n"
        "  gl_Position = a_position;\n"
        "  vColour = vec4(float(global_v)/255., 0.0, 0.0, 1.0);\n"
        "}\n";

    const std::string &fragmentShaderSource =
        "#version 300 es\n"
        "precision highp float;\n"
        "in vec4 vColour;"
        "out vec4 colour;"
        "void main() {\n"
        "  colour = vColour;\n"
        "}\n";

    GLuint program = CompileProgram(vertexShaderSource, fragmentShaderSource);
    ASSERT_NE(0u, program);

    GLint positionLocation = glGetAttribLocation(program, "a_position");

    glUseProgram(program);

    const GLfloat vertices[] =
    {
        -1.0f,  1.0f, 0.5f,
        -1.0f, -1.0f, 0.5f,
         1.0f, -1.0f, 0.5f,

        -1.0f,  1.0f, 0.5f,
         1.0f, -1.0f, 0.5f,
         1.0f,  1.0f, 0.5f,
    };

    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(positionLocation);

    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, 7);

    glDisableVertexAttribArray(positionLocation);
    glVertexAttribPointer(positionLocation, 4, GL_FLOAT, GL_FALSE, 0, NULL);

    glUseProgram(0);

    swapBuffers();

    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(0, 0, 6, 0, 0, 255);
}
