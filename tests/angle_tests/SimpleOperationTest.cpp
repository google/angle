#include "ANGLETest.h"

#include <vector>

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(SimpleOperationTest, ES2_D3D9, ES2_D3D11, ES3_D3D11, ES2_OPENGL, ES3_OPENGL);

template<typename T>
class SimpleOperationTest : public ANGLETest
{
  protected:
    SimpleOperationTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

TYPED_TEST(SimpleOperationTest, CompileVertexShader)
{
    const std::string source = SHADER_SOURCE
    (
        attribute vec4 a_input;
        void main()
        {
            gl_Position = a_input;
        }
    );

    GLuint shader = CompileShader(GL_VERTEX_SHADER, source);
    EXPECT_NE(shader, 0u);
    glDeleteShader(shader);

    EXPECT_GL_NO_ERROR();
}

TYPED_TEST(SimpleOperationTest, CompileFragmentShader)
{
    const std::string source = SHADER_SOURCE
    (
        precision mediump float;
        varying vec4 v_input;
        void main()
        {
            gl_FragColor = v_input;
        }
    );

    GLuint shader = CompileShader(GL_FRAGMENT_SHADER, source);
    EXPECT_NE(shader, 0u);
    glDeleteShader(shader);

    EXPECT_GL_NO_ERROR();
}
