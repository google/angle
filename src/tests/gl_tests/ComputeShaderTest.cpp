//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ComputeShaderTest:
//   Compute shader specific tests.

#include <vector>
#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

class ComputeShaderTest : public ANGLETest
{
  protected:
    ComputeShaderTest() {}
};

class ComputeShaderTestES3 : public ANGLETest
{
  protected:
    ComputeShaderTestES3() {}
};

// link a simple compute program. It should be successful.
TEST_P(ComputeShaderTest, LinkComputeProgram)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1) in;
        void main()
        {\
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);

    EXPECT_GL_NO_ERROR();
}

// Link a simple compute program. Then detach the shader and dispatch compute.
// It should be successful.
TEST_P(ComputeShaderTest, DetachShaderAfterLinkSuccess)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1) in;
        void main()
        {
        })";

    GLuint program = glCreateProgram();

    GLuint cs = CompileShader(GL_COMPUTE_SHADER, csSource);
    EXPECT_NE(0u, cs);

    glAttachShader(program, cs);
    glDeleteShader(cs);

    glLinkProgram(program);
    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    glDetachShader(program, cs);
    EXPECT_GL_NO_ERROR();

    glUseProgram(program);
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();
}

// link a simple compute program. There is no local size and linking should fail.
TEST_P(ComputeShaderTest, LinkComputeProgramNoLocalSizeLinkError)
{
    const std::string csSource =
        R"(#version 310 es
        void main()
        {
        })";

    GLuint program = CompileComputeProgram(csSource, false);
    EXPECT_EQ(0u, program);

    glDeleteProgram(program);

    EXPECT_GL_NO_ERROR();
}

// link a simple compute program.
// make sure that uniforms and uniform samplers get recorded
TEST_P(ComputeShaderTest, LinkComputeProgramWithUniforms)
{
    const std::string csSource =
        R"(#version 310 es
        precision mediump sampler2D;
        layout(local_size_x=1) in;
        uniform int myUniformInt;
        uniform sampler2D myUniformSampler;
        layout(rgba32i) uniform highp writeonly iimage2D imageOut;
        void main()
        {
            int q = myUniformInt;
            vec4 v = textureLod(myUniformSampler, vec2(0.0), 0.0);
            imageStore(imageOut, ivec2(0), ivec4(v) * q);
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);

    GLint uniformLoc = glGetUniformLocation(program.get(), "myUniformInt");
    EXPECT_NE(-1, uniformLoc);

    uniformLoc = glGetUniformLocation(program.get(), "myUniformSampler");
    EXPECT_NE(-1, uniformLoc);

    EXPECT_GL_NO_ERROR();
}

// Attach both compute and non-compute shaders. A link time error should occur.
// OpenGL ES 3.10, 7.3 Program Objects
TEST_P(ComputeShaderTest, AttachMultipleShaders)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1) in;
        void main()
        {
        })";

    const std::string vsSource =
        R"(#version 310 es
        void main()
        {
        })";

    const std::string fsSource =
        R"(#version 310 es
        void main()
        {
        })";

    GLuint program = glCreateProgram();

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);
    GLuint cs = CompileShader(GL_COMPUTE_SHADER, csSource);

    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, fs);
    EXPECT_NE(0u, cs);

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glAttachShader(program, cs);
    glDeleteShader(cs);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    EXPECT_GL_FALSE(linkStatus);

    EXPECT_GL_NO_ERROR();
}

// Attach a vertex, fragment and compute shader.
// Query for the number of attached shaders and check the count.
TEST_P(ComputeShaderTest, AttachmentCount)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1) in;
        void main()
        {
        })";

    const std::string vsSource =
        R"(#version 310 es
        void main()
        {
        })";

    const std::string fsSource =
        R"(#version 310 es
        void main()
        {
        })";

    GLuint program = glCreateProgram();

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);
    GLuint cs = CompileShader(GL_COMPUTE_SHADER, csSource);

    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, fs);
    EXPECT_NE(0u, cs);

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glAttachShader(program, cs);
    glDeleteShader(cs);

    GLint numAttachedShaders;
    glGetProgramiv(program, GL_ATTACHED_SHADERS, &numAttachedShaders);

    EXPECT_EQ(3, numAttachedShaders);

    glDeleteProgram(program);

    EXPECT_GL_NO_ERROR();
}

