//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"

using namespace angle;

namespace
{

GLsizei TypeStride(GLenum attribType)
{
    switch (attribType)
    {
        case GL_UNSIGNED_BYTE:
        case GL_BYTE:
            return 1;
        case GL_UNSIGNED_SHORT:
        case GL_SHORT:
            return 2;
        case GL_UNSIGNED_INT:
        case GL_INT:
        case GL_FLOAT:
            return 4;
        default:
            UNREACHABLE();
            return 0;
    }
}

class VertexAttributeTest : public ANGLETest
{
  protected:
    VertexAttributeTest() : mProgram(0), mTestAttrib(-1), mExpectedAttrib(-1), mBuffer(0)
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    enum class Source
    {
        BUFFER,
        IMMEDIATE,
    };

    struct TestData
    {
        GLenum type;
        GLboolean normalized;
        Source source;

        const void *inputData;
        const GLfloat *expectedData;
    };

    void runTest(const TestData &test)
    {
        // TODO(geofflang): Figure out why this is broken on AMD OpenGL
        if (IsAMD() && getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE)
        {
            std::cout << "Test skipped on AMD OpenGL." << std::endl;
            return;
        }

        GLint viewportSize[4];
        glGetIntegerv(GL_VIEWPORT, viewportSize);

        GLint midPixelX = (viewportSize[0] + viewportSize[2]) / 2;
        GLint midPixelY = (viewportSize[1] + viewportSize[3]) / 2;

        for (GLint i = 0; i < 4; i++)
        {
            GLint typeSize = i + 1;

            if (test.source == Source::BUFFER)
            {
                GLsizei dataSize = mVertexCount * TypeStride(test.type) * typeSize;
                glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
                glBufferData(GL_ARRAY_BUFFER, dataSize, test.inputData, GL_STATIC_DRAW);
                glVertexAttribPointer(mTestAttrib, typeSize, test.type, test.normalized, 0,
                                      nullptr);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
            }
            else
            {
                ASSERT_EQ(Source::IMMEDIATE, test.source);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glVertexAttribPointer(mTestAttrib, typeSize, test.type, test.normalized, 0,
                                      test.inputData);
            }

            glVertexAttribPointer(mExpectedAttrib, typeSize, GL_FLOAT, GL_FALSE, 0,
                                  test.expectedData);

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

    void SetUp() override
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
                vec4 threshold = max(abs(expected) * 0.01, 1.0 / 64.0);
                color          = vec4(lessThanEqual(abs(test - expected), threshold));
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

        glGenBuffers(1, &mBuffer);
    }

    void TearDown() override
    {
        glDeleteProgram(mProgram);
        glDeleteBuffers(1, &mBuffer);

        ANGLETest::TearDown();
    }

    GLuint compileMultiAttribProgram(GLint attribCount)
    {
        std::stringstream shaderStream;

        shaderStream << "attribute highp vec4 position;" << std::endl;
        for (GLint attribIndex = 0; attribIndex < attribCount; ++attribIndex)
        {
            shaderStream << "attribute float a" << attribIndex << ";" << std::endl;
        }
        shaderStream << "varying highp float color;" << std::endl
                     << "void main() {" << std::endl
                     << "  gl_Position = position;" << std::endl
                     << "  color = 0.0;" << std::endl;
        for (GLint attribIndex = 0; attribIndex < attribCount; ++attribIndex)
        {
            shaderStream << "  color += a" << attribIndex << ";" << std::endl;
        }
        shaderStream << "}" << std::endl;

        const std::string testFragmentShaderSource = SHADER_SOURCE
        (
            varying highp float color;
            void main(void)
            {
                gl_FragColor = vec4(color, 0.0, 0.0, 1.0);
            }
        );

        return CompileProgram(shaderStream.str(), testFragmentShaderSource);
    }

    void setupMultiAttribs(GLuint program, GLint attribCount, GLfloat value)
    {
        glUseProgram(program);
        for (GLint attribIndex = 0; attribIndex < attribCount; ++attribIndex)
        {
            std::stringstream attribStream;
            attribStream << "a" << attribIndex;
            GLint location = glGetAttribLocation(program, attribStream.str().c_str());
            ASSERT_NE(-1, location);
            glVertexAttrib1f(location, value);
            glDisableVertexAttribArray(location);
        }
    }

    static const size_t mVertexCount = 24;

    GLuint mProgram;
    GLint mTestAttrib;
    GLint mExpectedAttrib;
    GLuint mBuffer;
};

TEST_P(VertexAttributeTest, UnsignedByteUnnormalized)
{
    GLubyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, 5, 6, 7, 125, 126, 127, 128, 129, 250, 251, 252, 253, 254, 255 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = {GL_UNSIGNED_BYTE, GL_FALSE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTest, UnsignedByteNormalized)
{
    GLubyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, 5, 6, 7, 125, 126, 127, 128, 129, 250, 251, 252, 253, 254, 255 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i] / 255.0f;
    }

