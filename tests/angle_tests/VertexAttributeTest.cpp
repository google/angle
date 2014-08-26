#include "ANGLETest.h"

class VertexAttributeTest : public ANGLETest
{
protected:
    VertexAttributeTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);

        mProgram = 0;
        mTestAttrib = -1;
        mExpectedAttrib = -1;
    }

    struct TestData
    {
        GLenum type;
        GLboolean normalized;

        const void *inputData;
        const GLfloat *expectedData;
    };

    void runTest(const TestData& test)
    {
        GLint viewportSize[4];
        glGetIntegerv(GL_VIEWPORT, viewportSize);

        GLint midPixelX = (viewportSize[0] + viewportSize[2]) / 2;
        GLint midPixelY = (viewportSize[1] + viewportSize[3]) / 2;

        for (size_t i = 0; i < 4; i++)
        {
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glVertexAttribPointer(mTestAttrib, i + 1, test.type, test.normalized, 0, test.inputData);
            glVertexAttribPointer(mExpectedAttrib, i + 1, GL_FLOAT, GL_FALSE, 0, test.expectedData);

            glEnableVertexAttribArray(mTestAttrib);
            glEnableVertexAttribArray(mExpectedAttrib);

            drawQuad(mProgram, "position", 0.5f);

            glDisableVertexAttribArray(mTestAttrib);
            glDisableVertexAttribArray(mExpectedAttrib);

            // We need to offset our checks from triangle edges to ensure we don't fall on a single tri
            // Avoid making assumptions of drawQuad with four checks to check the four possible tri regions
            EXPECT_PIXEL_EQ((midPixelX + viewportSize[0]) / 2, midPixelY, 255, 255, 255, 255);
            EXPECT_PIXEL_EQ((midPixelX + viewportSize[2]) / 2, midPixelY, 255, 255, 255, 255);
            EXPECT_PIXEL_EQ(midPixelX, (midPixelY + viewportSize[1]) / 2, 255, 255, 255, 255);
            EXPECT_PIXEL_EQ(midPixelX, (midPixelY + viewportSize[3]) / 2, 255, 255, 255, 255);
        }
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const std::string testVertexShaderSource = SHADER_SOURCE
        (
            attribute highp vec4 position;
            attribute highp vec4 test;
            attribute highp vec4 expected;

            varying highp vec4 color;

            void main(void)
            {
                gl_Position = position;
                color = vec4(lessThan(abs(test - expected), vec4(1.0 / 64.0)));
            }
        );

        const std::string testFragmentShaderSource = SHADER_SOURCE
        (
            varying highp vec4 color;
            void main(void)
            {
                gl_FragColor = color;
            }
        );

        mProgram = CompileProgram(testVertexShaderSource, testFragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        mTestAttrib = glGetAttribLocation(mProgram, "test");
        mExpectedAttrib = glGetAttribLocation(mProgram, "expected");

        glUseProgram(mProgram);

        glClearColor(0, 0, 0, 0);
        glClearDepthf(0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);
    }

    virtual void TearDown()
    {
        glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    static const size_t mVertexCount = 24;

    GLuint mProgram;
    GLint mTestAttrib;
    GLint mExpectedAttrib;
};

TEST_F(VertexAttributeTest, UnsignedByteUnnormalized)
{
    GLubyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, 5, 6, 7, 125, 126, 127, 128, 129, 250, 251, 252, 253, 254, 255 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = { GL_UNSIGNED_BYTE, GL_FALSE, inputData, expectedData };
    runTest(data);
}

TEST_F(VertexAttributeTest, UnsignedByteNormalized)
{
    GLubyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, 5, 6, 7, 125, 126, 127, 128, 129, 250, 251, 252, 253, 254, 255 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i] / 255.0f;
    }

    TestData data = { GL_UNSIGNED_BYTE, GL_TRUE, inputData, expectedData };
    runTest(data);
}

TEST_F(VertexAttributeTest, ByteUnnormalized)
{
    GLbyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, -1, -2, -3, -4, 125, 126, 127, -128, -127, -126 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = { GL_BYTE, GL_FALSE, inputData, expectedData };
    runTest(data);
}

TEST_F(VertexAttributeTest, ByteNormalized)
{
    GLbyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, -1, -2, -3, -4, 125, 126, 127, -128, -127, -126 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = ((2.0f * inputData[i]) + 1.0f) / 255.0f;
    }

    TestData data = { GL_BYTE, GL_TRUE, inputData, expectedData };
    runTest(data);
}

TEST_F(VertexAttributeTest, UnsignedShortUnnormalized)
{
    GLushort inputData[mVertexCount] = { 0, 1, 2, 3, 254, 255, 256, 32766, 32767, 32768, 65533, 65534, 65535 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = { GL_UNSIGNED_SHORT, GL_FALSE, inputData, expectedData };
    runTest(data);
}

TEST_F(VertexAttributeTest, UnsignedShortNormalized)
{
    GLushort inputData[mVertexCount] = { 0, 1, 2, 3, 254, 255, 256, 32766, 32767, 32768, 65533, 65534, 65535 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i] / 65535.0f;
    }

    TestData data = { GL_UNSIGNED_SHORT, GL_TRUE, inputData, expectedData };
    runTest(data);
}

TEST_F(VertexAttributeTest, ShortUnnormalized)
{
    GLshort inputData[mVertexCount] = {  0, 1, 2, 3, -1, -2, -3, -4, 32766, 32767, -32768, -32767, -32766 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = { GL_SHORT, GL_FALSE, inputData, expectedData };
    runTest(data);
}

TEST_F(VertexAttributeTest, ShortNormalized)
{
    GLshort inputData[mVertexCount] = {  0, 1, 2, 3, -1, -2, -3, -4, 32766, 32767, -32768, -32767, -32766 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = ((2.0f * inputData[i]) + 1.0f) / 65535.0f;
    }

    TestData data = { GL_SHORT, GL_TRUE, inputData, expectedData };
    runTest(data);
}