// Attach a compute shader and link, but start rendering.
TEST_P(ComputeShaderTest, StartRenderingWithComputeProgram)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1) in;
        void main()
        {
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    EXPECT_GL_NO_ERROR();

    glUseProgram(program);
    glDrawArrays(GL_POINTS, 0, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Attach a vertex and fragment shader and link, but dispatch compute.
TEST_P(ComputeShaderTest, DispatchComputeWithRenderingProgram)
{
    const std::string vsSource =
        R"(#version 310 es
        void main()
        {
        })";

    const std::string fsSource =
        R"(#version 310 es
        void main()
        {
        })";

    GLuint program = glCreateProgram();

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);

    EXPECT_NE(0u, vs);
    EXPECT_NE(0u, fs);

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_TRUE(linkStatus);

    EXPECT_GL_NO_ERROR();

    glUseProgram(program);
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Access all compute shader special variables.
TEST_P(ComputeShaderTest, AccessAllSpecialVariables)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=4, local_size_y=3, local_size_z=2) in;
        layout(rgba32ui) uniform highp writeonly uimage2D imageOut;
        void main()
        {
            uvec3 temp1 = gl_NumWorkGroups;
            uvec3 temp2 = gl_WorkGroupSize;
            uvec3 temp3 = gl_WorkGroupID;
            uvec3 temp4 = gl_LocalInvocationID;
            uvec3 temp5 = gl_GlobalInvocationID;
            uint  temp6 = gl_LocalInvocationIndex;
            imageStore(imageOut, ivec2(gl_LocalInvocationIndex, 0), uvec4(temp1 + temp2 + temp3 + temp4 + temp5, temp6));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
}

// Access part compute shader special variables.
TEST_P(ComputeShaderTest, AccessPartSpecialVariables)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=4, local_size_y=3, local_size_z=2) in;
        layout(rgba32ui) uniform highp writeonly uimage2D imageOut;
        void main()
        {
            uvec3 temp1 = gl_WorkGroupSize;
            uvec3 temp2 = gl_WorkGroupID;
            uint  temp3 = gl_LocalInvocationIndex;
            imageStore(imageOut, ivec2(gl_LocalInvocationIndex, 0), uvec4(temp1 + temp2, temp3));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
}

// Use glDispatchCompute to define work group count.
TEST_P(ComputeShaderTest, DispatchCompute)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=4, local_size_y=3, local_size_z=2) in;
        layout(rgba32ui) uniform highp writeonly uimage2D imageOut;
        void main()
        {
            uvec3 temp = gl_NumWorkGroups;
            imageStore(imageOut, ivec2(0), uvec4(temp, 0u));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);

    glUseProgram(program.get());
    glDispatchCompute(8, 4, 2);
    EXPECT_GL_NO_ERROR();
}

// Use image uniform to write texture in compute shader, and verify the content is expected.
TEST_P(ComputeShaderTest, BindImageTexture)
{
    GLTexture mTexture[2];
    GLFramebuffer mFramebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(r32ui, binding = 0) writeonly uniform highp uimage2D uImage[2];
        void main()
        {
            imageStore(uImage[0], ivec2(gl_LocalInvocationIndex, gl_WorkGroupID.x), uvec4(100, 0,
        0, 0));
            imageStore(uImage[1], ivec2(gl_LocalInvocationIndex, gl_WorkGroupID.x), uvec4(100, 0,
        0, 0));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());
    int width = 1, height = 1;
    GLuint inputValues[] = {200};

    glBindTexture(GL_TEXTURE_2D, mTexture[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_INT,
                    inputValues);
    EXPECT_GL_NO_ERROR();

    glBindImageTexture(0, mTexture[0], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_2D, mTexture[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_INT,
                    inputValues);
    EXPECT_GL_NO_ERROR();

    glBindImageTexture(1, mTexture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    glUseProgram(0);
    GLuint outputValues[2][1];
    GLuint expectedValue = 100;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFramebuffer);

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture[0],
                           0);
    EXPECT_GL_NO_ERROR();
    glReadPixels(0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues[0]);
    EXPECT_GL_NO_ERROR();

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture[1],
                           0);
    EXPECT_GL_NO_ERROR();
    glReadPixels(0, 0, width, height, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues[1]);
    EXPECT_GL_NO_ERROR();

    for (int i = 0; i < width * height; i++)
    {
        EXPECT_EQ(expectedValue, outputValues[0][i]);
        EXPECT_EQ(expectedValue, outputValues[1][i]);
    }
}

