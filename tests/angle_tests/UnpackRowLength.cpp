#include "ANGLETest.h"

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(UnpackRowLengthTest, ES3_D3D11, ES2_D3D11);

#include <array>

template<typename T>
class UnpackRowLengthTest : public ANGLETest
{
  protected:
    UnpackRowLengthTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);

        mProgram = 0;
    }

    virtual void SetUp() override
    {
        ANGLETest::SetUp();

        const std::string vertexShaderSource = SHADER_SOURCE
        (
            precision highp float;
            attribute vec4 position;

            void main()
            {
                gl_Position = position;
            }
        );

        const std::string fragmentShaderSource = SHADER_SOURCE
        (
            uniform sampler2D tex;

            void main()
            {
                gl_FragColor = texture2D(tex, vec2(0.0, 1.0));
            }
        );

        mProgram = CompileProgram(vertexShaderSource, fragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }
    }

    virtual void TearDown() override
    {
        glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    void testRowLength(int texSize, int rowLength)
    {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);

        if (getClientVersion() == 3)
        {
            // Only texSize * texSize region is filled as WHITE, other parts are BLACK.
            // If the UNPACK_ROW_LENGTH is implemented correctly, all texels inside this texture are WHITE.
            std::vector<GLubyte> buf(rowLength * texSize * 4);
            for (int y = 0; y < texSize; y++)
            {
                std::vector<GLubyte>::iterator rowIter = buf.begin() + y * rowLength * 4;
                std::fill(rowIter, rowIter + texSize * 4, 255);
                std::fill(rowIter + texSize * 4, rowIter + rowLength * 4, 0);
            }

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, &buf[0]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            drawQuad(mProgram, "position", 0.5f);

            EXPECT_PIXEL_EQ(0, 0, 255, 255, 255, 255);
            EXPECT_PIXEL_EQ(1, 0, 255, 255, 255, 255);

            glDeleteTextures(1, &tex);
        }
        else
        {
            EXPECT_GL_ERROR(GL_INVALID_ENUM);
        }
    }

    GLuint mProgram;
};

TYPED_TEST(UnpackRowLengthTest, RowLength128)
{
    testRowLength(128, 128);
}

TYPED_TEST(UnpackRowLengthTest, RowLength1024)
{
    testRowLength(128, 1024);
}
