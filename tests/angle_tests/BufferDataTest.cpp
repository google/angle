#include "ANGLETest.h"

class BufferDataTest : public ANGLETest
{
protected:
    BufferDataTest()
        : mBuffer(0),
          mProgram(0),
          mAttribLocation(-1)
    {
        setWindowWidth(16);
        setWindowHeight(16);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const char * vsSource = SHADER_SOURCE
        (
            attribute vec4 position;
            attribute float in_attrib;
            varying float v_attrib;
            void main()
            {
                v_attrib = in_attrib;
                gl_Position = position;
            }
        );

        const char * fsSource = SHADER_SOURCE
        (
            precision mediump float;
            varying float v_attrib;
            void main()
            {
                gl_FragColor = vec4(v_attrib, 0, 0, 1);
            }
        );

        glGenBuffers(1, &mBuffer);
        ASSERT_NE(mBuffer, 0U);

        mProgram = compileProgram(vsSource, fsSource);
        ASSERT_NE(mProgram, 0U);

        mAttribLocation = glGetAttribLocation(mProgram, "in_attrib");
        ASSERT_NE(mAttribLocation, -1);

        glClearColor(0, 0, 0, 0);
        glClearDepthf(0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteBuffers(1, &mBuffer);
        glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    GLuint mBuffer;
    GLuint mProgram;
    GLint mAttribLocation;
};

TEST_F(BufferDataTest, null_data)
{
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
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
}

TEST_F(BufferDataTest, zero_nonnull_data)
{
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    EXPECT_GL_NO_ERROR();

    char *zeroData = new char[0];
    glBufferData(GL_ARRAY_BUFFER, 0, zeroData, GL_STATIC_DRAW);
    EXPECT_GL_NO_ERROR();

    glBufferSubData(GL_ARRAY_BUFFER, 0, 0, zeroData);
    EXPECT_GL_NO_ERROR();

    delete [] zeroData;
}

TEST_F(BufferDataTest, huge_setdata_should_not_crash)
{
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    EXPECT_GL_NO_ERROR();

    // use as large a size as possible without causing an exception
    GLsizei hugeSize = (1 << 30);

    // on x64, use as large a GLsizei value as possible
    if (sizeof(size_t) > 4)
    {
        hugeSize = std::numeric_limits<GLsizei>::max();
    }

    char *data = new (std::nothrow) char[hugeSize];
    EXPECT_NE((char * const)NULL, data);

    if (data == NULL)
    {
        return;
    }

    memset(data, 0, hugeSize);

    float * fValue = reinterpret_cast<float*>(data);
    for (unsigned int f = 0; f < 6; f++)
    {
        fValue[f] = 1.0f;
    }

    glBufferData(GL_ARRAY_BUFFER, hugeSize, data, GL_STATIC_DRAW);

    GLenum error = glGetError();
    if (error == GL_NO_ERROR)
    {
        // If we didn't fail because of an out of memory error, try drawing a quad
        // using the large buffer

        // DISABLED because it takes a long time, but left for posterity

        //glUseProgram(mProgram);
        //glVertexAttribPointer(mAttribLocation, 1, GL_FLOAT, GL_FALSE, 4, NULL);
        //glEnableVertexAttribArray(mAttribLocation);
        //glBindBuffer(GL_ARRAY_BUFFER, 0);
        //drawQuad(mProgram, "position", 0.5f);
        //swapBuffers();

        //// Draw operations can also generate out-of-memory, which is in-spec
        //error = glGetError();
        //if (error == GL_NO_ERROR)
        //{
        //    GLint viewportSize[4];
        //    glGetIntegerv(GL_VIEWPORT, viewportSize);

        //    GLint midPixelX = (viewportSize[0] + viewportSize[2]) / 2;
        //    GLint midPixelY = (viewportSize[1] + viewportSize[3]) / 2;

        //    EXPECT_PIXEL_EQ(midPixelX, midPixelY, 255, 0, 0, 255);
        //}
        //else
        //{
        //    EXPECT_EQ(GL_OUT_OF_MEMORY, error);
        //}
    }
    else
    {
        EXPECT_EQ(GL_OUT_OF_MEMORY, error);
    }

    delete[] data;
}