// When declare a image array without a binding qualifier, all elements are bound to unit zero.
TEST_P(ComputeShaderTest, ImageArrayWithoutBindingQualifier)
{
    ANGLE_SKIP_TEST_IF(IsD3D11());

    // TODO(xinghua.cao@intel.com): On AMD desktop OpenGL, bind two image variables to unit 0,
    // only one variable is valid.
    ANGLE_SKIP_TEST_IF(IsAMD() && IsDesktopOpenGL());

    GLTexture mTexture;
    GLFramebuffer mFramebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(r32ui) writeonly uniform highp uimage2D uImage[2];
        void main()
        {
            imageStore(uImage[0], ivec2(gl_LocalInvocationIndex, 0), uvec4(100, 0, 0, 0));
            imageStore(uImage[1], ivec2(gl_LocalInvocationIndex, 1), uvec4(100, 0, 0, 0));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());
    constexpr int kTextureWidth = 1, kTextureHeight = 2;
    GLuint inputValues[] = {200, 200};

    glBindTexture(GL_TEXTURE_2D, mTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, kTextureWidth, kTextureHeight);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kTextureWidth, kTextureHeight, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, inputValues);
    EXPECT_GL_NO_ERROR();

    glBindImageTexture(0, mTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mFramebuffer);

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0);
    GLuint outputValues[kTextureWidth * kTextureHeight];
    glReadPixels(0, 0, kTextureWidth, kTextureHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                 outputValues);
    EXPECT_GL_NO_ERROR();

    GLuint expectedValue = 100;
    for (int i = 0; i < kTextureWidth * kTextureHeight; i++)
    {
        EXPECT_EQ(expectedValue, outputValues[i]);
    }
}

// imageLoad functions
TEST_P(ComputeShaderTest, ImageLoad)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=8) in;
        layout(rgba8) uniform highp readonly image2D mImage2DInput;
        layout(rgba16i) uniform highp readonly iimageCube mImageCubeInput;
        layout(rgba32ui) uniform highp readonly uimage3D mImage3DInput;
        layout(r32i) uniform highp writeonly iimage2D imageOut;
        void main()
        {
            vec4 result2d = imageLoad(mImage2DInput, ivec2(gl_LocalInvocationID.xy));
            ivec4 resultCube = imageLoad(mImageCubeInput, ivec3(gl_LocalInvocationID.xyz));
            uvec4 result3d = imageLoad(mImage3DInput, ivec3(gl_LocalInvocationID.xyz));
            imageStore(imageOut, ivec2(gl_LocalInvocationIndex, 0), ivec4(result2d) + resultCube + ivec4(result3d));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    EXPECT_GL_NO_ERROR();
}

// imageStore functions
TEST_P(ComputeShaderTest, ImageStore)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=8) in;
        layout(rgba16f) uniform highp writeonly imageCube mImageCubeOutput;
        layout(r32f) uniform highp writeonly image3D mImage3DOutput;
        layout(rgba8ui) uniform highp writeonly uimage2DArray mImage2DArrayOutput;
        void main()
        {
            imageStore(mImageCubeOutput, ivec3(gl_LocalInvocationID.xyz), vec4(0.0));
            imageStore(mImage3DOutput, ivec3(gl_LocalInvocationID.xyz), vec4(0.0));
            imageStore(mImage2DArrayOutput, ivec3(gl_LocalInvocationID.xyz), uvec4(0));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    EXPECT_GL_NO_ERROR();
}

// imageSize functions
TEST_P(ComputeShaderTest, ImageSize)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=8) in;
        layout(rgba8) uniform highp readonly imageCube mImageCubeInput;
        layout(r32i) uniform highp readonly iimage2D mImage2DInput;
        layout(rgba16ui) uniform highp readonly uimage2DArray mImage2DArrayInput;
        layout(r32i) uniform highp writeonly iimage2D imageOut;
        void main()
        {
            ivec2 sizeCube = imageSize(mImageCubeInput);
            ivec2 size2D = imageSize(mImage2DInput);
            ivec3 size2DArray = imageSize(mImage2DArrayInput);
            imageStore(imageOut, ivec2(gl_LocalInvocationIndex, 0), ivec4(sizeCube, size2D.x, size2DArray.x));
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    EXPECT_GL_NO_ERROR();
}

