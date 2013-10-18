#include "ANGLETest.h"

class BufferDataNULLTest : public ANGLETest
{
protected:
    BufferDataNULLTest()
    {
        setWindowWidth(1);
        setWindowHeight(1);
    }
};

TEST_F(BufferDataNULLTest, null_data)
{
    GLuint buf;
    glGenBuffers(1, &buf);
    ASSERT_NE(buf, 0U);

    glBindBuffer(GL_ARRAY_BUFFER, buf);
    EXPECT_GL_NO_ERROR();

    const int numIterations = 128;
    for (int i = 0; i < numIterations; ++i)
    {
        GLsizei bufferSize = sizeof(GLfloat) * (i + 1);
        glBufferData(GL_ARRAY_BUFFER, bufferSize, NULL, GL_STATIC_DRAW);
        EXPECT_GL_NO_ERROR();

        for (int j = 0; j < bufferSize; j++)
        {
            for (int k = 0; k < bufferSize - j; k++)
            {
                glBufferSubData(GL_ARRAY_BUFFER, k, j, NULL);
                EXPECT_GL_NO_ERROR();
            }
        }
    }

    glDeleteBuffers(1, &buf);
    EXPECT_GL_NO_ERROR();
}
