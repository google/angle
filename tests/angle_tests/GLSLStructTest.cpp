#include "ANGLETest.h"

class GLSLStructTest : public ANGLETest
{
protected:
    GLSLStructTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

TEST_F(GLSLStructTest, scoped_structs_bug)
{
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

    GLuint program = compileProgram(vertexShaderSource, fragmentShaderSource);
    EXPECT_NE(0u, program);
}
