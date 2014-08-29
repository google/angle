//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "BufferSubData.h"

#include <cassert>
#include <sstream>

std::string BufferSubDataParams::name() const
{
    std::stringstream strstr;

    strstr << "BufferSubData - ";

    switch (requestedRenderer)
    {
      case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE: strstr << "D3D11"; break;
      case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE: strstr << "D3D9"; break;
      default: strstr << "UNKNOWN RENDERER (" << requestedRenderer << ")"; break;
    }

    strstr << " - ";

    switch (vertexType)
    {
      case GL_FLOAT: strstr << "Float"; break;
      case GL_INT: strstr << "Int"; break;
      case GL_BYTE: strstr << "Byte"; break;
      case GL_SHORT: strstr << "Short"; break;
      case GL_UNSIGNED_INT: strstr << "UInt"; break;
      case GL_UNSIGNED_BYTE: strstr << "UByte"; break;
      case GL_UNSIGNED_SHORT: strstr << "UShort"; break;
      default: strstr << "UNKNOWN FORMAT (" << vertexType << ")"; break;
    }

    strstr << vertexComponentCount;

    strstr << " - " << updateSize << "b updates - ";
    strstr << (bufferSize >> 10) << "k buffer - ";
    strstr << iterations << " updates";

    return strstr.str();
}

BufferSubDataBenchmark::BufferSubDataBenchmark(const BufferSubDataParams &params)
    : SimpleBenchmark(params.name(), 1280, 720, 2, params.requestedRenderer),
      mParams(params)
{
    mDrawIterations = mParams.iterations;

    assert(mParams.vertexComponentCount > 1);
    assert(mParams.iterations > 0);
}

bool BufferSubDataBenchmark::initializeBenchmark()
{
    const std::string vs = SHADER_SOURCE
    (
        attribute vec4 vPosition;
        void main()
        {
            gl_Position = vPosition;
        }
    );

    const std::string fs = SHADER_SOURCE
    (
        precision mediump float;
        void main()
        {
            gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
        }
    );

    mProgram = CompileProgram(vs, fs);
    if (!mProgram)
    {
        return false;
    }

    // Use the program object
    glUseProgram(mProgram);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glGenBuffers(1, &mBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glBufferData(GL_ARRAY_BUFFER, mParams.bufferSize, 0, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, mParams.vertexComponentCount, mParams.vertexType,
                          mParams.vertexNormalized, 0, 0);
    glEnableVertexAttribArray(0);

    mUpdateData = new uint8_t[mParams.updateSize];

    GLfloat vertices2[] =
    {
        0.0f, 0.5f,
        -0.5f, -0.5f,
        0.5f, -0.5f,
    };

    GLfloat vertices3[] =
    {
        0.0f, 0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
    };

    GLfloat vertices4[] =
    {
        0.0f, 0.5f, 0.0f, 1.0f,
        -0.5f, -0.5f, 0.0f, 1.0f,
        0.5f, -0.5f, 0.0f, 1.0f,
    };

    float *vertexData = NULL;

    switch (mParams.vertexComponentCount)
    {
      case 2: vertexData = vertices2; break;
      case 3: vertexData = vertices3; break;
      case 4: vertexData = vertices4; break;
      default: break;
    }

    assert(vertexData != NULL);

    GLsizeiptr vertexDataSize = sizeof(GLfloat) * mParams.vertexComponentCount;
    GLsizeiptr triDataSize = vertexDataSize * 3;
    mNumTris = mParams.updateSize / triDataSize;
    int offset = 0;
    for (int i = 0; i < mNumTris; ++i)
    {
        memcpy(mUpdateData + offset, vertexData, triDataSize);
        offset += triDataSize;
    }

    // Set the viewport
    glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());

    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR)
    {
        return false;
    }

    return true;
}

void BufferSubDataBenchmark::destroyBenchmark()
{
    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mBuffer);
    delete[] mUpdateData;
}

void BufferSubDataBenchmark::beginDrawBenchmark()
{
    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);
}

void BufferSubDataBenchmark::drawBenchmark()
{
    for (unsigned int it = 0; it < mParams.iterations; it++)
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, mParams.updateSize, mUpdateData);
        glDrawArrays(GL_TRIANGLES, 0, 3 * mNumTris);
    }
}
