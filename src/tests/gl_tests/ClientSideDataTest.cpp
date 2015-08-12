//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ClientSideDataTest.cpp : Tests that client side data is applied properly

#include "test_utils/ANGLETest.h"

using namespace angle;

namespace
{

class ClientSideDataTest : public ANGLETest
{
  protected:
    ClientSideDataTest()
    {
        setWindowWidth(256);
        setWindowHeight(256);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        const std::string testVertexShaderSource = SHADER_SOURCE
        (
            attribute highp vec4 a_position;
            attribute highp vec4 a_color;

            varying highp vec4 v_color;

            void main(void)
            {
                gl_Position = a_position;
                v_color = a_color;
            }
        );

        const std::string testFragmentShaderSource = SHADER_SOURCE
        (
            varying highp vec4 v_color;
            void main(void)
            {
                gl_FragColor = v_color;
            }
        );

        mProgram = CompileProgram(testVertexShaderSource, testFragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        mPositionAttrib = glGetAttribLocation(mProgram, "a_position");
        mColorAttrib    = glGetAttribLocation(mProgram, "a_color");

        glUseProgram(mProgram);

        glClearColor(0, 0, 0, 0);
        glClearDepthf(0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        // Top middle
        mTestTriangle.push_back(0.0f);
        mTestTriangle.push_back(0.5f);
        mTestTriangle.push_back(0.0f);

        // Bottom left
        mTestTriangle.push_back(-0.5f);
        mTestTriangle.push_back(-0.5f);
        mTestTriangle.push_back(0.0f);

        // Bottom right
        mTestTriangle.push_back(0.5f);
        mTestTriangle.push_back(-0.5f);
        mTestTriangle.push_back(0.0f);

        ASSERT(mTestTriangle.size() == 9);

        // Generate a random color to draw
        mTestColor.push_back(rand() % 255);
        mTestColor.push_back(rand() % 255);
        mTestColor.push_back(rand() % 255);
        mTestColor.push_back(255);
        for (size_t i = 4; i < 12; i++)
        {
            // Copy the test color
            mTestColor.push_back(mTestColor[i - 4]);
        }

        // Test 4 points: just inside each corner and in the middle
        size_t w = getWindowWidth();
        size_t h = getWindowHeight();
        mPixelTestPoints.push_back(std::make_pair(w / 2, ((h / 4) * 3) - 5));
        mPixelTestPoints.push_back(std::make_pair((w / 4) + 5, (h / 4) + 5));
        mPixelTestPoints.push_back(std::make_pair(((w / 4) * 3) - 5, (h / 4) + 5));
        mPixelTestPoints.push_back(std::make_pair(w / 2, h / 2));
    }

    void checkPixels() const
    {
        for (auto location : mPixelTestPoints)
        {
            EXPECT_PIXEL_EQ(location.first, location.second, mTestColor[0], mTestColor[1],
                            mTestColor[2], mTestColor[3]);
        }
    }

    void TearDown() override
    {
        glDeleteProgram(mProgram);

        ANGLETest::TearDown();
    }

    GLuint mProgram;
    GLint mPositionAttrib;
    GLint mColorAttrib;

    std::vector<GLfloat> mTestTriangle;
    std::vector<GLubyte> mTestColor;
    std::vector<std::pair<size_t, size_t>> mPixelTestPoints;
};

// Test a DrawArrays call with client side vertex data
TEST_P(ClientSideDataTest, NonIndexedClientSideVertex)
{
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &mTestTriangle[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    checkPixels();
}

// Test a DrawArrays call with client side vertex data with a non-zero start vertex
TEST_P(ClientSideDataTest, NonIndexedClientSideVertexWithStartVertex)
{
    // Create a copy of the vertices and offset them
    std::vector<GLfloat> triangleCopy(mTestTriangle);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &triangleCopy[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    std::vector<GLubyte> colorCopy(mTestColor);
    colorCopy.insert(colorCopy.begin(), 50);
    colorCopy.insert(colorCopy.begin(), 30);
    colorCopy.insert(colorCopy.begin(), 20);
    colorCopy.insert(colorCopy.begin(), 10);
    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &colorCopy[0]);
    glEnableVertexAttribArray(mColorAttrib);

    glDrawArrays(GL_TRIANGLES, 1, 3);

    checkPixels();
}

// Test a DrawArrays call with client side and buffered vertex data
TEST_P(ClientSideDataTest, NonIndexedClientSideVertexAndBufferVertex)
{
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    checkPixels();
}

// Test a DrawArrays call with client side and buffered vertex data with a buffer offset
TEST_P(ClientSideDataTest, NonIndexedClientSideVertexAndBufferVertexWithOffset)
{
    std::vector<GLfloat> triangleCopy(mTestTriangle);
    triangleCopy.insert(triangleCopy.begin(), 50.0f);

    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, triangleCopy.size() * sizeof(GLfloat), &triangleCopy[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<const GLvoid *>(sizeof(GLfloat)));
    glEnableVertexAttribArray(mPositionAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    checkPixels();
}

// Test a DrawArrays call with client side and buffered vertex data with a buffer offset and
// non-zero first vertex
TEST_P(ClientSideDataTest, NonIndexedClientSideVertexAndBufferVertexWithOffsetAndStartVertex)
{
    std::vector<GLfloat> triangleCopy(mTestTriangle);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);

    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, triangleCopy.size() * sizeof(GLfloat), &triangleCopy[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<const GLvoid *>(sizeof(GLfloat)));
    glEnableVertexAttribArray(mPositionAttrib);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    std::vector<GLubyte> colorCopy(mTestColor);
    colorCopy.insert(colorCopy.begin(), 50);
    colorCopy.insert(colorCopy.begin(), 30);
    colorCopy.insert(colorCopy.begin(), 20);
    colorCopy.insert(colorCopy.begin(), 10);
    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &colorCopy[0]);
    glEnableVertexAttribArray(mColorAttrib);

    glDrawArrays(GL_TRIANGLES, 1, 3);

    checkPixels();
}

// Test a DrawElements call with client side vertex and index data
TEST_P(ClientSideDataTest, ByteIndexedClientSideVertexAndClientSideIndices)
{
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &mTestTriangle[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLubyte indices[3] = {0, 1, 2};

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, indices);

    checkPixels();
}

// Test a DrawElements call with client side vertex and buffered index data
TEST_P(ClientSideDataTest, ByteIndexedClientSideVertexAndBufferedIndices)
{
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &mTestTriangle[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLubyte indices[3] = {0, 1, 2};
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, nullptr);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered index data
TEST_P(ClientSideDataTest, ByteIndexedClientSideVertexAndBufferVertexAndBufferedIndices)
{
    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLubyte indices[3] = {0, 1, 2};
    GLuint ib = 0;
    glGenBuffers(1, &ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, nullptr);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, ByteIndexedClientSideVertexAndBufferVertexAndBufferedOffsetIndices)
{
    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLubyte indices[7] = {2, 4, 2, 5, 0, 1, 2};
    GLuint ib = 0;
    glGenBuffers(1, &ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE,
                   reinterpret_cast<const GLvoid *>(sizeof(GLubyte) * 4));

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, ByteIndexedClientSideVertexAndBufferVertexAndClientSideIndices)
{
    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLubyte indices[3] = {0, 1, 2};
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, indices);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, ByteIndexedClientSideVertexAndBufferOffsetVertexAndClientSideIndices)
{
    std::vector<GLfloat> triangleCopy(mTestTriangle);
    triangleCopy.insert(triangleCopy.begin(), 50.0f);

    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, triangleCopy.size() * sizeof(GLfloat), &triangleCopy[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<const GLvoid *>(sizeof(GLfloat)));
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLubyte indices[3] = {0, 1, 2};
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, indices);

    checkPixels();
}

// Test a DrawElements call with client side vertex and index data
TEST_P(ClientSideDataTest, ShortIndexedClientSideVertexAndClientSideIndices)
{
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &mTestTriangle[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLushort indices[3] = {0, 1, 2};

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, indices);

    checkPixels();
}

// Test a DrawElements call with client side vertex and buffered index data
TEST_P(ClientSideDataTest, ShortIndexedClientSideVertexAndBufferedIndices)
{
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &mTestTriangle[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLushort indices[3] = {0, 1, 2};
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered index data
TEST_P(ClientSideDataTest, ShortIndexedClientSideVertexAndBufferVertexAndBufferedIndices)
{
    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLushort indices[3] = {0, 1, 2};
    GLuint ib = 0;
    glGenBuffers(1, &ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, ShortIndexedClientSideVertexAndBufferVertexAndBufferedOffsetIndices)
{
    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLushort indices[7] = {2, 4, 2, 5, 0, 1, 2};
    GLuint ib = 0;
    glGenBuffers(1, &ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
                   reinterpret_cast<const GLvoid *>(sizeof(GLushort) * 4));

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, ShortIndexedClientSideVertexAndBufferVertexAndClientSideIndices)
{
    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLushort indices[3] = {0, 1, 2};
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, indices);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, UshortIndexedClientSideVertexAndBufferOffsetVertexAndClientSideIndices)
{
    std::vector<GLfloat> triangleCopy(mTestTriangle);
    triangleCopy.insert(triangleCopy.begin(), 0.0f);

    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, triangleCopy.size() * sizeof(GLfloat), &triangleCopy[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<const GLvoid *>(sizeof(GLfloat)));
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLushort indices[3] = {0, 1, 2};
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, indices);

    checkPixels();
}

// Test a DrawElements call with client side vertex and index data
TEST_P(ClientSideDataTest, UintIndexedClientSideVertexAndClientSideIndices)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        std::cout << "Test skipped because ES3 or GL_OES_element_index_uint is not available."
                  << std::endl;
        return;
    }

    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &mTestTriangle[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLuint indices[3] = {0, 1, 2};

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, indices);

