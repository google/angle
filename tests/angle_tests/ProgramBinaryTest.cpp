#include "ANGLETest.h"
#include <memory>
#include <stdint.h>

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(ProgramBinaryTest, ES2_D3D9, ES2_D3D11);

template<typename T>
class ProgramBinaryTest : public ANGLETest
{
  protected:
    ProgramBinaryTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
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

        const std::string vertexShaderSource = SHADER_SOURCE
        (
            attribute vec4 inputAttribute;
            void main()
            {
                gl_Position = inputAttribute;
            }
        );

        const std::string fragmentShaderSource = SHADER_SOURCE
        (
            void main()
            {
                gl_FragColor = vec4(1,0,0,1);
            }
        );

        mProgram = CompileProgram(vertexShaderSource, fragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        glGenBuffers(1, &mBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
        glBufferData(GL_ARRAY_BUFFER, 128, NULL, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteProgram(mProgram);
        glDeleteBuffers(1, &mBuffer);

        ANGLETest::TearDown();
    }

    GLuint mProgram;
    GLuint mBuffer;
};

// This tests the assumption that float attribs of different size
// should not internally cause a vertex shader recompile (for conversion).
TYPED_TEST(ProgramBinaryTest, FloatDynamicShaderSize)
{
    glUseProgram(mProgram);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8, NULL);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINTS, 0, 1);

    GLint programLength;
    glGetProgramiv(mProgram, GL_PROGRAM_BINARY_LENGTH_OES, &programLength);

    EXPECT_GL_NO_ERROR();

    for (GLsizei size = 1; size <= 3; size++)
    {
        glVertexAttribPointer(0, size, GL_FLOAT, GL_FALSE, 8, NULL);
        glEnableVertexAttribArray(0);
        glDrawArrays(GL_POINTS, 0, 1);

        GLint newProgramLength;
        glGetProgramiv(mProgram, GL_PROGRAM_BINARY_LENGTH_OES, &newProgramLength);
        EXPECT_GL_NO_ERROR();
        EXPECT_EQ(programLength, newProgramLength);
    }
}

// This tests the ability to successfully save and load a program binary.
TYPED_TEST(ProgramBinaryTest, SaveAndLoadBinary)
{
    GLint programLength = 0;
    GLint writtenLength = 0;
    GLenum binaryFormat = 0;

    glGetProgramiv(mProgram, GL_PROGRAM_BINARY_LENGTH_OES, &programLength);
    EXPECT_GL_NO_ERROR();

    std::vector<uint8_t> binary(programLength);
    glGetProgramBinaryOES(mProgram, programLength, &writtenLength, &binaryFormat, binary.data());
    EXPECT_GL_NO_ERROR();

    // The lengths reported by glGetProgramiv and glGetProgramBinaryOES should match
    EXPECT_EQ(programLength, writtenLength);

    if (writtenLength)
    {
        GLuint program2 = glCreateProgram();
        glProgramBinaryOES(program2, binaryFormat, binary.data(), writtenLength);

        EXPECT_GL_NO_ERROR();

        GLint linkStatus;
        glGetProgramiv(program2, GL_LINK_STATUS, &linkStatus);
        if (linkStatus == 0)
        {
            GLint infoLogLength;
            glGetProgramiv(program2, GL_INFO_LOG_LENGTH, &infoLogLength);

            if (infoLogLength > 0)
            {
                std::vector<GLchar> infoLog(infoLogLength);
                glGetProgramInfoLog(program2, infoLog.size(), NULL, &infoLog[0]);
                FAIL() << "program link failed: " << &infoLog[0];
            }
            else
            {
                FAIL() << "program link failed.";
            }
        }
        else
        {
            glUseProgram(program2);
            glBindBuffer(GL_ARRAY_BUFFER, mBuffer);

            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 8, NULL);
            glEnableVertexAttribArray(0);
            glDrawArrays(GL_POINTS, 0, 1);

            EXPECT_GL_NO_ERROR();
        }

        glDeleteProgram(program2);
    }
}