// Test that sampling texture works well in compute shader.
TEST_P(ComputeShaderTest, TextureSampling)
{
    ANGLE_SKIP_TEST_IF(IsD3D11());

    const std::string &csSource =
        R"(#version 310 es
        layout(local_size_x=16, local_size_y=16) in;
        precision highp usampler2D;
        uniform usampler2D tex;
        layout(std140, binding = 0) buffer buf {
            uint outData[16][16];
        };

        void main()
        {
            uint x = gl_LocalInvocationID.x;
            uint y = gl_LocalInvocationID.y;
            outData[y][x] = texelFetch(tex, ivec2(x, y), 0).x;
        })";

    constexpr unsigned int kWidth  = 16;
    constexpr unsigned int kHeight = 16;
    GLTexture tex;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, kWidth, kHeight);
    GLuint texels[kHeight][kWidth] = {{0}};
    for (unsigned int y = 0; y < kHeight; ++y)
    {
        for (unsigned int x = 0; x < kWidth; ++x)
        {
            texels[y][x] = x + y * kWidth;
        }
    }
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                    texels);
    glBindTexture(GL_TEXTURE_2D, 0);

    // The array stride are rounded up to the base alignment of a vec4 for std140 layout.
    constexpr unsigned int kArrayStride = 16;
    GLBuffer ssbo;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, kWidth * kHeight * kArrayStride, nullptr,
                 GL_STREAM_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glUniform1i(glGetUniformLocation(program, "tex"), 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    glDispatchCompute(1, 1, 1);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    void *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, kWidth * kHeight * kArrayStride,
                                 GL_MAP_READ_BIT);
    for (unsigned int idx = 0; idx < kWidth * kHeight; idx++)
    {
        EXPECT_EQ(idx, *(reinterpret_cast<const GLuint *>(reinterpret_cast<const GLbyte *>(ptr) +
                                                          idx * kArrayStride)));
    }
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    EXPECT_GL_NO_ERROR();
}

// Use image uniform to read and write Texture2D in compute shader, and verify the contents.
TEST_P(ComputeShaderTest, BindImageTextureWithTexture2D)
{
    GLTexture texture[2];
    GLFramebuffer framebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(r32ui, binding = 0) readonly uniform highp uimage2D uImage_1;
        layout(r32ui, binding = 1) writeonly uniform highp uimage2D uImage_2;
        void main()
        {
            uvec4 value = imageLoad(uImage_1, ivec2(gl_LocalInvocationID.xy));
            imageStore(uImage_2, ivec2(gl_LocalInvocationID.xy), value);
        })";

    constexpr int kWidth = 1, kHeight = 1;
    constexpr GLuint kInputValues[2][1] = {{200}, {100}};

    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, kWidth, kHeight);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                    kInputValues[0]);
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, kWidth, kHeight);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                    kInputValues[1]);
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glBindImageTexture(1, texture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    GLuint outputValues[kWidth * kHeight];
    constexpr GLuint expectedValue = 200;
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture[1], 0);
    EXPECT_GL_NO_ERROR();
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();

    for (int i = 0; i < kWidth * kHeight; i++)
    {
        EXPECT_EQ(expectedValue, outputValues[i]);
    }
}

// Use image uniform to read and write Texture2DArray in compute shader, and verify the contents.
TEST_P(ComputeShaderTest, BindImageTextureWithTexture2DArray)
{
    GLTexture texture[2];
    GLFramebuffer framebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=2, local_size_y=2, local_size_z=2) in;
        layout(r32ui, binding = 0) readonly uniform highp uimage2DArray uImage_1;
        layout(r32ui, binding = 1) writeonly uniform highp uimage2DArray uImage_2;
        void main()
        {
            uvec4 value = imageLoad(uImage_1, ivec3(gl_LocalInvocationID.xyz));
            imageStore(uImage_2, ivec3(gl_LocalInvocationID.xyz), value);
        })";

    constexpr int kWidth = 1, kHeight = 1, kDepth = 2;
    constexpr GLuint kInputValues[2][2] = {{200, 200}, {100, 100}};

    glBindTexture(GL_TEXTURE_2D_ARRAY, texture[0]);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[0]);
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_2D_ARRAY, texture[1]);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[1]);
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture[0], 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glBindImageTexture(1, texture[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    GLuint outputValues[kWidth * kHeight];
    constexpr GLuint expectedValue = 200;
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture[1], 0, 0);
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, texture[1], 0, 1);
    EXPECT_GL_NO_ERROR();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kWidth * kHeight; i++)
    {
        EXPECT_EQ(expectedValue, outputValues[i]);
    }
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kWidth * kHeight; i++)
    {
        EXPECT_EQ(expectedValue, outputValues[i]);
    }
}

