//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BufferSubData.cpp:
//   Performance test for ANGLE buffer updates.
//

#include <sstream>

#include "ANGLEPerfTest.h"
#include "common/span.h"
#include "test_utils/draw_call_perf_utils.h"

using namespace angle;

namespace
{
constexpr unsigned int kIterationsPerStep = 4;

struct BufferSubDataParams final : public RenderTestParams
{
    BufferSubDataParams()
    {
        // Common default values
        majorVersion      = 2;
        minorVersion      = 0;
        windowWidth       = 512;
        windowHeight      = 512;
        updateSize        = 32000;
        bufferSize        = 40000;
        iterationsPerStep = kIterationsPerStep;
        updateRate        = 1;
    }

    std::string story() const override;

    GLboolean vertexNormalized;
    GLenum vertexType;
    GLint vertexComponentCount;
    unsigned int updateRate;

    // static parameters
    GLsizeiptr updateSize;
    GLsizeiptr bufferSize;
};

std::ostream &operator<<(std::ostream &os, const BufferSubDataParams &params)
{
    os << params.backendAndStory().substr(1);
    return os;
}

class BufferSubDataBenchmark : public ANGLERenderTest,
                               public ::testing::WithParamInterface<BufferSubDataParams>
{
  public:
    BufferSubDataBenchmark();

    void initializeBenchmark() override;
    void destroyBenchmark() override;
    void drawBenchmark() override;

  private:
    GLuint mProgram;
    GLuint mBuffer;
    std::vector<uint8_t> mUpdateData;
    int mNumTris;
};

Span<const GLfloat> GetFloatData(GLint componentCount)
{
    static constexpr GLfloat vertices2[] = {
        1, 2, 0, 0, 2, 0,
    };

    static constexpr GLfloat vertices3[] = {
        1, 2, 1, 0, 0, 1, 2, 0, 1,
    };

    static constexpr GLfloat vertices4[] = {
        1, 2, 1, 3, 0, 0, 1, 3, 2, 0, 1, 3,
    };

    switch (componentCount)
    {
        case 2:
            return vertices2;
        case 3:
            return vertices3;
        case 4:
            return vertices4;
        default:
            return {};
    }
}

template <class T>
GLsizeiptr GetNormalizedData(Span<const GLfloat> floatData, std::vector<uint8_t> *data)
{
    GLsizeiptr triDataSize = sizeof(T) * floatData.size();
    data->resize(triDataSize);

    Span<T> dest = reinterpret_span<T>(Span(*data));

    for (size_t dataIndex = 0; dataIndex < floatData.size(); dataIndex++)
    {
        GLfloat scaled = floatData[dataIndex] * 0.25f;
        dest[dataIndex] =
            static_cast<T>(scaled * static_cast<GLfloat>(std::numeric_limits<T>::max()));
    }

    return triDataSize;
}

template <class T>
GLsizeiptr GetIntData(Span<const GLfloat> floatData, std::vector<uint8_t> *data)
{
    GLsizeiptr triDataSize = sizeof(T) * floatData.size();
    data->resize(triDataSize);

    Span<T> dest = reinterpret_span<T>(Span(*data));

    for (size_t dataIndex = 0; dataIndex < floatData.size(); dataIndex++)
    {
        dest[dataIndex] = static_cast<T>(floatData[dataIndex]);
    }

    return triDataSize;
}

GLsizeiptr GetVertexData(GLenum type,
                         GLint componentCount,
                         GLboolean normalized,
                         std::vector<uint8_t> *data)
{
    Span<const GLfloat> floatData = GetFloatData(componentCount);

    if (type == GL_FLOAT)
    {
        const GLsizeiptr triDataSize = sizeof(GLfloat) * componentCount * 3;
        data->resize(triDataSize);
        Span(*data).copy_from(as_bytes(floatData));
        return triDataSize;
    }

    if (normalized == GL_TRUE)
    {
        switch (type)
        {
            case GL_BYTE:
                return GetNormalizedData<GLbyte>(floatData, data);
            case GL_SHORT:
                return GetNormalizedData<GLshort>(floatData, data);
            case GL_INT:
                return GetNormalizedData<GLint>(floatData, data);
            case GL_UNSIGNED_BYTE:
                return GetNormalizedData<GLubyte>(floatData, data);
            case GL_UNSIGNED_SHORT:
                return GetNormalizedData<GLushort>(floatData, data);
            case GL_UNSIGNED_INT:
                return GetNormalizedData<GLuint>(floatData, data);
            default:
                assert(0);
                return 0;
        }
    }

    switch (type)
    {
        case GL_BYTE:
            return GetIntData<GLbyte>(floatData, data);
        case GL_SHORT:
            return GetIntData<GLshort>(floatData, data);
        case GL_INT:
            return GetIntData<GLint>(floatData, data);
        case GL_UNSIGNED_BYTE:
            return GetIntData<GLubyte>(floatData, data);
        case GL_UNSIGNED_SHORT:
            return GetIntData<GLushort>(floatData, data);
        case GL_UNSIGNED_INT:
            return GetIntData<GLuint>(floatData, data);
        default:
            assert(0);
            return 0;
    }
}

std::string BufferSubDataParams::story() const
{
    std::stringstream strstr;

    strstr << RenderTestParams::story();

    if (vertexNormalized)
    {
        strstr << "_norm";
    }

    switch (vertexType)
    {
        case GL_FLOAT:
            strstr << "_float";
            break;
        case GL_INT:
            strstr << "_int";
            break;
        case GL_BYTE:
            strstr << "_byte";
            break;
        case GL_SHORT:
            strstr << "_short";
            break;
        case GL_UNSIGNED_INT:
            strstr << "_uint";
            break;
        case GL_UNSIGNED_BYTE:
            strstr << "_ubyte";
            break;
        case GL_UNSIGNED_SHORT:
            strstr << "_ushort";
            break;
        default:
            strstr << "_vunk_" << vertexType << "_";
            break;
    }

    strstr << vertexComponentCount;
    strstr << "_every" << updateRate;

    return strstr.str();
}

BufferSubDataBenchmark::BufferSubDataBenchmark()
    : ANGLERenderTest("BufferSubData", GetParam()), mProgram(0), mBuffer(0), mNumTris(0)
{}

void BufferSubDataBenchmark::initializeBenchmark()
{
    const auto &params = GetParam();

    ASSERT_LT(1, params.vertexComponentCount);

    mProgram = SetupSimpleScaleAndOffsetProgram();
    ASSERT_NE(0u, mProgram);

    if (params.vertexNormalized == GL_TRUE)
    {
        GLfloat scale  = 2.0f;
        GLfloat offset = -0.5f;
        glUniform1f(glGetUniformLocation(mProgram, "uScale"), scale);
        glUniform1f(glGetUniformLocation(mProgram, "uOffset"), offset);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    std::vector<uint8_t> zeroData(params.bufferSize, 0);

    glGenBuffers(1, &mBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
    glBufferData(GL_ARRAY_BUFFER, params.bufferSize, zeroData.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, params.vertexComponentCount, params.vertexType,
                          params.vertexNormalized, 0, 0);
    glEnableVertexAttribArray(0);

    mUpdateData.resize(params.updateSize);

    std::vector<uint8_t> data;
    GLsizei triDataSize = static_cast<GLsizei>(GetVertexData(
        params.vertexType, params.vertexComponentCount, params.vertexNormalized, &data));

    mNumTris = static_cast<int>(params.updateSize / triDataSize);
    Span updateData(mUpdateData);
    for (int i = 0, offset = 0; i < mNumTris; ++i)
    {
        updateData.subspan(offset, triDataSize).copy_from(data);
        offset += triDataSize;
    }

    if (params.updateSize == 0)
    {
        mNumTris = 1;
        glBufferSubData(GL_ARRAY_BUFFER, 0, data.size(), data.data());
    }

    // Set the viewport
    glViewport(0, 0, getWindow()->getWidth(), getWindow()->getHeight());

    ASSERT_GL_NO_ERROR();
}

void BufferSubDataBenchmark::destroyBenchmark()
{
    glDeleteProgram(mProgram);
    glDeleteBuffers(1, &mBuffer);
}

void BufferSubDataBenchmark::drawBenchmark()
{
    glClear(GL_COLOR_BUFFER_BIT);

    const auto &params = GetParam();

    for (unsigned int it = 0; it < params.iterationsPerStep; it++)
    {
        if (params.updateSize > 0 && ((getNumStepsPerformed() % params.updateRate) == 0))
        {
            glBufferSubData(GL_ARRAY_BUFFER, 0, mUpdateData.size(), mUpdateData.data());
        }

        glDrawArrays(GL_TRIANGLES, 0, 3 * mNumTris);
    }

    ASSERT_GL_NO_ERROR();
}

BufferSubDataParams BufferUpdateD3D11Params()
{
    BufferSubDataParams params;
    params.eglParameters        = egl_platform::D3D11();
    params.vertexType           = GL_FLOAT;
    params.vertexComponentCount = 4;
    params.vertexNormalized     = GL_FALSE;
    return params;
}

BufferSubDataParams BufferUpdateMetalParams()
{
    BufferSubDataParams params;
    params.eglParameters        = egl_platform::METAL();
    params.vertexType           = GL_FLOAT;
    params.vertexComponentCount = 4;
    params.vertexNormalized     = GL_FALSE;
    return params;
}

BufferSubDataParams BufferUpdateOpenGLOrGLESParams()
{
    BufferSubDataParams params;
    params.eglParameters        = egl_platform::OPENGL_OR_GLES();
    params.vertexType           = GL_FLOAT;
    params.vertexComponentCount = 4;
    params.vertexNormalized     = GL_FALSE;
    return params;
}

BufferSubDataParams BufferUpdateVulkanParams()
{
    BufferSubDataParams params;
    params.eglParameters        = egl_platform::VULKAN();
    params.vertexType           = GL_FLOAT;
    params.vertexComponentCount = 4;
    params.vertexNormalized     = GL_FALSE;
    return params;
}

TEST_P(BufferSubDataBenchmark, Run)
{
    run();
}

ANGLE_INSTANTIATE_TEST(BufferSubDataBenchmark,
                       BufferUpdateD3D11Params(),
                       BufferUpdateMetalParams(),
                       BufferUpdateOpenGLOrGLESParams(),
                       BufferUpdateVulkanParams());

}  // namespace
