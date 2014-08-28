#include "ANGLETest.h"

class UniformTest : public ANGLETest
{
  protected:
    UniformTest()
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

        const std::string &vertexShader = "void main() { gl_Position = vec4(1); }";
        const std::string &fragShader =
            "precision mediump float;\n"
            "uniform float uniF;\n"
            "uniform int uniI;\n"
            "void main() { gl_FragColor = vec4(uniF + float(uniI)); }";

        mProgram = CompileProgram(vertexShader, fragShader);
        ASSERT_NE(mProgram, 0u);

        mUniformFLocation = glGetUniformLocation(mProgram, "uniF");
        ASSERT_NE(mUniformFLocation, -1);

        mUniformILocation = glGetUniformLocation(mProgram, "uniI");
        ASSERT_NE(mUniformILocation, -1);

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteProgram(mProgram);
        ANGLETest::TearDown();
    }

    GLuint mProgram;
    GLint mUniformFLocation;
    GLint mUniformILocation;
};

TEST_F(UniformTest, GetUniformNoCurrentProgram)
{
    glUseProgram(mProgram);
    glUniform1f(mUniformFLocation, 1.0f);
    glUniform1i(mUniformILocation, 1);
    glUseProgram(0);

    GLfloat f;
    glGetnUniformfvEXT(mProgram, mUniformFLocation, 4, &f);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1.0f, f);

    glGetUniformfv(mProgram, mUniformFLocation, &f);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1.0f, f);

    GLint i;
    glGetnUniformivEXT(mProgram, mUniformILocation, 4, &i);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1, i);

    glGetUniformiv(mProgram, mUniformILocation, &i);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1, i);
}