// Use image uniform to read and write Texture3D in compute shader, and verify the contents.
TEST_P(ComputeShaderTest, BindImageTextureWithTexture3D)
{
    GLTexture texture[2];
    GLFramebuffer framebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=2) in;
        layout(r32ui, binding = 0) readonly uniform highp uimage3D uImage_1;
        layout(r32ui, binding = 1) writeonly uniform highp uimage3D uImage_2;
        void main()
        {
            uvec4 value = imageLoad(uImage_1, ivec3(gl_LocalInvocationID.xyz));
            imageStore(uImage_2, ivec3(gl_LocalInvocationID.xyz), value);
        })";

    constexpr int kWidth = 1, kHeight = 1, kDepth = 2;
    constexpr GLuint kInputValues[2][2] = {{200, 200}, {100, 100}};

    glBindTexture(GL_TEXTURE_3D, texture[0]);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[0]);
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_3D, texture[1]);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[1]);
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture[0], 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glBindImageTexture(1, texture[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    GLuint outputValues[kWidth * kHeight];
    constexpr GLuint expectedValue = 200;
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture[1], 0, 0);
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, texture[1], 0, 1);
    EXPECT_GL_NO_ERROR();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kWidth * kHeight; i++)
    {
        EXPECT_EQ(expectedValue, outputValues[i]);
    }
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kWidth * kHeight; i++)
    {
        EXPECT_EQ(expectedValue, outputValues[i]);
    }
}

// Use image uniform to read and write TextureCube in compute shader, and verify the contents.
TEST_P(ComputeShaderTest, BindImageTextureWithTextureCube)
{
    GLTexture texture[2];
    GLFramebuffer framebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(r32ui, binding = 0) readonly uniform highp uimageCube uImage_1;
        layout(r32ui, binding = 1) writeonly uniform highp uimageCube uImage_2;
        void main()
        {
            for (int i = 0; i < 6; i++)
            {
                uvec4 value = imageLoad(uImage_1, ivec3(gl_LocalInvocationID.xy, i));
                imageStore(uImage_2, ivec3(gl_LocalInvocationID.xy, i), value);
            }
        })";

    constexpr int kWidth = 1, kHeight = 1;
    constexpr GLuint kInputValues[2][1] = {{200}, {100}};

    glBindTexture(GL_TEXTURE_CUBE_MAP, texture[0]);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_R32UI, kWidth, kHeight);
    for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
         face++)
    {
        glTexSubImage2D(face, 0, 0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                        kInputValues[0]);
    }
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_CUBE_MAP, texture[1]);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_R32UI, kWidth, kHeight);
    for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
         face++)
    {
        glTexSubImage2D(face, 0, 0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                        kInputValues[1]);
    }
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture[0], 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glBindImageTexture(1, texture[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    GLuint outputValues[kWidth * kHeight];
    constexpr GLuint expectedValue = 200;
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

    for (GLenum face = 0; face < 6; face++)
    {
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture[1], 0);
        EXPECT_GL_NO_ERROR();
        glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
        EXPECT_GL_NO_ERROR();

        for (int i = 0; i < kWidth * kHeight; i++)
        {
            EXPECT_EQ(expectedValue, outputValues[i]);
        }
    }
}

// Use image uniform to read and write one layer of Texture2DArray in compute shader, and verify the
// contents.
TEST_P(ComputeShaderTest, BindImageTextureWithOneLayerTexture2DArray)
{
    ANGLE_SKIP_TEST_IF(IsD3D11());

    GLTexture texture[2];
    GLFramebuffer framebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(r32ui, binding = 0) readonly uniform highp uimage2D uImage_1;
        layout(r32ui, binding = 1) writeonly uniform highp uimage2D uImage_2;
        void main()
        {
            uvec4 value = imageLoad(uImage_1, ivec2(gl_LocalInvocationID.xy));
            imageStore(uImage_2, ivec2(gl_LocalInvocationID.xy), value);
        })";

    constexpr int kWidth = 1, kHeight = 1, kDepth = 2;
    constexpr int kResultSize           = kWidth * kHeight;
    constexpr GLuint kInputValues[2][2] = {{200, 150}, {100, 50}};
    constexpr GLuint expectedValue_1    = 200;
    constexpr GLuint expectedValue_2    = 100;
    GLuint outputValues[kResultSize];

    glBindTexture(GL_TEXTURE_2D_ARRAY, texture[0]);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[0]);
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_2D_ARRAY, texture[1]);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[1]);
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture[0], 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();
    glBindImageTexture(1, texture[1], 0, GL_FALSE, 1, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();
    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture[1], 0, 0);
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, texture[1], 0, 1);
    EXPECT_GL_NO_ERROR();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kResultSize; i++)
    {
        EXPECT_EQ(expectedValue_2, outputValues[i]);
    }
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kResultSize; i++)
    {
        EXPECT_EQ(expectedValue_1, outputValues[i]);
    }
}