    checkPixels();
}

// Test a DrawElements call with client side vertex and buffered index data
TEST_P(ClientSideDataTest, UintIndexedClientSideVertexAndBufferedIndices)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        std::cout << "Test skipped because ES3 or GL_OES_element_index_uint is not available."
                  << std::endl;
        return;
    }

    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, &mTestTriangle[0]);
    glEnableVertexAttribArray(mPositionAttrib);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLuint indices[3] = {0, 1, 2};
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered index data
TEST_P(ClientSideDataTest, UintIndexedClientSideVertexAndBufferVertexAndBufferedIndices)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        std::cout << "Test skipped because ES3 or GL_OES_element_index_uint is not available."
                  << std::endl;
        return;
    }

    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLuint indices[3] = {0, 1, 2};
    GLuint ib = 0;
    glGenBuffers(1, &ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, nullptr);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, UintIndexedClientSideVertexAndBufferVertexAndBufferedOffsetIndices)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        std::cout << "Test skipped because ES3 or GL_OES_element_index_uint is not available."
                  << std::endl;
        return;
    }

    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLuint indices[7] = {2, 4, 2, 5, 0, 1, 2};
    GLuint ib = 0;
    glGenBuffers(1, &ib);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT,
                   reinterpret_cast<const GLvoid *>(sizeof(GLuint) * 4));

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, UintIndexedClientSideVertexAndBufferVertexAndClientSideIndices)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        std::cout << "Test skipped because ES3 or GL_OES_element_index_uint is not available."
                  << std::endl;
        return;
    }

    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, mTestTriangle.size() * sizeof(GLfloat), &mTestTriangle[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLuint indices[3] = {0, 1, 2};
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, indices);

    checkPixels();
}

