//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "TexSubImage.h"

#include <sstream>
#include <cassert>

#include "shader_utils.h"

std::string TexSubImageParams::suffix() const
{
    // TODO(jmadill)
    return BenchmarkParams::suffix();
}

TexSubImageBenchmark::TexSubImageBenchmark(const TexSubImageParams &params)
    : SimpleBenchmark("TexSubImage", 512, 512, 2, params),
      mParams(params),
      mProgram(0),
      mPositionLoc(-1),
      mTexCoordLoc(-1),
      mSamplerLoc(-1),
      mTexture(0),
      mVertexBuffer(0),
      mIndexBuffer(0),
      mPixels(NULL)
{
    assert(mParams.iterations > 0);
    mDrawIterations = mParams.iterations;
}

GLuint TexSubImageBenchmark::createTexture()
{
    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate a texture object
    GLuint texture;
    glGenTextures(1, &texture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, mParams.imageWidth, mParams.imageHeight);

    // Set the filtering mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture;
}

bool TexSubImageBenchmark::initializeBenchmark()
{
    const std::string vs = SHADER_SOURCE
    (
        attribute vec4 a_position;
        attribute vec2 a_texCoord;
        varying vec2 v_texCoord;
        void main()
        {
            gl_Position = a_position;
            v_texCoord = a_texCoord;
        }
    );

    const std::string fs = SHADER_SOURCE
    (
        precision mediump float;
        varying vec2 v_texCoord;
        uniform sampler2D s_texture;
        void main()
        {
            gl_FragColor = texture2D(s_texture, v_texCoord);
        }
    );

    mProgram = CompileProgram(vs, fs);
    if (!mProgram)
    {
        return false;
    }

    // Get the attribute locations
    mPositionLoc = glGetAttribLocation(mProgram, "a_position");
    mTexCoordLoc = glGetAttribLocation(mProgram, "a_texCoord");

    // Get the sampler location
    mSamplerLoc = glGetUniformLocation(mProgram, "s_texture");

    // Build the vertex buffer
    GLfloat vertices[] =
    {
        -0.5f, 0.5f, 0.0f,  // Position 0
        0.0f, 0.0f,        // TexCoord 0
        -0.5f, -0.5f, 0.0f,  // Position 1
        0.0f, 1.0f,        // TexCoord 1
        0.5f, -0.5f, 0.0f,  // Position 2
        1.0f, 1.0f,        // TexCoord 2
        0.5f, 0.5f, 0.0f,  // Position 3
        1.0f, 0.0f         // TexCoord 3
    };

    glGenBuffers(1, &mVertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    glGenBuffers(1, &mIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Load the texture
    mTexture = createTexture();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    mPixels = new GLubyte[mParams.subImageWidth * mParams.subImageHeight * 4];

    // Fill the pixels structure with random data:
    for (int y = 0; y < mParams.subImageHeight; ++y)
    {
        for (int x = 0; x < mParams.subImageWidth; ++x)
        {
            int offset = (x + (y * mParams.subImageWidth)) * 4;
            mPixels[offset + 0] = rand() % 255; // Red
            mPixels[offset + 1] = rand() % 255; // Green
            mPixels[offset + 2] = rand() % 255; // Blue
            mPixels[offset + 3] = 255; // Alpha
        }
    }

    return true;
}

void TexSubImageBenchmark::destroyBenchmark()
{
    // print static parameters
    printResult("image_width", static_cast<size_t>(mParams.imageWidth), "pix", false);
    printResult("image_height", static_cast<size_t>(mParams.imageHeight), "pix", false);
    printResult("subimage_width", static_cast<size_t>(mParams.subImageWidth), "pix", false);
    printResult("subimage_height", static_cast<size_t>(mParams.subImageHeight), "pix", false);
    printResult("iterations", static_cast<size_t>(mParams.iterations), "updates", false);

    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mVertexBuffer);
    glDeleteBuffers(1, &mIndexBuffer);
    glDeleteTextures(1, &mTexture);
    delete[] mPixels;
}

void TexSubImageBenchmark::beginDrawBenchmark()
{
    // Set the viewport
    glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    // Use the program object
    glUseProgram(mProgram);

    // Bind the buffers
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);

    // Load the vertex position
    glVertexAttribPointer(mPositionLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);
    // Load the texture coordinate
    glVertexAttribPointer(mTexCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));

    glEnableVertexAttribArray(mPositionLoc);
    glEnableVertexAttribArray(mTexCoordLoc);

    // Bind the texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTexture);

    // Set the texture sampler to texture unit to 0
    glUniform1i(mSamplerLoc, 0);
}

void TexSubImageBenchmark::drawBenchmark()
{
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    rand() % (mParams.imageWidth - mParams.subImageWidth),
                    rand() % (mParams.imageHeight - mParams.subImageHeight),
                    mParams.subImageWidth, mParams.subImageHeight,
                    GL_RGBA, GL_UNSIGNED_BYTE, mPixels);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}