// Use image uniform to read and write one layer of Texture3D in compute shader, and verify the
// contents.
TEST_P(ComputeShaderTest, BindImageTextureWithOneLayerTexture3D)
{
    ANGLE_SKIP_TEST_IF(IsD3D11());
    GLTexture texture[2];
    GLFramebuffer framebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(r32ui, binding = 0) readonly uniform highp uimage2D uImage_1;
        layout(r32ui, binding = 1) writeonly uniform highp uimage2D uImage_2;
        void main()
        {
            uvec4 value = imageLoad(uImage_1, ivec2(gl_LocalInvocationID.xy));
            imageStore(uImage_2, ivec2(gl_LocalInvocationID.xy), value);
        })";

    constexpr int kWidth = 1, kHeight = 1, kDepth = 2;
    constexpr int kResultSize           = kWidth * kHeight;
    constexpr GLuint kInputValues[2][2] = {{200, 150}, {100, 50}};
    constexpr GLuint expectedValue_1    = 150;
    constexpr GLuint expectedValue_2    = 50;
    GLuint outputValues[kResultSize];

    glBindTexture(GL_TEXTURE_3D, texture[0]);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[0]);
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_3D, texture[1]);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, kWidth, kHeight, kDepth);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, kWidth, kHeight, kDepth, GL_RED_INTEGER,
                    GL_UNSIGNED_INT, kInputValues[1]);
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture[0], 0, GL_FALSE, 1, GL_READ_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();
    glBindImageTexture(1, texture[1], 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();
    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture[1], 0, 0);
    glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, texture[1], 0, 1);
    EXPECT_GL_NO_ERROR();
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kResultSize; i++)
    {
        EXPECT_EQ(expectedValue_1, outputValues[i]);
    }
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
    EXPECT_GL_NO_ERROR();
    for (int i = 0; i < kResultSize; i++)
    {
        EXPECT_EQ(expectedValue_2, outputValues[i]);
    }
}

// Use image uniform to read and write one layer of TextureCube in compute shader, and verify the
// contents.
TEST_P(ComputeShaderTest, BindImageTextureWithOneLayerTextureCube)
{
    ANGLE_SKIP_TEST_IF(IsD3D11());

    GLTexture texture[2];
    GLFramebuffer framebuffer;
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=1, local_size_y=1, local_size_z=1) in;
        layout(r32ui, binding = 0) readonly uniform highp uimage2D uImage_1;
        layout(r32ui, binding = 1) writeonly uniform highp uimage2D uImage_2;
        void main()
        {
            uvec4 value = imageLoad(uImage_1, ivec2(gl_LocalInvocationID.xy));
            imageStore(uImage_2, ivec2(gl_LocalInvocationID.xy), value);
        })";

    constexpr int kWidth = 1, kHeight = 1;
    constexpr int kResultSize           = kWidth * kHeight;
    constexpr GLuint kInputValues[2][1] = {{200}, {100}};
    constexpr GLuint expectedValue_1    = 200;
    constexpr GLuint expectedValue_2    = 100;
    GLuint outputValues[kResultSize];

    glBindTexture(GL_TEXTURE_CUBE_MAP, texture[0]);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_R32UI, kWidth, kHeight);
    for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
         face++)
    {
        glTexSubImage2D(face, 0, 0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                        kInputValues[0]);
    }
    EXPECT_GL_NO_ERROR();

    glBindTexture(GL_TEXTURE_CUBE_MAP, texture[1]);
    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_R32UI, kWidth, kHeight);
    for (GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X; face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z;
         face++)
    {
        glTexSubImage2D(face, 0, 0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT,
                        kInputValues[1]);
    }
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture[0], 0, GL_FALSE, 3, GL_READ_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();
    glBindImageTexture(1, texture[1], 0, GL_FALSE, 4, GL_WRITE_ONLY, GL_R32UI);
    EXPECT_GL_NO_ERROR();
    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

    for (GLenum face = 0; face < 6; face++)
    {
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, texture[1], 0);
        EXPECT_GL_NO_ERROR();
        glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, outputValues);
        EXPECT_GL_NO_ERROR();

        if (face == 4)
        {
            for (int i = 0; i < kResultSize; i++)
            {
                EXPECT_EQ(expectedValue_1, outputValues[i]);
            }
        }
        else
        {
            for (int i = 0; i < kResultSize; i++)
            {
                EXPECT_EQ(expectedValue_2, outputValues[i]);
            }
        }
    }
}