    TestData data = {GL_UNSIGNED_BYTE, GL_TRUE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTest, ByteUnnormalized)
{
    GLbyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, -1, -2, -3, -4, 125, 126, 127, -128, -127, -126 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = {GL_BYTE, GL_FALSE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTest, ByteNormalized)
{
    GLbyte inputData[mVertexCount] = { 0, 1, 2, 3, 4, -1, -2, -3, -4, 125, 126, 127, -128, -127, -126 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = ((2.0f * inputData[i]) + 1.0f) / 255.0f;
    }

    TestData data = {GL_BYTE, GL_TRUE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTest, UnsignedShortUnnormalized)
{
    GLushort inputData[mVertexCount] = { 0, 1, 2, 3, 254, 255, 256, 32766, 32767, 32768, 65533, 65534, 65535 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = {GL_UNSIGNED_SHORT, GL_FALSE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTest, UnsignedShortNormalized)
{
    GLushort inputData[mVertexCount] = { 0, 1, 2, 3, 254, 255, 256, 32766, 32767, 32768, 65533, 65534, 65535 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i] / 65535.0f;
    }

    TestData data = {GL_UNSIGNED_SHORT, GL_TRUE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTest, ShortUnnormalized)
{
    GLshort inputData[mVertexCount] = {  0, 1, 2, 3, -1, -2, -3, -4, 32766, 32767, -32768, -32767, -32766 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = inputData[i];
    }

    TestData data = {GL_SHORT, GL_FALSE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTest, ShortNormalized)
{
    GLshort inputData[mVertexCount] = {  0, 1, 2, 3, -1, -2, -3, -4, 32766, 32767, -32768, -32767, -32766 };
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = ((2.0f * inputData[i]) + 1.0f) / 65535.0f;
    }

    TestData data = {GL_SHORT, GL_TRUE, Source::IMMEDIATE, inputData, expectedData};
    runTest(data);
}

class VertexAttributeTestES3 : public VertexAttributeTest
{
  protected:
    VertexAttributeTestES3() {}
};

TEST_P(VertexAttributeTestES3, IntUnnormalized)
{
    GLint lo                      = std::numeric_limits<GLint>::min();
    GLint hi                      = std::numeric_limits<GLint>::max();
    GLint inputData[mVertexCount] = {0, 1, 2, 3, -1, -2, -3, -4, -1, hi, hi - 1, lo, lo + 1};
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = static_cast<GLfloat>(inputData[i]);
    }

    TestData data = {GL_INT, GL_FALSE, Source::BUFFER, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTestES3, IntNormalized)
{
    GLint lo                      = std::numeric_limits<GLint>::min();
    GLint hi                      = std::numeric_limits<GLint>::max();
    GLint inputData[mVertexCount] = {0, 1, 2, 3, -1, -2, -3, -4, -1, hi, hi - 1, lo, lo + 1};
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = ((2.0f * inputData[i]) + 1.0f) / static_cast<GLfloat>(0xFFFFFFFFu);
    }

    TestData data = {GL_INT, GL_TRUE, Source::BUFFER, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTestES3, UnsignedIntUnnormalized)
{
    GLuint mid                     = std::numeric_limits<GLuint>::max() >> 1;
    GLuint hi                      = std::numeric_limits<GLuint>::max();
    GLuint inputData[mVertexCount] = {0,       1,   2,       3,      254,    255, 256,
                                      mid - 1, mid, mid + 1, hi - 2, hi - 1, hi};
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = static_cast<GLfloat>(inputData[i]);
    }

    TestData data = {GL_UNSIGNED_INT, GL_FALSE, Source::BUFFER, inputData, expectedData};
    runTest(data);
}

TEST_P(VertexAttributeTestES3, UnsignedIntNormalized)
{
    GLuint mid                     = std::numeric_limits<GLuint>::max() >> 1;
    GLuint hi                      = std::numeric_limits<GLuint>::max();
    GLuint inputData[mVertexCount] = {0,       1,   2,       3,      254,    255, 256,
                                      mid - 1, mid, mid + 1, hi - 2, hi - 1, hi};
    GLfloat expectedData[mVertexCount];
    for (size_t i = 0; i < mVertexCount; i++)
    {
        expectedData[i] = static_cast<GLfloat>(inputData[i]) / static_cast<GLfloat>(hi);
    }

    TestData data = {GL_UNSIGNED_INT, GL_TRUE, Source::BUFFER, inputData, expectedData};
    runTest(data);
}

// Validate that we can support GL_MAX_ATTRIBS attribs
TEST_P(VertexAttributeTest, MaxAttribs)
{
    // TODO(jmadill): Figure out why we get this error on AMD/OpenGL and Intel.
    if ((IsIntel() || IsAMD()) && GetParam().getRenderer() == EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE)
    {
        std::cout << "Test skipped on Intel and AMD." << std::endl;
        return;
    }

    GLint maxAttribs;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    ASSERT_GL_NO_ERROR();

    // Reserve one attrib for position
    GLint drawAttribs = maxAttribs - 1;

    GLuint program = compileMultiAttribProgram(drawAttribs);
    ASSERT_NE(0u, program);

    setupMultiAttribs(program, drawAttribs, 0.5f / static_cast<float>(drawAttribs));
    drawQuad(program, "position", 0.5f);

    EXPECT_GL_NO_ERROR();
    EXPECT_PIXEL_NEAR(0, 0, 128, 0, 0, 255, 1);
}

// Validate that we cannot support GL_MAX_ATTRIBS+1 attribs
TEST_P(VertexAttributeTest, MaxAttribsPlusOne)
{
    // TODO(jmadill): Figure out why we get this error on AMD/ES2/OpenGL
    if (IsAMD() && GetParam() == ES2_OPENGL())
    {
        std::cout << "Test disabled on AMD/ES2/OpenGL" << std::endl;
        return;
    }

    GLint maxAttribs;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxAttribs);
    ASSERT_GL_NO_ERROR();

    // Exceed attrib count by one (counting position)
    GLint drawAttribs = maxAttribs;

    GLuint program = compileMultiAttribProgram(drawAttribs);
    ASSERT_EQ(0u, program);
}

// Simple test for when we use glBindAttribLocation
TEST_P(VertexAttributeTest, SimpleBindAttribLocation)
{
    // TODO(jmadill): Figure out why this fails on Intel.
    if (IsIntel() && GetParam().getRenderer() == EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE)
    {
        std::cout << "Test skipped on Intel." << std::endl;
        return;
    }

    // Re-use the multi-attrib program, binding attribute 0
    GLuint program = compileMultiAttribProgram(1);
    glBindAttribLocation(program, 2, "position");
    glBindAttribLocation(program, 3, "a0");
    glLinkProgram(program);

    // Setup and draw the quad
    setupMultiAttribs(program, 1, 0.5f);
    drawQuad(program, "position", 0.5f);
    EXPECT_GL_NO_ERROR();
    EXPECT_PIXEL_NEAR(0, 0, 128, 0, 0, 255, 1);
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
// D3D11 Feature Level 9_3 uses different D3D formats for vertex attribs compared to Feature Levels 10_0+, so we should test them separately.
ANGLE_INSTANTIATE_TEST(VertexAttributeTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES3_OPENGL(),
                       ES2_OPENGLES(),
                       ES3_OPENGLES());

ANGLE_INSTANTIATE_TEST(VertexAttributeTestES3, ES3_D3D11(), ES3_OPENGL(), ES3_OPENGLES());

}  // anonymous namespace
