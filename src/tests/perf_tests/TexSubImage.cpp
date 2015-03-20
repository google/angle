//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TexSubImageBenchmark:
//   Performace test for ANGLE texture updates.
//

#include <sstream>
#include <cassert>

#include "ANGLEPerfTest.h"
#include "shader_utils.h"

namespace
{

struct TexSubImageParams final : public PerfTestParams
{
    std::string suffix() const override;

    // Static parameters
    int imageWidth;
    int imageHeight;
    int subImageWidth;
    int subImageHeight;
    unsigned int iterations;
};

class TexSubImageBenchmark : public ANGLEPerfTest,
                             public ::testing::WithParamInterface<TexSubImageParams>
{
  public:
    TexSubImageBenchmark();

    bool initializeBenchmark() override;
    void destroyBenchmark() override;
    void beginDrawBenchmark() override;
    void drawBenchmark() override;

  private:
    DISALLOW_COPY_AND_ASSIGN(TexSubImageBenchmark);
    GLuint createTexture();

    // Handle to a program object
    GLuint mProgram;

    // Attribute locations
    GLint mPositionLoc;
    GLint mTexCoordLoc;

    // Sampler location
    GLint mSamplerLoc;

    // Texture handle
    GLuint mTexture;

    // Buffer handle
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;

    GLubyte *mPixels;
};

std::string TexSubImageParams::suffix() const
{
    // TODO(jmadill)
    return PerfTestParams::suffix();
}

TexSubImageBenchmark::TexSubImageBenchmark()
    : ANGLEPerfTest("TexSubImage", GetParam()),
      mProgram(0),
      mPositionLoc(-1),
      mTexCoordLoc(-1),
      mSamplerLoc(-1),
      mTexture(0),
      mVertexBuffer(0),
      mIndexBuffer(0),
      mPixels(NULL)
{
}

GLuint TexSubImageBenchmark::createTexture()
{
    const auto &params = GetParam();

    assert(params.iterations > 0);
    mDrawIterations = params.iterations;

    // Use tightly packed data
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Generate a texture object
    GLuint texture;
    glGenTextures(1, &texture);

    // Bind the texture object
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, params.imageWidth, params.imageHeight);

    // Set the filtering mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return texture;
}

bool TexSubImageBenchmark::initializeBenchmark()
{
    const auto &params = GetParam();

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

    mPixels = new GLubyte[params.subImageWidth * params.subImageHeight * 4];

    // Fill the pixels structure with random data:
    for (int y = 0; y < params.subImageHeight; ++y)
    {
        for (int x = 0; x < params.subImageWidth; ++x)
        {
            int offset = (x + (y * params.subImageWidth)) * 4;
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
    const auto &params = GetParam();

    // print static parameters
    printResult("image_width", static_cast<size_t>(params.imageWidth), "pix", false);
    printResult("image_height", static_cast<size_t>(params.imageHeight), "pix", false);
    printResult("subimage_width", static_cast<size_t>(params.subImageWidth), "pix", false);
    printResult("subimage_height", static_cast<size_t>(params.subImageHeight), "pix", false);
    printResult("iterations", static_cast<size_t>(params.iterations), "updates", false);

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
    const auto &params = GetParam();

    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    rand() % (params.imageWidth - params.subImageWidth),
                    rand() % (params.imageHeight - params.subImageHeight),
                    params.subImageWidth, params.subImageHeight,
                    GL_RGBA, GL_UNSIGNED_BYTE, mPixels);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
}

TexSubImageParams D3D11Params()
{
    TexSubImageParams params;

    params.glesMajorVersion = 2;
    params.widowWidth = 512;
    params.windowHeight = 512;
    params.requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
    params.deviceType = EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE;

    params.imageWidth = 1024;
    params.imageHeight = 1024;
    params.subImageWidth = 64;
    params.subImageHeight = 64;
    params.iterations = 10;

    return params;
}

TexSubImageParams D3D9Params()
{
    TexSubImageParams params;

    params.glesMajorVersion = 2;
    params.widowWidth = 512;
    params.windowHeight = 512;
    params.requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE;
    params.deviceType = EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE;

    params.imageWidth = 1024;
    params.imageHeight = 1024;
    params.subImageWidth = 64;
    params.subImageHeight = 64;
    params.iterations = 10;

    return params;
}

} // namespace

TEST_P(TexSubImageBenchmark, Run)
{
    run();
}

INSTANTIATE_TEST_CASE_P(TextureUpdates,
                        TexSubImageBenchmark,
                        ::testing::Values(D3D11Params(), D3D9Params()));