// Verify an INVALID_OPERATION error is reported when querying GL_COMPUTE_WORK_GROUP_SIZE for a
// program which has not been linked successfully or which does not contain objects to form a
// compute shader.
TEST_P(ComputeShaderTest, QueryComputeWorkGroupSize)
{
    const std::string vsSource =
        R"(#version 310 es
        void main()
        {
        })";

    const std::string fsSource =
        R"(#version 310 es
        void main()
        {
        })";

    GLint workGroupSize[3];

    ANGLE_GL_PROGRAM(graphicsProgram, vsSource, fsSource);
    glGetProgramiv(graphicsProgram, GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    GLuint computeProgram = glCreateProgram();
    GLShader computeShader(GL_COMPUTE_SHADER);
    glAttachShader(computeProgram, computeShader);
    glLinkProgram(computeProgram);
    glDetachShader(computeProgram, computeShader);

    GLint linkStatus;
    glGetProgramiv(computeProgram, GL_LINK_STATUS, &linkStatus);
    ASSERT_GL_FALSE(linkStatus);

    glGetProgramiv(computeProgram, GL_COMPUTE_WORK_GROUP_SIZE, workGroupSize);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glDeleteProgram(computeProgram);

    ASSERT_GL_NO_ERROR();
}

// Use groupMemoryBarrier and barrier to sync reads/writes order and the execution
// order of multiple shader invocations in compute shader.
TEST_P(ComputeShaderTest, groupMemoryBarrierAndBarrierTest)
{
    // TODO(xinghua.cao@intel.com): Figure out why we get this error message
    // that shader uses features not recognized by this D3D version.
    ANGLE_SKIP_TEST_IF((IsAMD() || IsNVIDIA()) && IsD3D11());

    GLTexture texture;
    GLFramebuffer framebuffer;

    // Each invocation first stores a single value in an image, then each invocation sums up
    // all the values in the image and stores the sum in the image. groupMemoryBarrier is
    // used to order reads/writes to variables stored in memory accessible to other shader
    // invocations, and barrier is used to control the relative execution order of multiple
    // shader invocations used to process a local work group.
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=2, local_size_y=2, local_size_z=1) in;
        layout(r32i, binding = 0) uniform highp iimage2D image;
        void main()
        {
            uint x = gl_LocalInvocationID.x;
            uint y = gl_LocalInvocationID.y;
            imageStore(image, ivec2(gl_LocalInvocationID.xy), ivec4(x + y));
            groupMemoryBarrier();
            barrier();
            int sum = 0;
            for (int i = 0; i < 2; i++)
            {
                for(int j = 0; j < 2; j++)
                {
                    sum += imageLoad(image, ivec2(i, j)).x;
                }
            }
            groupMemoryBarrier();
            barrier();
            imageStore(image, ivec2(gl_LocalInvocationID.xy), ivec4(sum));
        })";

    constexpr int kWidth = 2, kHeight = 2;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32I, kWidth, kHeight);
    EXPECT_GL_NO_ERROR();

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    glUseProgram(program.get());

    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);
    EXPECT_GL_NO_ERROR();

    glDispatchCompute(1, 1, 1);
    EXPECT_GL_NO_ERROR();

    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    GLuint outputValues[kWidth * kHeight];
    constexpr GLuint kExpectedValue = 4;
    glUseProgram(0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    EXPECT_GL_NO_ERROR();
    glReadPixels(0, 0, kWidth, kHeight, GL_RED_INTEGER, GL_INT, outputValues);
    EXPECT_GL_NO_ERROR();

    for (int i = 0; i < kWidth * kHeight; i++)
    {
        EXPECT_EQ(kExpectedValue, outputValues[i]);
    }
}