// Test a DrawElements call with buffered and client side vertex data and buffered, offset index
// data
TEST_P(ClientSideDataTest, UintIndexedClientSideVertexAndBufferOffsetVertexAndClientSideIndices)
{
    if (getClientVersion() < 3 && !extensionEnabled("GL_OES_element_index_uint"))
    {
        std::cout << "Test skipped because ES3 or GL_OES_element_index_uint is not available."
                  << std::endl;
        return;
    }

    std::vector<GLfloat> triangleCopy(mTestTriangle);
    triangleCopy.insert(triangleCopy.begin(), 50.0f);

    GLuint vb = 0;
    glGenBuffers(1, &vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glBufferData(GL_ARRAY_BUFFER, triangleCopy.size() * sizeof(GLfloat), &triangleCopy[0],
                 GL_STATIC_DRAW);
    glVertexAttribPointer(mPositionAttrib, 3, GL_FLOAT, GL_FALSE, 0,
                          reinterpret_cast<const GLvoid *>(sizeof(GLfloat)));
    glEnableVertexAttribArray(mPositionAttrib);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glVertexAttribPointer(mColorAttrib, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, &mTestColor[0]);
    glEnableVertexAttribArray(mColorAttrib);

    GLuint indices[3] = {0, 1, 2};
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, indices);

    checkPixels();
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST(ClientSideDataTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES3_OPENGL());

}  // namespace
