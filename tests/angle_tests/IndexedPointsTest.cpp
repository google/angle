#include "ANGLETest.h"
#include <array>

template <typename IndexType, GLenum IndexTypeName>
class IndexedPointsTest : public ANGLETest
{
protected:
    IndexedPointsTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    float getIndexPositionX(size_t idx)
    {
        return (idx == 0 || idx == 3) ? -0.5f : 0.5f;
    }

    float getIndexPositionY(size_t idx)
    {
        return (idx == 2 || idx == 3) ? -0.5f : 0.5f;
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const std::string vertexShaderSource = SHADER_SOURCE
        (
            precision highp float;
            attribute vec2 position;

            void main()
            {
                gl_PointSize = 5.0;
                gl_Position = vec4(position, 0.0, 1.0);
            }
        );

        const std::string fragmentShaderSource = SHADER_SOURCE
        (
            precision highp float;

            void main()
            {
                gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        );

        mProgram = CompileProgram(vertexShaderSource, fragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        std::array<GLfloat, mPointCount * 2> vertices =
        {
            getIndexPositionX(0), getIndexPositionY(0),
            getIndexPositionX(1), getIndexPositionY(1),
            getIndexPositionX(2), getIndexPositionY(2),
            getIndexPositionX(3), getIndexPositionY(3),
        };
        glGenBuffers(1, &mVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

        std::array<IndexType, mPointCount> indices = { 0, 1, 2, 3 };
        glGenBuffers(1, &mIndexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(IndexType), indices.data(), GL_STATIC_DRAW);
    }

    virtual void TearDown()
    {
        glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    void runTest(GLuint firstIndex)
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLint viewportSize[4];
        glGetIntegerv(GL_VIEWPORT, viewportSize);

        glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
        GLint vertexLocation = glGetAttribLocation(mProgram, "position");
        glVertexAttribPointer(vertexLocation, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(vertexLocation);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

        glUseProgram(mProgram);

        glDrawElements(GL_POINTS, mPointCount - firstIndex, IndexTypeName, reinterpret_cast<void*>(firstIndex * sizeof(IndexType)));

        for (size_t i = 0; i < mPointCount; i++)
        {
            GLuint x = viewportSize[0] + (getIndexPositionX(i) * 0.5f + 0.5f) * (viewportSize[2] - viewportSize[0]);
            GLuint y = viewportSize[1] + (getIndexPositionY(i) * 0.5f + 0.5f) * (viewportSize[3] - viewportSize[1]);

            if (i < firstIndex)
            {
                EXPECT_PIXEL_EQ(x, y, 0, 0, 0, 255);
            }
            else
            {
                EXPECT_PIXEL_EQ(x, y, 255, 0, 0, 255);
            }
        }
    }

    GLuint mProgram;
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;
    static const GLuint mPointCount = 4;
};

typedef IndexedPointsTest<GLubyte, GL_UNSIGNED_BYTE> IndexedPointsTestUByte;

TEST_F(IndexedPointsTestUByte, UnsignedByteOffset0)
{
    runTest(0);
}

TEST_F(IndexedPointsTestUByte, UnsignedByteOffset1)
{
    runTest(1);
}

TEST_F(IndexedPointsTestUByte, UnsignedByteOffset2)
{
    runTest(2);
}

TEST_F(IndexedPointsTestUByte, UnsignedByteOffset3)
{
    runTest(3);
}

typedef IndexedPointsTest<GLushort, GL_UNSIGNED_SHORT> IndexedPointsTestUShort;

TEST_F(IndexedPointsTestUShort, UnsignedShortOffset0)
{
    runTest(0);
}

TEST_F(IndexedPointsTestUShort, UnsignedShortOffset1)
{
    runTest(1);
}

TEST_F(IndexedPointsTestUShort, UnsignedShortOffset2)
{
    runTest(2);
}

TEST_F(IndexedPointsTestUShort, UnsignedShortOffset3)
{
    runTest(3);
}

typedef IndexedPointsTest<GLuint, GL_UNSIGNED_INT> IndexedPointsTestUInt;

TEST_F(IndexedPointsTestUInt, UnsignedIntOffset0)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(0);
}

TEST_F(IndexedPointsTestUInt, UnsignedIntOffset1)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(1);
}

TEST_F(IndexedPointsTestUInt, UnsignedIntOffset2)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(2);
}

TEST_F(IndexedPointsTestUInt, UnsignedIntOffset3)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        return;
    }

    runTest(3);
}