// Verify that a link error is generated when the sum of the number of active image uniforms and
// active shader storage blocks in a compute shader exceeds GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES.
TEST_P(ComputeShaderTest, ExceedCombinedShaderOutputResourcesInCS)
{
    // TODO(jiawei.shao@intel.com): enable this test when shader storage buffer is supported on
    // D3D11 back-ends.
    ANGLE_SKIP_TEST_IF(IsD3D11());

    GLint maxCombinedShaderOutputResources;
    GLint maxComputeShaderStorageBlocks;
    GLint maxComputeImageUniforms;

    glGetIntegerv(GL_MAX_COMBINED_SHADER_OUTPUT_RESOURCES, &maxCombinedShaderOutputResources);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &maxComputeShaderStorageBlocks);
    glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &maxComputeImageUniforms);

    ANGLE_SKIP_TEST_IF(maxCombinedShaderOutputResources >=
                       maxComputeShaderStorageBlocks + maxComputeImageUniforms);

    std::ostringstream computeShaderStream;
    computeShaderStream << "#version 310 es\n"
                           "layout(local_size_x = 3, local_size_y = 1, local_size_z = 1) in;\n"
                           "layout(shared, binding = 0) buffer blockName"
                           "{\n"
                           "    uint data;\n"
                           "} instance["
                        << maxComputeShaderStorageBlocks << "];\n";

    ASSERT_GE(maxComputeImageUniforms, 4);
    int numImagesInArray  = maxComputeImageUniforms / 2;
    int numImagesNonArray = maxComputeImageUniforms - numImagesInArray;
    for (int i = 0; i < numImagesNonArray; ++i)
    {
        computeShaderStream << "layout(r32f, binding = " << i << ") uniform highp image2D image"
                            << i << ";\n";
    }

    computeShaderStream << "layout(r32f, binding = " << numImagesNonArray
                        << ") uniform highp image2D imageArray[" << numImagesInArray << "];\n";

    computeShaderStream << "void main()\n"
                           "{\n"
                           "    uint val = 0u;\n"
                           "    vec4 val2 = vec4(0.0);\n";

    for (int i = 0; i < maxComputeShaderStorageBlocks; ++i)
    {
        computeShaderStream << "    val += instance[" << i << "].data; \n";
    }

    for (int i = 0; i < numImagesNonArray; ++i)
    {
        computeShaderStream << "    val2 += imageLoad(image" << i
                            << ", ivec2(gl_LocalInvocationID.xy)); \n";
    }

    for (int i = 0; i < numImagesInArray; ++i)
    {
        computeShaderStream << "    val2 += imageLoad(imageArray[" << i << "]"
                            << ", ivec2(gl_LocalInvocationID.xy)); \n";
    }

    computeShaderStream << "    instance[0].data = val + uint(val2.x);\n"
                           "}\n";

    GLuint computeProgram = CompileComputeProgram(computeShaderStream.str());
    EXPECT_EQ(0u, computeProgram);
}

// Test that uniform block with struct member in compute shader is supported.
TEST_P(ComputeShaderTest, UniformBlockWithStructMember)
{
    const std::string csSource =
        R"(#version 310 es
        layout(local_size_x=8) in;
        layout(rgba8) uniform highp readonly image2D mImage2DInput;
        layout(rgba8) uniform highp writeonly image2D mImage2DOutput;
        struct S {
          ivec3 a;
          ivec2 b;
        };

        layout(std140, binding=0) uniform blockName {
            S bd;
        } instanceName;
        void main()
        {
            ivec2 t1 = instanceName.bd.b;
            vec4 result2d = imageLoad(mImage2DInput, t1);
            imageStore(mImage2DOutput, ivec2(gl_LocalInvocationID.xy), result2d);
        })";

    ANGLE_GL_COMPUTE_PROGRAM(program, csSource);
    EXPECT_GL_NO_ERROR();
}

// Check that it is not possible to create a compute shader when the context does not support ES
// 3.10
TEST_P(ComputeShaderTestES3, NotSupported)
{
    GLuint computeShaderHandle = glCreateShader(GL_COMPUTE_SHADER);
    EXPECT_EQ(0u, computeShaderHandle);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
}

ANGLE_INSTANTIATE_TEST(ComputeShaderTest, ES31_OPENGL(), ES31_OPENGLES(), ES31_D3D11());
ANGLE_INSTANTIATE_TEST(ComputeShaderTestES3, ES3_OPENGL(), ES3_OPENGLES());

}  // namespace
