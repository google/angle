/*-------------------------------------------------------------------------
 * OpenGL Conformance Test Suite
 * -----------------------------
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */ /*!
 * \file
 * \brief
 */ /*-------------------------------------------------------------------*/

#include <cstdarg>
#include "es31cVertexAttribBindingTests.hpp"
#include "glwEnums.hpp"
#include "tcuMatrix.hpp"
#include "tcuRenderTarget.hpp"

#include <cmath>

namespace glcts
{
using namespace glw;
using tcu::IVec2;
using tcu::IVec3;
using tcu::IVec4;
using tcu::Mat4;
using tcu::UVec2;
using tcu::UVec3;
using tcu::UVec4;
using tcu::Vec2;
using tcu::Vec3;
using tcu::Vec4;

namespace
{

class VertexAttribBindingBase : public glcts::SubcaseBase
{
    virtual std::string Title() { return NL ""; }

    virtual std::string Purpose() { return NL ""; }

    virtual std::string Method() { return NL ""; }

    virtual std::string PassCriteria() { return NL ""; }

  public:
    bool IsSSBOInVSFSAvailable(int required)
    {
        GLint blocksVS, blocksFS;
        glGetIntegerv(GL_MAX_VERTEX_SHADER_STORAGE_BLOCKS, &blocksVS);
        glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &blocksFS);
        if (blocksVS >= required && blocksFS >= required)
            return true;
        else
        {
            std::ostringstream reason;
            reason << "Required " << required << " VS storage blocks but only " << blocksVS
                   << " available." << std::endl
                   << "Required " << required << " FS storage blocks but only " << blocksFS
                   << " available." << std::endl;
            OutputNotSupported(reason.str());
            return false;
        }
    }

    int getWindowWidth()
    {
        const tcu::RenderTarget &renderTarget = m_context.getRenderTarget();
        return renderTarget.getWidth();
    }

    int getWindowHeight()
    {
        const tcu::RenderTarget &renderTarget = m_context.getRenderTarget();
        return renderTarget.getHeight();
    }

    inline bool ColorEqual(const Vec4 &c0, const Vec4 &c1, const Vec4 &epsilon)
    {
        if (fabs(c0[0] - c1[0]) > epsilon[0])
            return false;
        if (fabs(c0[1] - c1[1]) > epsilon[1])
            return false;
        if (fabs(c0[2] - c1[2]) > epsilon[2])
            return false;
        if (fabs(c0[3] - c1[3]) > epsilon[3])
            return false;
        return true;
    }

    bool CheckProgram(GLuint program)
    {
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);

        if (status == GL_FALSE)
        {
            GLint attached_shaders;
            glGetProgramiv(program, GL_ATTACHED_SHADERS, &attached_shaders);

            if (attached_shaders > 0)
            {
                std::vector<GLuint> shaders(attached_shaders);
                glGetAttachedShaders(program, attached_shaders, NULL, &shaders[0]);

                for (GLint i = 0; i < attached_shaders; ++i)
                {
                    GLenum type;
                    glGetShaderiv(shaders[i], GL_SHADER_TYPE, reinterpret_cast<GLint *>(&type));
                    switch (type)
                    {
                        case GL_VERTEX_SHADER:
                            m_context.getTestContext().getLog()
                                << tcu::TestLog::Message << "*** Vertex Shader ***"
                                << tcu::TestLog::EndMessage;
                            break;
                        case GL_FRAGMENT_SHADER:
                            m_context.getTestContext().getLog()
                                << tcu::TestLog::Message << "*** Fragment Shader ***"
                                << tcu::TestLog::EndMessage;
                            break;
                        default:
                            m_context.getTestContext().getLog()
                                << tcu::TestLog::Message << "*** Unknown Shader ***"
                                << tcu::TestLog::EndMessage;
                            break;
                    }
                    GLint length;
                    glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &length);
                    if (length > 0)
                    {
                        std::vector<GLchar> source(length);
                        glGetShaderSource(shaders[i], length, NULL, &source[0]);
                        m_context.getTestContext().getLog()
                            << tcu::TestLog::Message << &source[0] << tcu::TestLog::EndMessage;
                    }
                    glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &length);
                    if (length > 0)
                    {
                        std::vector<GLchar> log(length);
                        glGetShaderInfoLog(shaders[i], length, NULL, &log[0]);
                        m_context.getTestContext().getLog()
                            << tcu::TestLog::Message << &log[0] << tcu::TestLog::EndMessage;
                    }
                }
            }
            GLint length;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            if (length > 0)
            {
                std::vector<GLchar> log(length);
                glGetProgramInfoLog(program, length, NULL, &log[0]);
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << &log[0] << tcu::TestLog::EndMessage;
            }
        }
        return status == GL_TRUE ? true : false;
    }

    bool IsEqual(IVec4 a, IVec4 b)
    {
        return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
    }

    bool IsEqual(UVec4 a, UVec4 b)
    {
        return (a[0] == b[0]) && (a[1] == b[1]) && (a[2] == b[2]) && (a[3] == b[3]);
    }

    bool IsEqual(Vec2 a, Vec2 b) { return (a[0] == b[0]) && (a[1] == b[1]); }

    bool IsEqual(IVec2 a, IVec2 b) { return (a[0] == b[0]) && (a[1] == b[1]); }

    bool IsEqual(UVec2 a, UVec2 b) { return (a[0] == b[0]) && (a[1] == b[1]); }

    bool CheckFB(Vec3 expected)
    {
        const tcu::RenderTarget &renderTarget = m_context.getRenderContext().getRenderTarget();
        const tcu::PixelFormat &pixelFormat   = renderTarget.getPixelFormat();
        Vec3 g_color_eps = Vec3(1.f / static_cast<float>(1 << pixelFormat.redBits),
                                1.f / static_cast<float>(1 << pixelFormat.greenBits),
                                1.f / static_cast<float>(1 << pixelFormat.blueBits));
        Vec3 g_color_max = Vec3(255);
        std::vector<GLubyte> fb(getWindowWidth() * getWindowHeight() * 4);
        int fb_w = getWindowWidth();
        int fb_h = getWindowHeight();
        glReadPixels(0, 0, fb_w, fb_h, GL_RGBA, GL_UNSIGNED_BYTE, &fb[0]);
        for (GLint i = 0, y = 0; y < fb_h; ++y)
            for (GLint x = 0; x < fb_w; ++x, i += 4)
            {
                if (fabs(fb[i + 0] / g_color_max[0] - expected[0]) > g_color_eps[0] ||
                    fabs(fb[i + 1] / g_color_max[1] - expected[1]) > g_color_eps[1] ||
                    fabs(fb[i + 2] / g_color_max[2] - expected[2]) > g_color_eps[2])
                {
                    m_context.getTestContext().getLog()
                        << tcu::TestLog::Message << "Incorrect framebuffer color at pixel (" << x
                        << " " << y << "). Color is (" << fb[i + 0] / g_color_max[0] << " "
                        << fb[i + 1] / g_color_max[1] << " " << fb[i + 2] / g_color_max[2]
                        << ". Color should be (" << expected[0] << " " << expected[1] << " "
                        << expected[2] << ")." << tcu::TestLog::EndMessage;
                    return false;
                }
            }
        return true;
    }

    GLhalf FloatToHalf(float f)
    {
        const unsigned int HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP = 0x38000000;
        /* max exponent value in single precision that will be converted */
        /* to Inf or Nan when stored as a half-float */
        const unsigned int HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP = 0x47800000;
        /* 255 is the max exponent biased value */
        const unsigned int FLOAT_MAX_BIASED_EXP      = (0xFF << 23);
        const unsigned int HALF_FLOAT_MAX_BIASED_EXP = (0x1F << 10);
        char *c                                      = reinterpret_cast<char *>(&f);
        unsigned int x                               = *reinterpret_cast<unsigned int *>(c);
        unsigned int sign                            = (GLhalf)(x >> 31);
        unsigned int mantissa;
        unsigned int exp;
        GLhalf hf;

        /* get mantissa */
        mantissa = x & ((1 << 23) - 1);
        /* get exponent bits */
        exp = x & FLOAT_MAX_BIASED_EXP;
        if (exp >= HALF_FLOAT_MAX_BIASED_EXP_AS_SINGLE_FP_EXP)
        {
            /* check if the original single precision float number is a NaN */
            if (mantissa && (exp == FLOAT_MAX_BIASED_EXP))
            {
                /* we have a single precision NaN */
                mantissa = (1 << 23) - 1;
            }
            else
            {
                /* 16-bit half-float representation stores number as Inf */
                mantissa = 0;
            }
            hf = (GLhalf)((((GLhalf)sign) << 15) | (GLhalf)(HALF_FLOAT_MAX_BIASED_EXP) |
                          (GLhalf)(mantissa >> 13));
        }
        /* check if exponent is <= -15 */
        else if (exp <= HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP)
        {
            /* store a denorm half-float value or zero */
            exp = ((HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP - exp) >> 23) + 14;
            // handle 0.0 specially to avoid a right-shift by too many bits
            if (exp >= 32)
            {
                return 0;
            }
            mantissa |= (1 << 23);
            mantissa >>= exp;
            hf = (GLhalf)((((GLhalf)sign) << 15) | (GLhalf)(mantissa));
        }
        else
        {
            hf = (GLhalf)((((GLhalf)sign) << 15) |
                          (GLhalf)((exp - HALF_FLOAT_MIN_BIASED_EXP_AS_SINGLE_FP_EXP) >> 13) |
                          (GLhalf)(mantissa >> 13));
        }

        return hf;
    }
};
//=============================================================================
// 1.1 BasicUsage
//-----------------------------------------------------------------------------
class BasicUsage : public VertexAttribBindingBase
{
    bool pipeline;

    GLuint m_vsp, m_fsp, m_ppo, m_vao, m_vbo;

    virtual long Setup()
    {
        if (pipeline)
        {
            m_vsp = m_fsp = 0;
            glGenProgramPipelines(1, &m_ppo);
        }
        else
        {
            m_ppo = 0;
        }
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        if (pipeline)
        {
            glDeleteProgram(m_vsp);
            glDeleteProgram(m_fsp);
            glDeleteProgramPipelines(1, &m_ppo);
        }
        else
        {
            glUseProgram(0);
            glDeleteProgram(m_ppo);
        }
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        const char *const glsl_vs =
            "#version 310 es" NL "layout(location = 7) in vec4 vs_in_position;" NL
            "layout(location = 1) in vec3 vs_in_color;" NL "out vec3 g_color;" NL "void main() {" NL
            "  gl_Position = vs_in_position;" NL "  g_color = vs_in_color;" NL "}";
        const char *const glsl_fs = "#version 310 es" NL "precision highp float;" NL
                                    "in vec3 g_color;" NL "out vec4 fs_out_color;" NL
                                    "void main() {" NL "  fs_out_color = vec4(g_color, 1);" NL "}";
        if (pipeline)
        {
            m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
            m_fsp = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
            if (!CheckProgram(m_vsp) || !CheckProgram(m_fsp))
                return ERROR;
            glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);
            glUseProgramStages(m_ppo, GL_FRAGMENT_SHADER_BIT, m_fsp);
        }
        else
        {
            m_ppo            = glCreateProgram();
            const GLuint sh  = glCreateShader(GL_VERTEX_SHADER);
            const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sh, 1, &glsl_vs, NULL);
            glShaderSource(fsh, 1, &glsl_fs, NULL);
            glCompileShader(sh);
            glCompileShader(fsh);
            glAttachShader(m_ppo, sh);
            glAttachShader(m_ppo, fsh);
            glDeleteShader(sh);
            glDeleteShader(fsh);
            glLinkProgram(m_ppo);
            if (!CheckProgram(m_ppo))
                return ERROR;
        }
        /* vbo */
        {
            const float data[] = {
                -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
                -1.0f, 1.0f,  0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
                -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f, 1.0f, 0.0f,
                -1.0f, 1.0f,  1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f, 0.0f,
            };
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        glBindVertexArray(m_vao);
        glVertexAttribFormat(7, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 8);
        glVertexAttribBinding(7, 0);
        glVertexAttribBinding(1, 0);
        glBindVertexBuffer(0, m_vbo, 0, 20);
        glEnableVertexAttribArray(7);
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(m_vao);
        if (pipeline)
            glBindProgramPipeline(m_ppo);
        else
            glUseProgram(m_ppo);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        if (!CheckFB(Vec3(0, 1, 0)))
            return ERROR;

        glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
        if (!CheckFB(Vec3(1, 1, 0)))
            return ERROR;

        return NO_ERROR;
    }

  public:
    BasicUsage() : pipeline(true) {}
};
//=============================================================================
// BasicInputBase
//-----------------------------------------------------------------------------
class BasicInputBase : public VertexAttribBindingBase
{

    GLuint m_po, m_xfbo;

  protected:
    Vec4 expected_data[60];
    GLsizei instance_count;
    GLint base_instance;

    virtual long Setup()
    {
        m_po = 0;
        glGenBuffers(1, &m_xfbo);
        for (int i = 0; i < 60; ++i)
            expected_data[i] = Vec4(0.0f);
        instance_count = 1;
        base_instance  = -1;
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDisable(GL_RASTERIZER_DISCARD);
        glUseProgram(0);
        glDeleteProgram(m_po);
        glDeleteBuffers(1, &m_xfbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        const char *const glsl_vs =
            "#version 310 es" NL "layout(location = 0) in vec4 vs_in_attrib0;" NL
            "layout(location = 1) in vec4 vs_in_attrib1;" NL
            "layout(location = 2) in vec4 vs_in_attrib2;" NL
            "layout(location = 3) in vec4 vs_in_attrib3;" NL
            "layout(location = 4) in vec4 vs_in_attrib4;" NL
            "layout(location = 5) in vec4 vs_in_attrib5;" NL
            "layout(location = 6) in vec4 vs_in_attrib6;" NL
            "layout(location = 7) in vec4 vs_in_attrib7;" NL
            "layout(location = 8) in vec4 vs_in_attrib8;" NL
            "layout(location = 9) in vec4 vs_in_attrib9;" NL
            "layout(location = 10) in vec4 vs_in_attrib10;" NL
            "layout(location = 11) in vec4 vs_in_attrib11;" NL
            "layout(location = 12) in vec4 vs_in_attrib12;" NL
            "layout(location = 13) in vec4 vs_in_attrib13;" NL
            "layout(location = 14) in vec4 vs_in_attrib14;" NL "out vec4 attrib[15];" NL
            "void main() {" NL "  attrib[0] = vs_in_attrib0;" NL "  attrib[1] = vs_in_attrib1;" NL
            "  attrib[2] = vs_in_attrib2;" NL "  attrib[3] = vs_in_attrib3;" NL
            "  attrib[4] = vs_in_attrib4;" NL "  attrib[5] = vs_in_attrib5;" NL
            "  attrib[6] = vs_in_attrib6;" NL "  attrib[7] = vs_in_attrib7;" NL
            "  attrib[8] = vs_in_attrib8;" NL "  attrib[9] = vs_in_attrib9;" NL
            "  attrib[10] = vs_in_attrib10;" NL "  attrib[11] = vs_in_attrib11;" NL
            "  attrib[12] = vs_in_attrib12;" NL "  attrib[13] = vs_in_attrib13;" NL
            "  attrib[14] = vs_in_attrib14;" NL "}";
        const char *const glsl_fs =
            "#version 310 es" NL "precision mediump float;" NL "in vec4 attrib[15];" NL
            "out vec4 fs_out_color;" NL "void main() {" NL "  fs_out_color = attrib[8];" NL "}";
        m_po = glCreateProgram();
        /* attach shader */
        {
            const GLuint sh  = glCreateShader(GL_VERTEX_SHADER);
            const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sh, 1, &glsl_vs, NULL);
            glShaderSource(fsh, 1, &glsl_fs, NULL);
            glCompileShader(sh);
            glCompileShader(fsh);
            glAttachShader(m_po, sh);
            glAttachShader(m_po, fsh);
            glDeleteShader(sh);
            glDeleteShader(fsh);
        }
        /* setup XFB */
        {
            const GLchar *const v = "attrib";
            glTransformFeedbackVaryings(m_po, 1, &v, GL_INTERLEAVED_ATTRIBS);
        }
        glLinkProgram(m_po);
        if (!CheckProgram(m_po))
            return ERROR;

        /* buffer data */
        {
            std::vector<GLubyte> zero(sizeof(expected_data));
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_xfbo);
            glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, sizeof(expected_data), &zero[0],
                         GL_DYNAMIC_DRAW);
        }

        // capture
        glEnable(GL_RASTERIZER_DISCARD);
        glUseProgram(m_po);
        glBeginTransformFeedback(GL_POINTS);
        if (base_instance != -1)
        {
            glDrawArraysInstancedBaseInstance(GL_POINTS, 0, 2, instance_count,
                                              static_cast<GLuint>(base_instance));
        }
        else
        {
            glDrawArraysInstanced(GL_POINTS, 0, 2, instance_count);
        }
        glEndTransformFeedback();

        Vec4 *data = static_cast<Vec4 *>(
            glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(Vec4) * 60, GL_MAP_READ_BIT));

        long status = NO_ERROR;
        for (int i = 0; i < 60; ++i)
        {
            if (!ColorEqual(expected_data[i], data[i], Vec4(0.01f)))
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Data is: " << data[i][0] << " " << data[i][1]
                    << " " << data[i][2] << " " << data[i][3]
                    << ", data should be: " << expected_data[i][0] << " " << expected_data[i][1]
                    << " " << expected_data[i][2] << " " << expected_data[i][3]
                    << ", index is: " << i << tcu::TestLog::EndMessage;
                status = ERROR;
                break;
            }
        }
        return status;
    }
};

//=============================================================================
// 1.2.1 BasicInputCase1
//-----------------------------------------------------------------------------
class BasicInputCase1 : public BasicInputBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * 2, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec3), &Vec3(1.0f, 2.0f, 3.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(Vec3), &Vec3(4.0f, 5.0f, 6.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glBindVertexBuffer(0, m_vbo, 0, 12);
        glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(1, 0);
        glEnableVertexAttribArray(1);
        expected_data[1]  = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
        expected_data[16] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.2 BasicInputCase2
//-----------------------------------------------------------------------------
class BasicInputCase2 : public BasicInputBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * 2, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec3), &Vec3(1.0f, 2.0f, 3.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(Vec3), &Vec3(4.0f, 5.0f, 6.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribBinding(1, 0);
        glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(7, 1, GL_FLOAT, GL_FALSE, 8);
        glVertexAttribFormat(14, 2, GL_FLOAT, GL_FALSE, 4);
        glVertexAttribBinding(0, 0);
        glVertexAttribBinding(7, 0);
        glVertexAttribBinding(14, 0);
        glBindVertexBuffer(0, m_vbo, 0, 12);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(7);
        glEnableVertexAttribArray(14);

        expected_data[0]  = Vec4(1.0f, 2.0f, 0.0f, 1.0f);
        expected_data[1]  = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
        expected_data[7]  = Vec4(3.0f, 0.0f, 0.0f, 1.0f);
        expected_data[14] = Vec4(2.0f, 3.0f, 0.0f, 1.0f);
        expected_data[15] = Vec4(4.0f, 5.0f, 0.0f, 1.0f);
        expected_data[16] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
        expected_data[22] = Vec4(6.0f, 0.0f, 0.0f, 1.0f);
        expected_data[29] = Vec4(5.0f, 6.0f, 0.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.3 BasicInputCase3
//-----------------------------------------------------------------------------
class BasicInputCase3 : public BasicInputBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 36 * 2, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLubyte d[] = {1, 2, 3, 4};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }
        glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec3), &Vec3(5.0f, 6.0f, 7.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(Vec2), &Vec2(8.0f, 9.0f)[0]);
        /* */
        {
            GLubyte d[] = {10, 11, 12, 13};
            glBufferSubData(GL_ARRAY_BUFFER, 0 + 36, sizeof(d), d);
        }
        glBufferSubData(GL_ARRAY_BUFFER, 16 + 36, sizeof(Vec3), &Vec3(14.0f, 15.0f, 16.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 28 + 36, sizeof(Vec2), &Vec2(17.0f, 18.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glEnableVertexAttribArray(1);
        glVertexAttribFormat(0, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0);
        glVertexAttribBinding(1, 3);
        glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 16);
        glVertexAttribBinding(2, 3);
        glVertexAttribFormat(2, 2, GL_FLOAT, GL_FALSE, 28);
        glVertexAttribBinding(0, 3);
        glBindVertexBuffer(3, m_vbo, 0, 36);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);

        expected_data[0]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[1]      = Vec4(5.0f, 6.0f, 7.0f, 1.0f);
        expected_data[2]      = Vec4(8.0f, 9.0f, 0.0f, 1.0f);
        expected_data[0 + 15] = Vec4(10.0f, 11.0f, 12.0f, 13.0f);
        expected_data[1 + 15] = Vec4(14.0f, 15.0f, 16.0f, 1.0f);
        expected_data[2 + 15] = Vec4(17.0f, 18.0f, 0.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.4 BasicInputCase4
//-----------------------------------------------------------------------------
class BasicInputCase4 : public BasicInputBase
{

    GLuint m_vao, m_vbo[2];

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, 20 * 2, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLbyte d[] = {-127, 127, -127, 127};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {1, 2, 3, 4};
            glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {5, 6};
            glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
        }
        /* */
        {
            GLbyte d[] = {127, -127, 127, -127};
            glBufferSubData(GL_ARRAY_BUFFER, 0 + 20, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {7, 8, 9, 10};
            glBufferSubData(GL_ARRAY_BUFFER, 4 + 20, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {11, 12};
            glBufferSubData(GL_ARRAY_BUFFER, 12 + 20, sizeof(d), d);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, 24 * 2 + 8, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLhalf d[] = {FloatToHalf(0.0), FloatToHalf(100.0), FloatToHalf(200.0)};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }
        /* */
        {
            GLhalf d[] = {FloatToHalf(300.0), FloatToHalf(400.0)};
            glBufferSubData(GL_ARRAY_BUFFER, 26, sizeof(d), d);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribFormat(0, 4, GL_BYTE, GL_TRUE, 0);
        glVertexAttribFormat(1, 4, GL_UNSIGNED_SHORT, GL_FALSE, 4);
        glVertexAttribFormat(2, 2, GL_UNSIGNED_INT, GL_FALSE, 12);
        glVertexAttribFormat(5, 2, GL_HALF_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(0, 0);
        glVertexAttribBinding(1, 0);
        glVertexAttribBinding(2, 0);
        glVertexAttribBinding(5, 6);
        glBindVertexBuffer(0, m_vbo[0], 0, 20);
        glBindVertexBuffer(6, m_vbo[1], 2, 24);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(5);

        expected_data[0]      = Vec4(-1.0f, 1.0f, -1.0f, 1.0f);
        expected_data[1]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[2]      = Vec4(5.0f, 6.0f, 0.0f, 1.0f);
        expected_data[5]      = Vec4(100.0f, 200.0f, 0.0f, 1.0f);
        expected_data[0 + 15] = Vec4(1.0f, -1.0f, 1.0f, -1.0f);
        expected_data[1 + 15] = Vec4(7.0f, 8.0f, 9.0f, 10.0f);
        expected_data[2 + 15] = Vec4(11.0f, 12.0f, 0.0f, 1.0f);
        expected_data[5 + 15] = Vec4(300.0f, 400.0f, 0.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.5 BasicInputCase5
//-----------------------------------------------------------------------------
class BasicInputCase5 : public BasicInputBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        const int kStride = 116;
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, kStride * 2, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLubyte d[] = {0, 0xff, 0xff / 2, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {0, 0xffff, 0xffff / 2, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {0, 0xffffffff, 0xffffffff / 2, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
        }
        /* */
        {
            GLbyte d[] = {0, -127, 127, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(d), d);
        }
        /* */
        {
            GLshort d[] = {0, -32767, 32767, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {0, -2147483647, 2147483647, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 40, sizeof(d), d);
        }
        /* */
        {
            GLfloat d[] = {0, 1.0f, 2.0f, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 56, sizeof(d), d);
        }
        /* */
        {
            GLhalf d[] = {FloatToHalf(0.0), FloatToHalf(10.0), FloatToHalf(20.0), FloatToHalf(0.0)};
            glBufferSubData(GL_ARRAY_BUFFER, 72, sizeof(d), d);
        }
        /* */
        {
            GLubyte d[] = {0, 0xff / 4, 0xff / 2, 0xff};
            glBufferSubData(GL_ARRAY_BUFFER, 104, sizeof(d), d);
        }
        /* */
        {
            GLuint d = 0 | (1023 << 10) | (511 << 20) | (1 << 30);
            glBufferSubData(GL_ARRAY_BUFFER, 108, sizeof(d), &d);
        }
        /* */
        {
            GLint d = 0 | (511 << 10) | (255 << 20) | (0 << 30);
            glBufferSubData(GL_ARRAY_BUFFER, 112, sizeof(d), &d);
        }

        /* */
        {
            GLubyte d[] = {0xff, 0xff, 0xff / 2, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 0 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {0xffff, 0xffff, 0xffff / 2, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 4 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {0xffffffff, 0xffffffff, 0xffffffff / 2, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 12 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLbyte d[] = {127, -127, 127, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 28 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLshort d[] = {32767, -32767, 32767, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 32 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {2147483647, -2147483647, 2147483647, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 40 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLfloat d[] = {0, 3.0f, 4.0f, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 56 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLhalf d[] = {FloatToHalf(0.0), FloatToHalf(30.0), FloatToHalf(40.0), FloatToHalf(0.0)};
            glBufferSubData(GL_ARRAY_BUFFER, 72 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLubyte d[] = {0xff, 0xff / 2, 0xff / 4, 0};
            glBufferSubData(GL_ARRAY_BUFFER, 104 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLuint d = 0 | (1023 << 10) | (511 << 20) | (2u << 30);
            glBufferSubData(GL_ARRAY_BUFFER, 108 + kStride, sizeof(d), &d);
        }
        /* */
        {
            GLint d = (-511 & 0x3ff) | (511 << 10) | (255 << 20) | 3 << 30;
            glBufferSubData(GL_ARRAY_BUFFER, 112 + kStride, sizeof(d), &d);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribFormat(0, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0);
        glVertexAttribFormat(1, 4, GL_UNSIGNED_SHORT, GL_TRUE, 4);
        glVertexAttribFormat(2, 4, GL_UNSIGNED_INT, GL_TRUE, 12);
        glVertexAttribFormat(3, 4, GL_BYTE, GL_TRUE, 28);
        glVertexAttribFormat(4, 4, GL_SHORT, GL_TRUE, 32);
        glVertexAttribFormat(5, 4, GL_INT, GL_TRUE, 40);
        glVertexAttribFormat(6, 4, GL_FLOAT, GL_TRUE, 56);
        glVertexAttribFormat(7, 4, GL_HALF_FLOAT, GL_TRUE, 72);
        glVertexAttribFormat(8, 4, GL_UNSIGNED_BYTE, GL_TRUE, 104);
        glVertexAttribFormat(9, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, 108);
        glVertexAttribFormat(10, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE, 108);
        glVertexAttribFormat(11, 4, GL_INT_2_10_10_10_REV, GL_TRUE, 112);
        glVertexAttribFormat(12, 4, GL_INT_2_10_10_10_REV, GL_TRUE, 112);
        for (GLuint i = 0; i < 13; ++i)
        {
            glVertexAttribBinding(i, 0);
            glEnableVertexAttribArray(i);
        }
        glBindVertexBuffer(0, m_vbo, 0, kStride);

        expected_data[0]       = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
        expected_data[1]       = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
        expected_data[2]       = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
        expected_data[3]       = Vec4(0.0f, -1.0f, 1.0f, 0.0f);
        expected_data[4]       = Vec4(0.0f, -1.0f, 1.0f, 0.0f);
        expected_data[5]       = Vec4(0.0f, -1.0f, 1.0f, 0.0f);
        expected_data[6]       = Vec4(0.0f, 1.0f, 2.0f, 0.0f);
        expected_data[7]       = Vec4(0.0f, 10.0f, 20.0f, 0.0f);
        expected_data[8]       = Vec4(0.0f, 0.25f, 0.5f, 1.0f);
        expected_data[9]       = Vec4(0.0f, 1.0f, 0.5f, 0.33f);
        expected_data[10]      = Vec4(0.0f, 1.0f, 0.5f, 0.33f);
        expected_data[11]      = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
        expected_data[12]      = Vec4(0.0f, 1.0f, 0.5f, 0.0f);
        expected_data[0 + 15]  = Vec4(1.0f, 1.0f, 0.5f, 0.0f);
        expected_data[1 + 15]  = Vec4(1.0f, 1.0f, 0.5f, 0.0f);
        expected_data[2 + 15]  = Vec4(1.0f, 1.0f, 0.5f, 0.0f);
        expected_data[3 + 15]  = Vec4(1.0f, -1.0f, 1.0f, 0.0f);
        expected_data[4 + 15]  = Vec4(1.0f, -1.0f, 1.0f, 0.0f);
        expected_data[5 + 15]  = Vec4(1.0f, -1.0f, 1.0f, 0.0f);
        expected_data[6 + 15]  = Vec4(0.0f, 3.0f, 4.0f, 0.0f);
        expected_data[7 + 15]  = Vec4(0.0f, 30.0f, 40.0f, 0.0f);
        expected_data[8 + 15]  = Vec4(1.0f, 0.5f, 0.25f, 0.0f);
        expected_data[9 + 15]  = Vec4(0.0f, 1.0f, 0.5f, 0.66f);
        expected_data[10 + 15] = Vec4(0.0f, 1.0f, 0.5f, 0.66f);
        expected_data[11 + 15] = Vec4(-1.0f, 1.0f, 0.5f, -1.0f);
        expected_data[12 + 15] = Vec4(-1.0f, 1.0f, 0.5f, -1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.6 BasicInputCase6
//-----------------------------------------------------------------------------
class BasicInputCase6 : public BasicInputBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        const int kStride = 112;
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, kStride * 2, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLubyte d[] = {1, 2, 3, 4};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {5, 6, 7, 8};
            glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {9, 10, 11, 12};
            glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
        }
        /* */
        {
            GLbyte d[] = {-1, 2, -3, 4};
            glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(d), d);
        }
        /* */
        {
            GLshort d[] = {-5, 6, -7, 8};
            glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {-9, 10, -11, 12};
            glBufferSubData(GL_ARRAY_BUFFER, 40, sizeof(d), d);
        }
        /* */
        {
            GLfloat d[] = {-13.0f, 14.0f, -15.0f, 16.0f};
            glBufferSubData(GL_ARRAY_BUFFER, 56, sizeof(d), d);
        }
        /* */
        {
            GLhalf d[] = {FloatToHalf(-18.0), FloatToHalf(19.0), FloatToHalf(-20.0),
                          FloatToHalf(21.0)};
            glBufferSubData(GL_ARRAY_BUFFER, 72, sizeof(d), d);
        }
        /* */
        {
            GLuint d = 0 | (11 << 10) | (12 << 20) | (2u << 30);
            glBufferSubData(GL_ARRAY_BUFFER, 104, sizeof(d), &d);
        }
        /* */
        {
            GLint d = 0 | ((0xFFFFFFF5 << 10) & (0x3ff << 10)) | (12 << 20) | (1 << 30);
            glBufferSubData(GL_ARRAY_BUFFER, 108, sizeof(d), &d);
        }

        /* */
        {
            GLubyte d[] = {22, 23, 24, 25};
            glBufferSubData(GL_ARRAY_BUFFER, 0 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {26, 27, 28, 29};
            glBufferSubData(GL_ARRAY_BUFFER, 4 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {30, 31, 32, 33};
            glBufferSubData(GL_ARRAY_BUFFER, 12 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLbyte d[] = {-34, 35, -36, 37};
            glBufferSubData(GL_ARRAY_BUFFER, 28 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLshort d[] = {-38, 39, -40, 41};
            glBufferSubData(GL_ARRAY_BUFFER, 32 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {-42, 43, -44, 45};
            glBufferSubData(GL_ARRAY_BUFFER, 40 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLfloat d[] = {-46.0f, 47.0f, -48.0f, 49.0f};
            glBufferSubData(GL_ARRAY_BUFFER, 56 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLhalf d[] = {FloatToHalf(-50.0), FloatToHalf(51.0), FloatToHalf(-52.0),
                          FloatToHalf(53.0)};
            glBufferSubData(GL_ARRAY_BUFFER, 72 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLuint d = 0 | (11 << 10) | (12 << 20) | (1 << 30);
            glBufferSubData(GL_ARRAY_BUFFER, 104 + kStride, sizeof(d), &d);
        }
        /* */
        {
            GLint d = 123 | ((0xFFFFFFFD << 10) & (0x3ff << 10)) |
                      ((0xFFFFFE0C << 20) & (0x3ff << 20)) | ((0xFFFFFFFF << 30) & (0x3 << 30));
            glBufferSubData(GL_ARRAY_BUFFER, 108 + kStride, sizeof(d), &d);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribFormat(0, 4, GL_UNSIGNED_BYTE, GL_FALSE, 0);
        glVertexAttribFormat(1, 4, GL_UNSIGNED_SHORT, GL_FALSE, 4);
        glVertexAttribFormat(2, 4, GL_UNSIGNED_INT, GL_FALSE, 12);
        glVertexAttribFormat(3, 4, GL_BYTE, GL_FALSE, 28);
        glVertexAttribFormat(4, 4, GL_SHORT, GL_FALSE, 32);
        glVertexAttribFormat(5, 4, GL_INT, GL_FALSE, 40);
        glVertexAttribFormat(6, 4, GL_FLOAT, GL_FALSE, 56);
        glVertexAttribFormat(7, 4, GL_HALF_FLOAT, GL_FALSE, 72);
        glVertexAttribFormat(8, 4, GL_UNSIGNED_INT_2_10_10_10_REV, GL_FALSE, 104);
        glVertexAttribFormat(9, 4, GL_INT_2_10_10_10_REV, GL_FALSE, 108);
        for (GLuint i = 0; i < 10; ++i)
        {
            glVertexAttribBinding(i, 0);
            glEnableVertexAttribArray(i);
        }
        glBindVertexBuffer(0, m_vbo, 0, kStride);

        expected_data[0]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[1]      = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[2]      = Vec4(9.0f, 10.0f, 11.0f, 12.0f);
        expected_data[3]      = Vec4(-1.0f, 2.0f, -3.0f, 4.0f);
        expected_data[4]      = Vec4(-5.0f, 6.0f, -7.0f, 8.0f);
        expected_data[5]      = Vec4(-9.0f, 10.0f, -11.0f, 12.0f);
        expected_data[6]      = Vec4(-13.0f, 14.0f, -15.0f, 16.0f);
        expected_data[7]      = Vec4(-18.0f, 19.0f, -20.0f, 21.0f);
        expected_data[8]      = Vec4(0.0f, 11.0f, 12.0f, 2.0f);
        expected_data[9]      = Vec4(0.0f, -11.0f, 12.0f, 1.0f);
        expected_data[0 + 15] = Vec4(22.0f, 23.0f, 24.0f, 25.0f);
        expected_data[1 + 15] = Vec4(26.0f, 27.0f, 28.0f, 29.0f);
        expected_data[2 + 15] = Vec4(30.0f, 31.0f, 32.0f, 33.0f);
        expected_data[3 + 15] = Vec4(-34.0f, 35.0f, -36.0f, 37.0f);
        expected_data[4 + 15] = Vec4(-38.0f, 39.0f, -40.0f, 41.0f);
        expected_data[5 + 15] = Vec4(-42.0f, 43.0f, -44.0f, 45.0f);
        expected_data[6 + 15] = Vec4(-46.0f, 47.0f, -48.0f, 49.0f);
        expected_data[7 + 15] = Vec4(-50.0f, 51.0f, -52.0f, 53.0f);
        expected_data[8 + 15] = Vec4(0.0f, 11.0f, 12.0f, 1.0f);
        expected_data[9 + 15] = Vec4(123.0f, -3.0f, -500.0f, -1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.8 BasicInputCase8
//-----------------------------------------------------------------------------
class BasicInputCase8 : public BasicInputBase
{

    GLuint m_vao, m_vbo[2];

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, 6 * 4, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLfloat d[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, 10 * 4, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLfloat d[] = {-1.0f, -2.0f, -3.0f, -4.0f, -5.0f, -6.0f, -7.0f, -8.0f, -9.0f, -10.0f};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(2, 1, GL_FLOAT, GL_FALSE, 4);
        glVertexAttribFormat(5, 4, GL_FLOAT, GL_FALSE, 12);
        glVertexAttribFormat(14, 2, GL_FLOAT, GL_FALSE, 8);
        glVertexAttribBinding(0, 0);
        glVertexAttribBinding(1, 1);
        glVertexAttribBinding(2, 1);
        glVertexAttribBinding(5, 15);
        glVertexAttribBinding(14, 7);
        glBindVertexBuffer(0, m_vbo[0], 0, 12);
        glBindVertexBuffer(1, m_vbo[0], 4, 4);
        glBindVertexBuffer(7, m_vbo[1], 8, 16);
        glBindVertexBuffer(15, m_vbo[1], 12, 0);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(5);
        glEnableVertexAttribArray(14);

        expected_data[0]       = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
        expected_data[1]       = Vec4(2.0f, 3.0f, 4.0f, 1.0f);
        expected_data[2]       = Vec4(3.0f, 0.0f, 0.0f, 1.0f);
        expected_data[5]       = Vec4(-7.0f, -8.0f, -9.0f, -10.0f);
        expected_data[14]      = Vec4(-5.0f, -6.0f, 0.0f, 1.0f);
        expected_data[0 + 15]  = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
        expected_data[1 + 15]  = Vec4(3.0f, 4.0f, 5.0f, 1.0f);
        expected_data[2 + 15]  = Vec4(4.0f, 0.0f, 0.0f, 1.0f);
        expected_data[5 + 15]  = Vec4(-7.0f, -8.0f, -9.0f, -10.0f);
        expected_data[14 + 15] = Vec4(-9.0f, -10.0f, 0.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.9 BasicInputCase9
//-----------------------------------------------------------------------------
class BasicInputCase9 : public BasicInputBase
{

    GLuint m_vao, m_vbo[2];

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(1.0f, 2.0f, 3.0f, 4.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(5.0f, 6.0f, 7.0f, 8.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(Vec4), &Vec4(9.0f, 10.0f, 11.0f, 12.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(10.0f, 20.0f, 30.0f, 40.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(50.0f, 60.0f, 70.0f, 80.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(2, 4, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(4, 2, GL_FLOAT, GL_FALSE, 4);
        glVertexAttribBinding(0, 0);
        glVertexAttribBinding(2, 1);
        glVertexAttribBinding(4, 3);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(4);
        glBindVertexBuffer(0, m_vbo[0], 0, 16);
        glBindVertexBuffer(1, m_vbo[0], 0, 16);
        glBindVertexBuffer(3, m_vbo[1], 4, 8);
        glVertexBindingDivisor(1, 1);

        instance_count        = 2;
        expected_data[0]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[2]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[4]      = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
        expected_data[0 + 15] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[2 + 15] = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[4 + 15] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);

        expected_data[0 + 30]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[2 + 30]      = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[4 + 30]      = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
        expected_data[0 + 15 + 30] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[2 + 15 + 30] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[4 + 15 + 30] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.11 BasicInputCase11
//-----------------------------------------------------------------------------
class BasicInputCase11 : public BasicInputBase
{

    GLuint m_vao, m_vbo[2];

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(1.0f, 2.0f, 3.0f, 4.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(5.0f, 6.0f, 7.0f, 8.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(Vec4), &Vec4(9.0f, 10.0f, 11.0f, 12.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec4) * 3, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec4), &Vec4(10.0f, 20.0f, 30.0f, 40.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 16, sizeof(Vec4), &Vec4(50.0f, 60.0f, 70.0f, 80.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(2, 4, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(4, 2, GL_FLOAT, GL_FALSE, 4);
        glVertexAttribBinding(0, 0);
        glVertexAttribBinding(2, 1);
        glVertexAttribBinding(4, 2);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(4);
        glBindVertexBuffer(0, m_vbo[0], 0, 16);
        glBindVertexBuffer(1, m_vbo[0], 0, 16);
        glBindVertexBuffer(2, m_vbo[1], 4, 8);
        glVertexBindingDivisor(1, 1);

        instance_count        = 2;
        expected_data[0]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[2]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[4]      = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
        expected_data[0 + 15] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[2 + 15] = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[4 + 15] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);

        expected_data[0 + 30]      = Vec4(1.0f, 2.0f, 3.0f, 4.0f);
        expected_data[2 + 30]      = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[4 + 30]      = Vec4(30.0f, 40.0f, 0.0f, 1.0f);
        expected_data[0 + 15 + 30] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[2 + 15 + 30] = Vec4(5.0f, 6.0f, 7.0f, 8.0f);
        expected_data[4 + 15 + 30] = Vec4(50.0f, 60.0f, 0.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// 1.2.12 BasicInputCase12
//-----------------------------------------------------------------------------
class BasicInputCase12 : public BasicInputBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 16; ++i)
        {
            glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vec3) * 2, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vec3), &Vec3(1.0f, 2.0f, 3.0f)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(Vec3), &Vec3(4.0f, 5.0f, 6.0f)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);

        // glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
        // glVertexAttribBinding(1, 1);
        // glBindVertexBuffer(1, m_vbo, 0, 12);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribBinding(0, 1);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        expected_data[0]      = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
        expected_data[1]      = Vec4(1.0f, 2.0f, 3.0f, 1.0f);
        expected_data[0 + 15] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
        expected_data[1 + 15] = Vec4(4.0f, 5.0f, 6.0f, 1.0f);
        return BasicInputBase::Run();
    }
};
//=============================================================================
// BasicInputIBase
//-----------------------------------------------------------------------------
class BasicInputIBase : public VertexAttribBindingBase
{

    GLuint m_po, m_xfbo;

  protected:
    IVec4 expected_datai[32];
    UVec4 expected_dataui[32];
    GLsizei instance_count;
    GLuint base_instance;

    virtual long Setup()
    {
        m_po = 0;
        glGenBuffers(1, &m_xfbo);
        for (int i = 0; i < 32; ++i)
        {
            expected_datai[i]  = IVec4(0);
            expected_dataui[i] = UVec4(0);
        }
        instance_count = 1;
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDisable(GL_RASTERIZER_DISCARD);
        glUseProgram(0);
        glDeleteProgram(m_po);
        glDeleteBuffers(1, &m_xfbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        const char *const glsl_vs =
            "#version 310 es" NL "layout(location = 0) in ivec4 vs_in_attribi0;" NL
            "layout(location = 1) in ivec4 vs_in_attribi1;" NL
            "layout(location = 2) in ivec4 vs_in_attribi2;" NL
            "layout(location = 3) in ivec4 vs_in_attribi3;" NL
            "layout(location = 4) in ivec4 vs_in_attribi4;" NL
            "layout(location = 5) in ivec4 vs_in_attribi5;" NL
            "layout(location = 6) in ivec4 vs_in_attribi6;" NL
            "layout(location = 7) in ivec4 vs_in_attribi7;" NL
            "layout(location = 8) in uvec4 vs_in_attribui8;" NL
            "layout(location = 9) in uvec4 vs_in_attribui9;" NL
            "layout(location = 10) in uvec4 vs_in_attribui10;" NL
            "layout(location = 11) in uvec4 vs_in_attribui11;" NL
            "layout(location = 12) in uvec4 vs_in_attribui12;" NL
            "layout(location = 13) in uvec4 vs_in_attribui13;" NL
            "layout(location = 14) in uvec4 vs_in_attribui14;" NL
            "layout(location = 15) in uvec4 vs_in_attribui15;" NL "flat out ivec4 attribi[8];" NL
            "flat out uvec4 attribui[7];" NL "void main() {" NL "  attribi[0] = vs_in_attribi0;" NL
            "  attribi[1] = vs_in_attribi1;" NL "  attribi[2] = vs_in_attribi2;" NL
            "  attribi[3] = vs_in_attribi3;" NL "  attribi[4] = vs_in_attribi4;" NL
            "  attribi[5] = vs_in_attribi5;" NL "  attribi[6] = vs_in_attribi6;" NL
            "  attribi[7] = vs_in_attribi7;" NL "  attribui[0] = vs_in_attribui8;" NL
            "  attribui[1] = vs_in_attribui9;" NL "  attribui[2] = vs_in_attribui10;" NL
            "  attribui[3] = vs_in_attribui11;" NL "  attribui[4] = vs_in_attribui12;" NL
            "  attribui[5] = vs_in_attribui13;" NL "  attribui[6] = vs_in_attribui14;" NL "}";
        const char *const glsl_fs =
            "#version 310 es" NL "precision mediump float;" NL "flat in ivec4 attribi[8];" NL
            "flat in uvec4 attribui[7];" NL "out vec4 fs_out_color;" NL "void main() {" NL
            "  fs_out_color = vec4(attribui[1]);" NL "}";
        m_po = glCreateProgram();
        /* attach shader */
        {
            const GLuint sh  = glCreateShader(GL_VERTEX_SHADER);
            const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sh, 1, &glsl_vs, NULL);
            glShaderSource(fsh, 1, &glsl_fs, NULL);
            glCompileShader(sh);
            glCompileShader(fsh);
            glAttachShader(m_po, sh);
            glAttachShader(m_po, fsh);
            glDeleteShader(sh);
            glDeleteShader(fsh);
        }
        /* setup XFB */
        {
            const GLchar *const v[2] = {"attribi", "attribui"};
            glTransformFeedbackVaryings(m_po, 2, v, GL_INTERLEAVED_ATTRIBS);
        }
        glLinkProgram(m_po);
        if (!CheckProgram(m_po))
            return ERROR;

        /* buffer data */
        {
            std::vector<GLubyte> zero(64 * 16);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_xfbo);
            glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, (GLsizeiptr)zero.size(), &zero[0],
                         GL_DYNAMIC_COPY);
        }

        glEnable(GL_RASTERIZER_DISCARD);
        glUseProgram(m_po);
        glBeginTransformFeedback(GL_POINTS);
        glDrawArraysInstanced(GL_POINTS, 0, 2, instance_count);
        glEndTransformFeedback();

        void *data =
            glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(UVec4) * 64, GL_MAP_READ_BIT);
        IVec4 *datai  = static_cast<IVec4 *>(data);
        UVec4 *dataui = (static_cast<UVec4 *>(data)) + 8;

        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 8; ++j)
            {
                if (!IsEqual(expected_datai[i * 8 + j], datai[i * 15 + j]))
                {
                    m_context.getTestContext().getLog()
                        << tcu::TestLog::Message << "Datai is: " << datai[i * 15 + j][0] << " "
                        << datai[i * 15 + j][1] << " " << datai[i * 15 + j][2] << " "
                        << datai[i * 15 + j][3]
                        << ", data should be: " << expected_datai[i * 8 + j][0] << " "
                        << expected_datai[i * 8 + j][1] << " " << expected_datai[i * 8 + j][2]
                        << " " << expected_datai[i * 8 + j][3] << ", index is: " << i * 8 + j
                        << tcu::TestLog::EndMessage;
                    return ERROR;
                }
                if (j != 7 && !IsEqual(expected_dataui[i * 8 + j], dataui[i * 15 + j]))
                {
                    m_context.getTestContext().getLog()
                        << tcu::TestLog::Message << "Dataui is: " << dataui[i * 15 + j][0] << " "
                        << dataui[i * 15 + j][1] << " " << dataui[i * 15 + j][2] << " "
                        << dataui[i * 15 + j][3]
                        << ", data should be: " << expected_datai[i * 8 + j][0] << " "
                        << expected_datai[i * 8 + j][1] << " " << expected_datai[i * 8 + j][2]
                        << " " << expected_datai[i * 8 + j][3] << ", index is: " << i * 8 + j
                        << tcu::TestLog::EndMessage;
                    return ERROR;
                }
            }
        return NO_ERROR;
    }
};
//=============================================================================
// 1.3.1 BasicInputICase1
//-----------------------------------------------------------------------------
class BasicInputICase1 : public BasicInputIBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputIBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputIBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 8; ++i)
        {
            glVertexAttribI4i(i, 0, 0, 0, 0);
            glVertexAttribI4ui(i + 8, 0, 0, 0, 0);
        }
        const int kStride = 88;
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, kStride * 2, NULL, GL_STATIC_DRAW);
        /* */
        {
            GLbyte d[] = {1, -2, 3, -4};
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(d), d);
        }
        /* */
        {
            GLshort d[] = {5, -6, 7, -8};
            glBufferSubData(GL_ARRAY_BUFFER, 4, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {9, -10, 11, -12};
            glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(d), d);
        }
        /* */
        {
            GLubyte d[] = {13, 14, 15, 16};
            glBufferSubData(GL_ARRAY_BUFFER, 28, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {17, 18, 19, 20};
            glBufferSubData(GL_ARRAY_BUFFER, 32, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {21, 22, 23, 24};
            glBufferSubData(GL_ARRAY_BUFFER, 40, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {90, -91, 92, -93};
            glBufferSubData(GL_ARRAY_BUFFER, 56, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {94, 95, 96, 97};
            glBufferSubData(GL_ARRAY_BUFFER, 72, sizeof(d), d);
        }

        /* */
        {
            GLbyte d[] = {25, -26, 27, -28};
            glBufferSubData(GL_ARRAY_BUFFER, 0 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLshort d[] = {29, -30, 31, -32};
            glBufferSubData(GL_ARRAY_BUFFER, 4 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {33, -34, 35, -36};
            glBufferSubData(GL_ARRAY_BUFFER, 12 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLubyte d[] = {37, 38, 39, 40};
            glBufferSubData(GL_ARRAY_BUFFER, 28 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLushort d[] = {41, 42, 43, 44};
            glBufferSubData(GL_ARRAY_BUFFER, 32 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {45, 46, 47, 48};
            glBufferSubData(GL_ARRAY_BUFFER, 40 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLint d[] = {98, -99, 100, -101};
            glBufferSubData(GL_ARRAY_BUFFER, 56 + kStride, sizeof(d), d);
        }
        /* */
        {
            GLuint d[] = {102, 103, 104, 105};
            glBufferSubData(GL_ARRAY_BUFFER, 72 + kStride, sizeof(d), d);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribIFormat(0, 1, GL_BYTE, 0);
        glVertexAttribIFormat(1, 2, GL_SHORT, 4);
        glVertexAttribIFormat(2, 3, GL_INT, 12);
        glVertexAttribIFormat(3, 4, GL_INT, 56);
        glVertexAttribIFormat(8, 3, GL_UNSIGNED_BYTE, 28);
        glVertexAttribIFormat(9, 2, GL_UNSIGNED_SHORT, 32);
        glVertexAttribIFormat(10, 1, GL_UNSIGNED_INT, 40);
        glVertexAttribIFormat(11, 4, GL_UNSIGNED_INT, 72);
        glVertexAttribBinding(0, 0);
        glVertexAttribBinding(1, 0);
        glVertexAttribBinding(2, 0);
        glVertexAttribBinding(3, 0);
        glVertexAttribBinding(8, 0);
        glVertexAttribBinding(9, 0);
        glVertexAttribBinding(10, 0);
        glVertexAttribBinding(11, 0);
        glBindVertexBuffer(0, m_vbo, 0, kStride);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(8);
        glEnableVertexAttribArray(9);
        glEnableVertexAttribArray(10);
        glEnableVertexAttribArray(11);

        expected_datai[0]   = IVec4(1, 0, 0, 1);
        expected_datai[1]   = IVec4(5, -6, 0, 1);
        expected_datai[2]   = IVec4(9, -10, 11, 1);
        expected_datai[3]   = IVec4(90, -91, 92, -93);
        expected_dataui[0]  = UVec4(13, 14, 15, 1);
        expected_dataui[1]  = UVec4(17, 18, 0, 1);
        expected_dataui[2]  = UVec4(21, 0, 0, 1);
        expected_dataui[3]  = UVec4(94, 95, 96, 97);
        expected_datai[8]   = IVec4(25, 0, 0, 1);
        expected_datai[9]   = IVec4(29, -30, 0, 1);
        expected_datai[10]  = IVec4(33, -34, 35, 1);
        expected_datai[11]  = IVec4(98, -99, 100, -101);
        expected_dataui[8]  = UVec4(37, 38, 39, 1);
        expected_dataui[9]  = UVec4(41, 42, 0, 1);
        expected_dataui[10] = UVec4(45, 0, 0, 1);
        expected_dataui[11] = UVec4(102, 103, 104, 105);
        return BasicInputIBase::Run();
    }
};
//=============================================================================
// 1.3.2 BasicInputICase2
//-----------------------------------------------------------------------------
class BasicInputICase2 : public BasicInputIBase
{

    GLuint m_vao, m_vbo[2];

    virtual long Setup()
    {
        BasicInputIBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputIBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(2, m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 8; ++i)
        {
            glVertexAttribI4i(i, 0, 0, 0, 0);
            glVertexAttribI4ui(i + 8, 0, 0, 0, 0);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(IVec3) * 2, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(IVec3), &IVec3(1, 2, 3)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(IVec3), &IVec3(4, 5, 6)[0]);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(UVec4), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(UVec4), &UVec4(10, 20, 30, 40)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);
        glVertexAttribIFormat(0, 3, GL_INT, 0);
        glVertexAttribIFormat(2, 2, GL_INT, 4);
        glVertexAttribIFormat(14, 1, GL_UNSIGNED_INT, 0);
        glVertexAttribBinding(0, 2);
        glVertexAttribBinding(2, 0);
        glVertexAttribBinding(14, 7);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(14);
        glBindVertexBuffer(0, m_vbo[0], 0, 8);
        glBindVertexBuffer(2, m_vbo[0], 0, 12);
        glBindVertexBuffer(7, m_vbo[1], 4, 16);
        glVertexBindingDivisor(0, 1);
        glVertexBindingDivisor(2, 0);
        glVertexBindingDivisor(7, 2);

        instance_count      = 2;
        expected_datai[0]   = IVec4(1, 2, 3, 1);
        expected_datai[2]   = IVec4(2, 3, 0, 1);
        expected_dataui[6]  = UVec4(20, 0, 0, 1);
        expected_datai[8]   = IVec4(4, 5, 6, 1);
        expected_datai[10]  = IVec4(2, 3, 0, 1);
        expected_dataui[14] = UVec4(20, 0, 0, 1);

        expected_datai[16]  = IVec4(1, 2, 3, 1);
        expected_datai[18]  = IVec4(4, 5, 0, 1);
        expected_dataui[22] = UVec4(20, 0, 0, 1);
        expected_datai[24]  = IVec4(4, 5, 6, 1);
        expected_datai[26]  = IVec4(4, 5, 0, 1);
        expected_dataui[30] = UVec4(20, 0, 0, 1);
        return BasicInputIBase::Run();
    }
};
//=============================================================================
// 1.3.3 BasicInputICase3
//-----------------------------------------------------------------------------
class BasicInputICase3 : public BasicInputIBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        BasicInputIBase::Setup();
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        BasicInputIBase::Cleanup();
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        for (GLuint i = 0; i < 8; ++i)
        {
            glVertexAttribI4i(i, 0, 0, 0, 0);
            glVertexAttribI4ui(i + 8, 0, 0, 0, 0);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(IVec3) * 2, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(IVec3), &IVec3(1, 2, 3)[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 12, sizeof(IVec3), &IVec3(4, 5, 6)[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glVertexAttribIPointer(7, 3, GL_INT, 12, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glVertexAttribIFormat(0, 2, GL_INT, 4);
        glVertexAttribBinding(0, 7);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(7);

        expected_datai[0]     = IVec4(2, 3, 0, 1);
        expected_datai[7]     = IVec4(1, 2, 3, 1);
        expected_datai[0 + 8] = IVec4(5, 6, 0, 1);
        expected_datai[7 + 8] = IVec4(4, 5, 6, 1);
        return BasicInputIBase::Run();
    }
};

class VertexAttribState : public glcts::GLWrapper
{
  public:
    int array_enabled;
    int array_size;
    int array_stride;
    int array_type;
    int array_normalized;
    int array_integer;
    int array_divisor;
    deUintptr array_pointer;
    int array_buffer_binding;
    int binding;
    int relative_offset;
    int index;

    VertexAttribState(int attribindex)
        : array_enabled(0),
          array_size(4),
          array_stride(0),
          array_type(GL_FLOAT),
          array_normalized(0),
          array_integer(0),
          array_divisor(0),
          array_pointer(0),
          array_buffer_binding(0),
          binding(attribindex),
          relative_offset(0),
          index(attribindex)
    {}

    bool stateVerify()
    {
        GLint p;
        bool status = true;
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &p);
        if (p != array_enabled)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_ENABLED(" << index << ") is "
                << p << " should be " << array_enabled << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_SIZE, &p);
        if (p != array_size)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_SIZE(" << index << ") is " << p
                << " should be " << array_size << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &p);
        if (p != array_stride)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_STRIDE(" << index << ") is "
                << p << " should be " << array_stride << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_TYPE, &p);
        if (p != array_type)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_TYPE(" << index << ") is "
                << tcu::toHex(p) << " should be " << tcu::toHex(array_type)
                << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &p);
        if (p != array_normalized)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_NORMALIZED(" << index << ") is "
                << p << " should be " << array_normalized << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_INTEGER, &p);
        if (p != array_integer)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_INTEGER(" << index << ") is "
                << p << " should be " << array_integer << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &p);
        if (p != array_divisor)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_DIVISOR(" << index << ") is "
                << p << " should be " << array_divisor << tcu::TestLog::EndMessage;
            status = false;
        }
        void *pp;
        glGetVertexAttribPointerv(index, GL_VERTEX_ATTRIB_ARRAY_POINTER, &pp);
        if (reinterpret_cast<deUintptr>(pp) != array_pointer)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_POINTER(" << index << ") is "
                << pp << " should be " << reinterpret_cast<void *>(array_pointer)
                << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &p);
        if (p != array_buffer_binding)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING(" << index
                << ") is " << p << " should be " << array_buffer_binding
                << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_BINDING, &p);
        if (static_cast<GLint>(binding) != p)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_BINDING(" << index << ") is " << p
                << " should be " << binding << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetVertexAttribiv(index, GL_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
        if (p != relative_offset)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_ATTRIB_RELATIVE_OFFSET(" << index << ") is "
                << p << " should be " << relative_offset << tcu::TestLog::EndMessage;
            status = false;
        }
        return status;
    }
};
class VertexBindingState : public glcts::GLWrapper
{
  public:
    int buffer;
    long int offset;
    int stride;
    int divisor;
    int index;

    VertexBindingState(int bindingindex)
        : buffer(0), offset(0), stride(16), divisor(0), index(bindingindex)
    {}
    bool stateVerify()
    {
        bool status = true;
        GLint p;
        glGetIntegeri_v(GL_VERTEX_BINDING_BUFFER, index, &p);
        if (p != buffer)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_BINDING_BUFFER(" << index << ") is " << p
                << " should be " << buffer << tcu::TestLog::EndMessage;
            status = false;
        }
        GLint64 p64;
        glGetInteger64i_v(GL_VERTEX_BINDING_OFFSET, index, &p64);
        if (p64 != offset)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_BINDING_OFFSET(" << index << ") is " << p64
                << " should be " << offset << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetIntegeri_v(GL_VERTEX_BINDING_STRIDE, index, &p);
        if (p != stride)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_BINDING_STRIDE(" << index << ") is " << p
                << " should be " << stride << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetIntegeri_v(GL_VERTEX_BINDING_DIVISOR, index, &p);
        if (p != divisor)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_VERTEX_BINDING_DIVISOR(" << index << ") is " << p
                << " should be " << divisor << tcu::TestLog::EndMessage;
            status = false;
        }
        return status;
    }
};
//=============================================================================
// 1.5 BasicState1
//-----------------------------------------------------------------------------
class BasicState1 : public VertexAttribBindingBase
{

    GLuint m_vao, m_vbo[3];

    virtual long Setup()
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(3, m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(3, m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        bool status = true;
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, 10000, NULL, GL_DYNAMIC_COPY);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, 10000, NULL, GL_DYNAMIC_COPY);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[2]);
        glBufferData(GL_ARRAY_BUFFER, 10000, NULL, GL_DYNAMIC_COPY);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        GLint p;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &p);
        if (p < 16)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_BINDINGS is" << p
                << "but must be at least 16." << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
        if (p < 2047)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET is" << p
                << "but must be at least 2047." << tcu::TestLog::EndMessage;
            status = false;
        }
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_STRIDE, &p);
        if (p < 2048)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_MAX_VERTEX_ATTRIB_STRIDE is" << p
                << "but must be at least 2048." << tcu::TestLog::EndMessage;
            status = false;
        }

        glBindVertexArray(m_vao);
        // check default state
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &p);
        if (0 != p)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "GL_ELEMENT_ARRAY_BUFFER_BINDING is" << p
                << "should be 0." << tcu::TestLog::EndMessage;
            status = false;
        }
        for (GLuint i = 0; i < 16; ++i)
        {
            VertexAttribState va(i);
            if (!va.stateVerify())
                status = false;
        }
        for (GLuint i = 0; i < 16; ++i)
        {
            VertexBindingState vb(i);
            if (!vb.stateVerify())
                status = false;
        }
        if (!status)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "Default state check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        VertexAttribState va0(0);
        va0.array_size       = 2;
        va0.array_type       = GL_BYTE;
        va0.array_normalized = 1;
        va0.relative_offset  = 16;
        VertexBindingState vb0(0);
        glVertexAttribFormat(0, 2, GL_BYTE, GL_TRUE, 16);
        if (!va0.stateVerify() || !vb0.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribFormat state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        VertexAttribState va2(2);
        va2.array_size      = 3;
        va2.array_type      = GL_INT;
        va2.array_integer   = 1;
        va2.relative_offset = 512;
        VertexBindingState vb2(2);
        glVertexAttribIFormat(2, 3, GL_INT, 512);
        if (!va2.stateVerify() || !vb2.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribIFormat state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        va0.array_buffer_binding = m_vbo[0];
        vb0.buffer               = m_vbo[0];
        vb0.offset               = 2048;
        vb0.stride               = 128;
        glBindVertexBuffer(0, m_vbo[0], 2048, 128);
        if (!va0.stateVerify() || !vb0.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glBindVertexBuffer state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        va2.array_buffer_binding = m_vbo[2];
        vb2.buffer               = m_vbo[2];
        vb2.offset               = 64;
        vb2.stride               = 256;
        glBindVertexBuffer(2, m_vbo[2], 64, 256);
        if (!va2.stateVerify() || !vb2.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glBindVertexBuffer state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        glVertexAttribBinding(2, 0);
        va2.binding              = 0;
        va2.array_buffer_binding = m_vbo[0];
        if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribBinding state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        VertexAttribState va15(15);
        VertexBindingState vb15(15);
        glVertexAttribBinding(0, 15);
        va0.binding              = 15;
        va0.array_buffer_binding = 0;
        if (!va0.stateVerify() || !vb0.stateVerify() || !va15.stateVerify() || !vb15.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribBinding state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        glBindVertexBuffer(15, m_vbo[1], 16, 32);
        va0.array_buffer_binding  = m_vbo[1];
        va15.array_buffer_binding = m_vbo[1];
        vb15.buffer               = m_vbo[1];
        vb15.offset               = 16;
        vb15.stride               = 32;
        if (!va0.stateVerify() || !vb0.stateVerify() || !va15.stateVerify() || !vb15.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glBindVertexBuffer state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        glVertexAttribFormat(15, 1, GL_HALF_FLOAT, GL_FALSE, 1024);
        va15.array_size      = 1;
        va15.array_type      = GL_HALF_FLOAT;
        va15.relative_offset = 1024;
        if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify() ||
            !va15.stateVerify() || !vb15.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribFormat state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[2]);
        glVertexAttribPointer(0, 4, GL_UNSIGNED_BYTE, GL_FALSE, 8, (void *)640);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        va0.array_size           = 4;
        va0.array_type           = GL_UNSIGNED_BYTE;
        va0.array_stride         = 8;
        va0.array_pointer        = 640;
        va0.relative_offset      = 0;
        va0.array_normalized     = 0;
        va0.binding              = 0;
        va0.array_buffer_binding = m_vbo[2];
        vb0.buffer               = m_vbo[2];
        vb0.offset               = 640;
        vb0.stride               = 8;
        va2.array_buffer_binding = m_vbo[2];
        if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify() ||
            !va15.stateVerify() || !vb15.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribPointer state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        glBindVertexBuffer(0, m_vbo[1], 80, 24);
        vb0.buffer               = m_vbo[1];
        vb0.offset               = 80;
        vb0.stride               = 24;
        va2.array_buffer_binding = m_vbo[1];
        va0.array_buffer_binding = m_vbo[1];
        if (!va0.stateVerify() || !vb0.stateVerify() || !va2.stateVerify() || !vb2.stateVerify() ||
            !va15.stateVerify() || !vb15.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glBindVertexBuffer state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        if (status)
            return NO_ERROR;
        else
            return ERROR;
    }
};
//=============================================================================
// 1.6 BasicState2
//-----------------------------------------------------------------------------
class BasicState2 : public VertexAttribBindingBase
{

    GLuint m_vao;

    virtual long Setup()
    {
        glGenVertexArrays(1, &m_vao);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDeleteVertexArrays(1, &m_vao);
        return NO_ERROR;
    }

    virtual long Run()
    {
        bool status = true;
        glBindVertexArray(m_vao);

        for (GLuint i = 0; i < 16; ++i)
        {
            VertexAttribState va(i);
            VertexBindingState vb(i);
            glVertexAttribDivisor(i, i + 7);
            va.array_divisor = i + 7;
            vb.divisor       = i + 7;
            if (!va.stateVerify() || !vb.stateVerify())
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "glVertexAttribDivisor state change check failed."
                    << tcu::TestLog::EndMessage;
                status = false;
            }
        }
        for (GLuint i = 0; i < 16; ++i)
        {
            VertexAttribState va(i);
            VertexBindingState vb(i);
            glVertexBindingDivisor(i, i);
            va.array_divisor = i;
            vb.divisor       = i;
            if (!va.stateVerify() || !vb.stateVerify())
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "glVertexBindingDivisor state change check failed."
                    << tcu::TestLog::EndMessage;
                status = false;
            }
        }

        glVertexAttribBinding(2, 5);
        VertexAttribState va5(5);
        va5.array_divisor = 5;
        VertexBindingState vb5(5);
        vb5.divisor = 5;
        VertexAttribState va2(2);
        va2.array_divisor = 5;  // binding state seen thru mapping
        VertexBindingState vb2(2);
        vb2.divisor = 2;
        va2.binding = 5;
        if (!va5.stateVerify() || !vb5.stateVerify() || !va2.stateVerify() || !vb2.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribBinding state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        glVertexAttribDivisor(2, 23);
        va2.binding       = 2;  // glVAD defaults mapping
        va2.array_divisor = 23;
        vb2.divisor       = 23;
        if (!va5.stateVerify() || !vb5.stateVerify() || !va2.stateVerify() || !vb2.stateVerify())
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "glVertexAttribDivisor state change check failed."
                << tcu::TestLog::EndMessage;
            status = false;
        }

        if (status)
            return NO_ERROR;
        else
            return ERROR;
    }
};
//=============================================================================
// 2.1 AdvancedBindingUpdate
//-----------------------------------------------------------------------------
class AdvancedBindingUpdate : public VertexAttribBindingBase
{
    bool pipeline;
    GLuint m_vao[2], m_vbo[2], m_ebo[2], m_vsp, m_fsp, m_ppo;

    virtual long Setup()
    {
        glGenVertexArrays(2, m_vao);
        glGenBuffers(2, m_vbo);
        glGenBuffers(2, m_ebo);
        if (pipeline)
        {
            m_vsp = m_fsp = 0;
            glGenProgramPipelines(1, &m_ppo);
        }
        else
        {
            m_ppo = 0;
        }
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDeleteVertexArrays(2, m_vao);
        glDeleteBuffers(2, m_vbo);
        glDeleteBuffers(2, m_ebo);
        if (pipeline)
        {
            glDeleteProgram(m_vsp);
            glDeleteProgram(m_fsp);
            glDeleteProgramPipelines(1, &m_ppo);
        }
        else
        {
            glUseProgram(0);
            glDeleteProgram(m_ppo);
        }
        return NO_ERROR;
    }

    virtual long Run()
    {
        const char *const glsl_vs =
            "#version 310 es" NL "layout(location = 0) in vec4 vs_in_position;" NL
            "layout(location = 1) in vec2 vs_in_color_rg;" NL
            "layout(location = 2) in float vs_in_color_b;" NL
            "layout(location = 3) in uvec3 vs_in_data0;" NL
            "layout(location = 4) in ivec2 vs_in_data1;" NL "out vec2 color_rg;" NL
            "out float color_b;" NL "flat out uvec3 data0;" NL "flat out ivec2 data1;" NL
            "void main() {" NL "  data0 = vs_in_data0;" NL "  data1 = vs_in_data1;" NL
            "  color_b = vs_in_color_b;" NL "  color_rg = vs_in_color_rg;" NL
            "  gl_Position = vs_in_position;" NL "}";
        const char *const glsl_fs =
            "#version 310 es" NL "precision highp float;" NL "precision highp int;" NL
            "in vec2 color_rg;" NL "in float color_b;" NL "flat in uvec3 data0;" NL
            "flat in ivec2 data1;" NL "out vec4 fs_out_color;" NL
            "uniform uvec3 g_expected_data0;" NL "uniform ivec2 g_expected_data1;" NL
            "void main() {" NL "  fs_out_color = vec4(color_rg, color_b, 1);" NL
            "  if (data0 != g_expected_data0) fs_out_color = vec4(1);" NL
            "  if (data1 != g_expected_data1) fs_out_color = vec4(1);" NL "}";
        if (pipeline)
        {
            m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
            m_fsp = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
            if (!CheckProgram(m_vsp) || !CheckProgram(m_fsp))
                return ERROR;
            glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);
            glUseProgramStages(m_ppo, GL_FRAGMENT_SHADER_BIT, m_fsp);
        }
        else
        {
            m_ppo            = glCreateProgram();
            const GLuint sh  = glCreateShader(GL_VERTEX_SHADER);
            const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sh, 1, &glsl_vs, NULL);
            glShaderSource(fsh, 1, &glsl_fs, NULL);
            glCompileShader(sh);
            glCompileShader(fsh);
            glAttachShader(m_ppo, sh);
            glAttachShader(m_ppo, fsh);
            glDeleteShader(sh);
            glDeleteShader(fsh);
            glLinkProgram(m_ppo);
            if (!CheckProgram(m_ppo))
                return ERROR;
        }

        const GLsizei kStride[2]  = {52, 64};
        const GLintptr kOffset[2] = {0, 8};
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[0]);
        /* first VBO */
        {
            glBufferData(GL_ARRAY_BUFFER, kOffset[0] + 4 * kStride[0], NULL, GL_STATIC_DRAW);
            GLubyte *ptr = static_cast<GLubyte *>(glMapBufferRange(
                GL_ARRAY_BUFFER, 0, kOffset[0] + 4 * kStride[0], GL_MAP_WRITE_BIT));
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[0] + 0 * kStride[0]]) = Vec2(-1.0f, -1.0f);
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[0] + 1 * kStride[0]]) = Vec2(1.0f, -1.0f);
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[0] + 2 * kStride[0]]) = Vec2(1.0f, 1.0f);
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[0] + 3 * kStride[0]]) = Vec2(-1.0f, 1.0f);
            for (int i = 0; i < 4; ++i)
            {
                *reinterpret_cast<Vec2 *>(&ptr[kOffset[0] + 8 + i * kStride[0]]) = Vec2(0.0f, 1.0f);
                *reinterpret_cast<float *>(&ptr[kOffset[0] + 16 + i * kStride[0]]) = 0.0f;
                *reinterpret_cast<UVec3 *>(&ptr[kOffset[0] + 20 + i * kStride[0]]) = UVec3(1, 2, 3);
                *reinterpret_cast<IVec2 *>(&ptr[kOffset[0] + 44 + i * kStride[0]]) = IVec2(1, 2);
            }
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo[1]);
        /* second VBO */
        {
            glBufferData(GL_ARRAY_BUFFER, kOffset[1] + 4 * kStride[1], NULL, GL_STATIC_DRAW);
            GLubyte *ptr = static_cast<GLubyte *>(glMapBufferRange(
                GL_ARRAY_BUFFER, 0, kOffset[1] + 4 * kStride[1], GL_MAP_WRITE_BIT));
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[1] + 0 * kStride[1]]) = Vec2(-1.0f, 1.0f);
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[1] + 1 * kStride[1]]) = Vec2(1.0f, 1.0f);
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[1] + 2 * kStride[1]]) = Vec2(1.0f, -1.0f);
            *reinterpret_cast<Vec2 *>(&ptr[kOffset[1] + 3 * kStride[1]]) = Vec2(-1.0f, -1.0f);
            for (int i = 0; i < 4; ++i)
            {
                *reinterpret_cast<Vec2 *>(&ptr[kOffset[1] + 8 + i * kStride[1]]) = Vec2(0.0f, 0.0f);
                *reinterpret_cast<float *>(&ptr[kOffset[1] + 16 + i * kStride[1]]) = 1.0f;
                *reinterpret_cast<UVec3 *>(&ptr[kOffset[1] + 20 + i * kStride[1]]) = UVec3(4, 5, 6);
                *reinterpret_cast<IVec2 *>(&ptr[kOffset[1] + 44 + i * kStride[1]]) = IVec2(3, 4);
            }
            glUnmapBuffer(GL_ARRAY_BUFFER);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
        /* first EBO */
        {
            GLushort data[4] = {0, 1, 3, 2};
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[1]);
        /* second EBO */
        {
            GLuint data[4] = {3, 2, 0, 1};
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
        }
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao[0]);
        glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 8);
        glVertexAttribFormat(2, 1, GL_FLOAT, GL_FALSE, 16);
        glVertexAttribIFormat(3, 3, GL_UNSIGNED_INT, 20);
        glVertexAttribIFormat(4, 2, GL_INT, 44);
        for (GLuint i = 0; i < 5; ++i)
        {
            glVertexAttribBinding(i, 0);
            glEnableVertexAttribArray(i);
        }
        glBindVertexArray(m_vao[1]);
        glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 0);
        glVertexAttribFormat(1, 2, GL_FLOAT, GL_FALSE, 8);
        glVertexAttribFormat(2, 1, GL_FLOAT, GL_FALSE, 16);
        glVertexAttribIFormat(3, 3, GL_UNSIGNED_INT, 20);
        glVertexAttribIFormat(4, 2, GL_INT, 44);
        glVertexAttribBinding(0, 1);
        glVertexAttribBinding(1, 8);
        glVertexAttribBinding(2, 1);
        glVertexAttribBinding(3, 1);
        glVertexAttribBinding(4, 8);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glEnableVertexAttribArray(3);
        glEnableVertexAttribArray(4);
        glBindVertexBuffer(1, m_vbo[1], kOffset[1], kStride[1]);
        glBindVertexBuffer(8, m_vbo[0], kOffset[0], kStride[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[1]);
        glBindVertexArray(0);

        glClear(GL_COLOR_BUFFER_BIT);
        GLuint ppo;
        if (pipeline)
        {
            glBindProgramPipeline(m_ppo);
            ppo = m_fsp;
        }
        else
        {
            glUseProgram(m_ppo);
            ppo = m_ppo;
        }
        glBindVertexArray(m_vao[0]);

        // Bind first VBO
        glProgramUniform3ui(ppo, glGetUniformLocation(ppo, "g_expected_data0"), 1, 2, 3);
        glProgramUniform2i(ppo, glGetUniformLocation(ppo, "g_expected_data1"), 1, 2);
        glBindVertexBuffer(0, m_vbo[0], kOffset[0], kStride[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1);

        bool status = true;
        if (!CheckFB(Vec3(0, 1, 0)))
            status = false;
        if (!status)
            return ERROR;

        // Bind second VBO (change all vertex attribs with one call)
        glProgramUniform3ui(ppo, glGetUniformLocation(ppo, "g_expected_data0"), 4, 5, 6);
        glProgramUniform2i(ppo, glGetUniformLocation(ppo, "g_expected_data1"), 3, 4);

        glBindVertexBuffer(0, m_vbo[1], kOffset[1], kStride[1]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[1]);
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0, 1);

        if (!CheckFB(Vec3(0, 0, 1)))
            status = false;
        if (!status)
            return ERROR;

        // Change attrib bindings (all attribs from one buffer)
        glBindVertexBuffer(0, 0, 0, 0);  // "unbind" buffer

        glProgramUniform3ui(ppo, glGetUniformLocation(ppo, "g_expected_data0"), 1, 2, 3);
        glProgramUniform2i(ppo, glGetUniformLocation(ppo, "g_expected_data1"), 1, 2);

        for (GLuint i = 0; i < 5; ++i)
            glVertexAttribBinding(i, 15);
        glBindVertexBuffer(15, m_vbo[0], kOffset[0], kStride[0]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1);

        if (!CheckFB(Vec3(0, 1, 0)))
            status = false;
        if (!status)
            return ERROR;

        // Change attrib bindings (attribs from two buffers)
        glBindVertexBuffer(15, 0, 0, 0);  // "unbind" buffer

        glProgramUniform3ui(ppo, glGetUniformLocation(ppo, "g_expected_data0"), 1, 2, 3);
        glProgramUniform2i(ppo, glGetUniformLocation(ppo, "g_expected_data1"), 3, 4);

        glBindVertexBuffer(7, m_vbo[0], kOffset[0], kStride[0]);
        glBindVertexBuffer(12, m_vbo[1], kOffset[1], kStride[1]);
        glVertexAttribBinding(0, 7);
        glVertexAttribBinding(1, 12);
        glVertexAttribBinding(2, 12);
        glVertexAttribBinding(3, 7);
        glVertexAttribBinding(4, 12);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo[0]);
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1);

        if (!CheckFB(Vec3(0, 0, 1)))
            status = false;
        if (!status)
            return ERROR;

        // Disable one of the attribs
        glClear(GL_COLOR_BUFFER_BIT);
        glProgramUniform2i(ppo, glGetUniformLocation(ppo, "g_expected_data1"), 0, 0);
        glDisableVertexAttribArray(4);
        glVertexAttribI4i(4, 0, 0, 0, 0);
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0, 1);

        if (!CheckFB(Vec3(0, 0, 1)))
            status = false;
        if (!status)
            return ERROR;

        // Change VAO
        glProgramUniform3ui(ppo, glGetUniformLocation(ppo, "g_expected_data0"), 4, 5, 6);
        glProgramUniform2i(ppo, glGetUniformLocation(ppo, "g_expected_data1"), 1, 2);

        glBindVertexArray(m_vao[1]);
        glDrawElementsInstanced(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, 0, 1);

        if (!CheckFB(Vec3(0, 1, 1)))
            status = false;
        if (!status)
            return ERROR;

        return NO_ERROR;
    }

  public:
    AdvancedBindingUpdate() : pipeline(true) {}
};
//=============================================================================
// 2.3 AdvancedIterations
//-----------------------------------------------------------------------------
class AdvancedIterations : public VertexAttribBindingBase
{

    GLuint m_po, m_vao[2], m_buffer[2];

    virtual long Setup()
    {
        m_po = 0;
        glGenVertexArrays(2, m_vao);
        glGenBuffers(2, m_buffer);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDisable(GL_RASTERIZER_DISCARD);
        glUseProgram(0);
        glDeleteProgram(m_po);
        glDeleteVertexArrays(2, m_vao);
        glDeleteBuffers(2, m_buffer);
        return NO_ERROR;
    }

    virtual long Run()
    {
        const char *const glsl_vs =
            "#version 310 es" NL "in ivec4 vs_in_data;" NL "flat out ivec4 data;" NL
            "void main() {" NL "  data = vs_in_data + 1;" NL "}";
        const char *const glsl_fs =
            "#version 310 es" NL "precision mediump float;" NL "flat in ivec4 data;" NL
            "out vec4 fs_out_color;" NL "void main() {" NL "  fs_out_color = vec4(data);" NL "}";
        m_po = glCreateProgram();
        /* attach shader */
        {
            const GLuint sh  = glCreateShader(GL_VERTEX_SHADER);
            const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sh, 1, &glsl_vs, NULL);
            glShaderSource(fsh, 1, &glsl_fs, NULL);
            glCompileShader(sh);
            glCompileShader(fsh);
            glAttachShader(m_po, sh);
            glAttachShader(m_po, fsh);
            glDeleteShader(sh);
            glDeleteShader(fsh);
        }
        if (!RelinkProgram(1))
            return ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, m_buffer[0]);
        IVec4 zero(0);
        glBufferData(GL_ARRAY_BUFFER, 16, &zero, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_buffer[1]);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, 16, &zero, GL_DYNAMIC_READ);
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, 0);

        glBindVertexArray(m_vao[0]);
        glVertexAttribIFormat(1, 4, GL_INT, 0);
        glEnableVertexAttribArray(1);
        glBindVertexBuffer(1, m_buffer[0], 0, 16);
        glBindVertexArray(m_vao[1]);
        glVertexAttribIFormat(1, 4, GL_INT, 0);
        glEnableVertexAttribArray(1);
        glBindVertexBuffer(1, m_buffer[1], 0, 16);
        glBindVertexArray(0);
        glEnable(GL_RASTERIZER_DISCARD);
        glUseProgram(m_po);

        for (int i = 0; i < 10; ++i)
        {
            glBindVertexArray(m_vao[i % 2]);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer[(i + 1) % 2]);
            glBeginTransformFeedback(GL_POINTS);
            glDrawArrays(GL_POINTS, 0, 1);
            glEndTransformFeedback();
        }
        /* */
        {
            IVec4 *data = static_cast<IVec4 *>(
                glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
            if (!IsEqual(*data, IVec4(10)))
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Data is: " << (*data)[0] << " " << (*data)[1]
                    << " " << (*data)[2] << " " << (*data)[3] << ", data should be: 10 10 10 10."
                    << tcu::TestLog::EndMessage;
                return ERROR;
            }
            glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
        }

        if (!RelinkProgram(5))
            return ERROR;
        glBindVertexArray(m_vao[0]);
        glDisableVertexAttribArray(1);
        glBindVertexBuffer(1, 0, 0, 0);
        glVertexAttribIFormat(5, 4, GL_INT, 0);
        glEnableVertexAttribArray(5);
        glBindVertexBuffer(5, m_buffer[0], 0, 16);
        glBindVertexArray(m_vao[1]);
        glDisableVertexAttribArray(1);
        glBindVertexBuffer(1, 0, 0, 0);
        glVertexAttribIFormat(5, 4, GL_INT, 0);
        glEnableVertexAttribArray(5);
        glBindVertexBuffer(7, m_buffer[1], 0, 16);
        glVertexAttribBinding(5, 7);
        glBindVertexArray(0);

        for (int i = 0; i < 10; ++i)
        {
            glBindVertexArray(m_vao[i % 2]);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer[(i + 1) % 2]);
            glBeginTransformFeedback(GL_POINTS);
            glDrawArrays(GL_POINTS, 0, 1);
            glEndTransformFeedback();
        }
        /* */
        {
            IVec4 *data = static_cast<IVec4 *>(
                glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
            if (!IsEqual(*data, IVec4(20)))
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Data is: " << (*data)[0] << " " << (*data)[1]
                    << " " << (*data)[2] << " " << (*data)[3] << ", data should be: 20 20 20 20."
                    << tcu::TestLog::EndMessage;
                return ERROR;
            }
            glUnmapBuffer(GL_TRANSFORM_FEEDBACK_BUFFER);
        }

        if (!RelinkProgram(11))
            return ERROR;
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        glBindVertexArray(m_vao[0]);
        glDisableVertexAttribArray(5);
        glBindVertexBuffer(5, 0, 0, 0);
        glVertexAttribIFormat(11, 4, GL_INT, 0);
        glEnableVertexAttribArray(11);
        for (int i = 0; i < 10; ++i)
        {
            glBindVertexBuffer(11, m_buffer[i % 2], 0, 16);
            glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_buffer[(i + 1) % 2]);
            glBeginTransformFeedback(GL_POINTS);
            glDrawArrays(GL_POINTS, 0, 1);
            glEndTransformFeedback();
        }
        /* */
        {
            IVec4 *data = static_cast<IVec4 *>(
                glMapBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, 0, sizeof(IVec4), GL_MAP_READ_BIT));
            if (!IsEqual(*data, IVec4(30)))
            {
                m_context.getTestContext().getLog()
                    << tcu::TestLog::Message << "Data is: " << (*data)[0] << " " << (*data)[1]
                    << " " << (*data)[2] << " " << (*data)[3] << ", data should be: 30 30 30 30."
                    << tcu::TestLog::EndMessage;
                return ERROR;
            }
        }

        return NO_ERROR;
    }

    bool RelinkProgram(GLuint index)
    {
        glBindAttribLocation(m_po, index, "vs_in_data");
        /* setup XFB */
        {
            const GLchar *const v[1] = {"data"};
            glTransformFeedbackVaryings(m_po, 1, v, GL_INTERLEAVED_ATTRIBS);
        }
        glLinkProgram(m_po);
        if (!CheckProgram(m_po))
            return false;
        return true;
    }
};
//=============================================================================
// 2.4 AdvancedLargeStrideAndOffsetsNewAndLegacyAPI
//-----------------------------------------------------------------------------
class AdvancedLargeStrideAndOffsetsNewAndLegacyAPI : public VertexAttribBindingBase
{
    bool pipeline;
    GLuint m_vsp, m_fsp, m_ppo, m_ssbo, m_vao, m_vbo;

    virtual long Setup()
    {
        m_vsp = 0;
        if (pipeline)
        {
            m_vsp = m_fsp = 0;
            glGenProgramPipelines(1, &m_ppo);
        }
        else
        {
            m_ppo = 0;
        }
        glGenBuffers(1, &m_ssbo);
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDisable(GL_RASTERIZER_DISCARD);
        if (pipeline)
        {
            glDeleteProgram(m_vsp);
            glDeleteProgram(m_fsp);
            glDeleteProgramPipelines(1, &m_ppo);
        }
        else
        {
            glUseProgram(0);
            glDeleteProgram(m_ppo);
        }
        glDeleteBuffers(1, &m_ssbo);
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        if (!IsSSBOInVSFSAvailable(2))
            return NOT_SUPPORTED;
        const char *const glsl_vs =
            "#version 310 es" NL "layout(location = 0) in vec2 vs_in_attrib0;" NL
            "layout(location = 4) in ivec2 vs_in_attrib1;" NL
            "layout(location = 8) in uvec2 vs_in_attrib2;" NL
            "layout(location = 15) in float vs_in_attrib3;" NL
            "layout(std430, binding = 1) buffer Output {" NL "  vec2 attrib0[4];" NL
            "  ivec2 attrib1[4];" NL "  uvec2 attrib2[4];" NL "  float attrib3[4];" NL
            "} g_output;" NL "void main() {" NL
            "  g_output.attrib0[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib0;" NL
            "  g_output.attrib1[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib1;" NL
            "  g_output.attrib2[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib2;" NL
            "  g_output.attrib3[2 * gl_InstanceID + gl_VertexID] = vs_in_attrib3;" NL "}";
        const char *const glsl_fs =
            "#version 310 es" NL "precision mediump float;" NL "out vec4 fs_out_color;" NL
            "void main() {" NL "  fs_out_color = vec4(0.5,0.5,0.5,1.0);" NL "}";
        if (pipeline)
        {
            m_vsp = glCreateShaderProgramv(GL_VERTEX_SHADER, 1, &glsl_vs);
            m_fsp = glCreateShaderProgramv(GL_FRAGMENT_SHADER, 1, &glsl_fs);
            if (!CheckProgram(m_vsp) || !CheckProgram(m_fsp))
                return ERROR;
            glUseProgramStages(m_ppo, GL_VERTEX_SHADER_BIT, m_vsp);
            glUseProgramStages(m_ppo, GL_FRAGMENT_SHADER_BIT, m_fsp);
        }
        else
        {
            m_ppo            = glCreateProgram();
            const GLuint sh  = glCreateShader(GL_VERTEX_SHADER);
            const GLuint fsh = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(sh, 1, &glsl_vs, NULL);
            glShaderSource(fsh, 1, &glsl_fs, NULL);
            glCompileShader(sh);
            glCompileShader(fsh);
            glAttachShader(m_ppo, sh);
            glAttachShader(m_ppo, fsh);
            glDeleteShader(sh);
            glDeleteShader(fsh);
            glLinkProgram(m_ppo);
            if (!CheckProgram(m_ppo))
                return ERROR;
        }

        /* vbo */
        {
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, 100000, NULL, GL_STATIC_DRAW);
            GLubyte *ptr = static_cast<GLubyte *>(
                glMapBufferRange(GL_ARRAY_BUFFER, 0, 100000, GL_MAP_WRITE_BIT));
            // attrib0
            *reinterpret_cast<Vec2 *>(&ptr[16 + 0 * 2048]) = Vec2(1.0f, 2.0f);
            *reinterpret_cast<Vec2 *>(&ptr[16 + 1 * 2048]) = Vec2(3.0f, 4.0f);
            // attrib1
            *reinterpret_cast<IVec2 *>(&ptr[128 + 0 * 2048]) = IVec2(5, 6);
            *reinterpret_cast<IVec2 *>(&ptr[128 + 1 * 2048]) = IVec2(7, 8);
            // attrib2
            *reinterpret_cast<UVec2 *>(&ptr[1024 + 0 * 2048]) = UVec2(9, 10);
            *reinterpret_cast<UVec2 *>(&ptr[1024 + 1 * 2048]) = UVec2(11, 12);
            // attrib3
            *reinterpret_cast<float *>(&ptr[2032 + 0 * 2048]) = 13.0f;
            *reinterpret_cast<float *>(&ptr[2032 + 1 * 2048]) = 14.0f;
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        // vao
        glBindVertexArray(m_vao);
        glVertexAttribFormat(0, 2, GL_FLOAT, GL_FALSE, 16);
        glVertexAttribIFormat(8, 2, GL_UNSIGNED_INT, 1024);
        glVertexAttribFormat(15, 1, GL_FLOAT, GL_FALSE, 2032);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glVertexAttribIPointer(4, 2, GL_INT, 2048, reinterpret_cast<void *>(128));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribBinding(8, 3);
        glVertexAttribBinding(15, 3);
        glBindVertexBuffer(0, m_vbo, 0, 2048);
        glBindVertexBuffer(3, m_vbo, 0, 2048);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(4);
        glEnableVertexAttribArray(8);
        glEnableVertexAttribArray(15);
        glBindVertexArray(0);

        // ssbo
        std::vector<GLubyte> data(
            (sizeof(Vec2) + sizeof(IVec2) + sizeof(UVec2) + sizeof(float)) * 4, 0xff);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)data.size(), &data[0], GL_DYNAMIC_DRAW);

        glEnable(GL_RASTERIZER_DISCARD);
        if (pipeline)
            glBindProgramPipeline(m_ppo);
        else
            glUseProgram(m_ppo);
        glBindVertexArray(m_vao);
        glDrawArraysInstanced(GL_POINTS, 0, 2, 2);

        /* */
        {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
            GLubyte *ptr = static_cast<GLubyte *>(glMapBufferRange(
                GL_SHADER_STORAGE_BUFFER, 0, (GLsizeiptr)data.size(), GL_MAP_READ_BIT));
            // attrib0
            Vec2 i0_v0_a0 = *reinterpret_cast<Vec2 *>(&ptr[0]);
            Vec2 i0_v1_a0 = *reinterpret_cast<Vec2 *>(&ptr[8]);
            Vec2 i1_v0_a0 = *reinterpret_cast<Vec2 *>(&ptr[16]);
            Vec2 i1_v1_a0 = *reinterpret_cast<Vec2 *>(&ptr[24]);
            if (!IsEqual(i0_v0_a0, Vec2(1.0f, 2.0f)))
                return ERROR;
            if (!IsEqual(i0_v1_a0, Vec2(3.0f, 4.0f)))
                return ERROR;
            if (!IsEqual(i1_v0_a0, Vec2(1.0f, 2.0f)))
                return ERROR;
            if (!IsEqual(i1_v1_a0, Vec2(3.0f, 4.0f)))
                return ERROR;
            // attrib1
            IVec2 i0_v0_a1 = *reinterpret_cast<IVec2 *>(&ptr[32]);
            IVec2 i0_v1_a1 = *reinterpret_cast<IVec2 *>(&ptr[40]);
            IVec2 i1_v0_a1 = *reinterpret_cast<IVec2 *>(&ptr[48]);
            IVec2 i1_v1_a1 = *reinterpret_cast<IVec2 *>(&ptr[56]);
            if (!IsEqual(i0_v0_a1, IVec2(5, 6)))
                return ERROR;
            if (!IsEqual(i0_v1_a1, IVec2(7, 8)))
                return ERROR;
            if (!IsEqual(i1_v0_a1, IVec2(5, 6)))
                return ERROR;
            if (!IsEqual(i1_v1_a1, IVec2(7, 8)))
                return ERROR;
            // attrib2
            UVec2 i0_v0_a2 = *reinterpret_cast<UVec2 *>(&ptr[64]);
            UVec2 i0_v1_a2 = *reinterpret_cast<UVec2 *>(&ptr[72]);
            UVec2 i1_v0_a2 = *reinterpret_cast<UVec2 *>(&ptr[80]);
            UVec2 i1_v1_a2 = *reinterpret_cast<UVec2 *>(&ptr[88]);
            if (!IsEqual(i0_v0_a2, UVec2(9, 10)))
                return ERROR;
            if (!IsEqual(i0_v1_a2, UVec2(11, 12)))
                return ERROR;
            if (!IsEqual(i1_v0_a2, UVec2(9, 10)))
                return ERROR;
            if (!IsEqual(i1_v1_a2, UVec2(11, 12)))
                return ERROR;
            // attrib3
            float i0_v0_a3 = *reinterpret_cast<float *>(&ptr[96]);
            float i0_v1_a3 = *reinterpret_cast<float *>(&ptr[100]);
            float i1_v0_a3 = *reinterpret_cast<float *>(&ptr[104]);
            float i1_v1_a3 = *reinterpret_cast<float *>(&ptr[108]);
            if (i0_v0_a3 != 13.0f)
                return ERROR;
            if (i0_v1_a3 != 14.0f)
                return ERROR;
            if (i1_v0_a3 != 13.0f)
                return ERROR;
            if (i1_v1_a3 != 14.0f)
                return ERROR;
            glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        }
        return NO_ERROR;
    }

  public:
    AdvancedLargeStrideAndOffsetsNewAndLegacyAPI() : pipeline(true) {}
};
//=============================================================================
// 4.1 NegativeBindVertexBuffer
//-----------------------------------------------------------------------------
class NegativeBindVertexBuffer : public VertexAttribBindingBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        /*
         Errors
         An INVALID_OPERATION error is generated if buffer is not zero or a name
         returned from a previous call to GenBuffers, or if such a name has since been
         deleted with DeleteBuffers.
         An INVALID_VALUE error is generated if bindingindex is greater than or
         equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.
         OpenGL 4.4 (Core Profile) - July 21, 2013
         10.3. VERTEX ARRAYS 315
         An INVALID_VALUE error is generated if stride or offset is negative, or if
         stride is greater than the value of MAX_VERTEX_ATTRIB_STRIDE.
         An INVALID_OPERATION error is generated if no vertex array object is
         bound.
         */
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 1000, NULL, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);

        glBindVertexBuffer(0, 1234, 0, 12);
        if (glGetError() != GL_INVALID_OPERATION)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_OPERATION should be generated (buffer name not genned)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }

        GLint p;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &p);
        glBindVertexBuffer(p + 1, m_vbo, 0, 12);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (bindingIndex greater than "
                   "GL_MAX_VERTEX_ATTRIB_BINDINGS)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }

        glBindVertexBuffer(0, m_vbo, -10, 12);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_VALUE should be generated (negative offset)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glBindVertexBuffer(0, m_vbo, 0, -12);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_VALUE should be generated (negative stride)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }

        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_STRIDE, &p);
        glBindVertexBuffer(0, m_vbo, 0, p + 4);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message
                                                << "INVALID_VALUE should be generated (stride "
                                                   "greater than GL_MAX_VERTEX_ATTRIB_STRIDE)."
                                                << tcu::TestLog::EndMessage;
            return ERROR;
        }

        glBindVertexArray(0);
        glBindVertexBuffer(0, m_vbo, 0, 12);
        if (glGetError() != GL_INVALID_OPERATION)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_OPERATION should be generated (default VAO)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }

        return NO_ERROR;
    }
};
//=============================================================================
// 4.2 NegativeVertexAttribFormat
//-----------------------------------------------------------------------------
class NegativeVertexAttribFormat : public VertexAttribBindingBase
{

    GLuint m_vao, m_vbo;

    virtual long Setup()
    {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        return NO_ERROR;
    }

    virtual long Run()
    {
        /*
         Errors
         An INVALID_VALUE error is generated if attribindex is greater than or
         equal to the value of MAX_VERTEX_ATTRIBS.
         An INVALID_VALUE error is generated if size is not one of the values
         shown in table 10.2 for the corresponding command.
         An INVALID_ENUM error is generated if type is not one of the parameter
         token names from table 8.2 corresponding to one of the allowed GL data types
         for that command as shown in table 10.2.
         An INVALID_OPERATION error is generated under any of the following
         conditions:
         - if no vertex array object is currently bound (see section 10.4);
         - type is INT_2_10_10_10_REV or UNSIGNED_INT_2_10_10_10_-
         REV, and size is not 4;
         An INVALID_VALUE error is generated if relativeoffset is larger than the
         value of MAX_VERTEX_ATTRIB_RELATIVE_OFFSET.
         */
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, 1000, NULL, GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(m_vao);

        GLint p;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &p);
        glVertexAttribFormat(p + 1, 4, GL_FLOAT, GL_FALSE, 0);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message
                                                << "INVALID_VALUE should be generated (attribindex "
                                                   "greater than GL_MAX_VERTEX_ATTRIBS)."
                                                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribIFormat(p + 2, 4, GL_INT, 0);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message
                                                << "INVALID_VALUE should be generated (attribindex "
                                                   "greater than GL_MAX_VERTEX_ATTRIBS)."
                                                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribFormat(0, 0, GL_FLOAT, GL_FALSE, 0);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (invalid number of components)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribFormat(0, 5, GL_FLOAT, GL_FALSE, 0);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (invalid number of components)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribIFormat(0, 5, GL_INT, 0);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (invalid number of components)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribFormat(0, 4, GL_R32F, GL_FALSE, 0);
        if (glGetError() != GL_INVALID_ENUM)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_ENUM should be generated (invalid type)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribIFormat(0, 4, GL_FLOAT, 0);
        if (glGetError() != GL_INVALID_ENUM)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_ENUM should be generated (invalid type)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribFormat(0, 3, GL_INT_2_10_10_10_REV, GL_FALSE, 0);
        if (glGetError() != GL_INVALID_OPERATION)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message
                                                << "INVALID_OPERATION should be generated (invalid "
                                                   "number of components for packed type)."
                                                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET, &p);
        glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, p + 10);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (relativeoffset greater than "
                   "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribIFormat(0, 4, GL_INT, p + 10);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (relativeoffset greater than "
                   "GL_MAX_VERTEX_ATTRIB_RELATIVE_OFFSET)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glBindVertexArray(0);
        glVertexAttribFormat(0, 4, GL_FLOAT, GL_FALSE, 0);
        if (glGetError() != GL_INVALID_OPERATION)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_OPERATION should be generated (default VAO)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glVertexAttribIFormat(0, 4, GL_INT, 0);
        if (glGetError() != GL_INVALID_OPERATION)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_OPERATION should be generated (default VAO)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        return NO_ERROR;
    }
};

//=============================================================================
// 4.3 NegativeVertexAttribBinding
//-----------------------------------------------------------------------------
class NegativeVertexAttribBinding : public VertexAttribBindingBase
{
    GLuint m_vao;

    virtual long Setup()
    {
        glGenVertexArrays(1, &m_vao);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDeleteVertexArrays(1, &m_vao);
        return NO_ERROR;
    }

    virtual long Run()
    {
        /*
         Errors
         An INVALID_VALUE error is generated if attribindex is greater than or
         equal to the value of MAX_VERTEX_ATTRIBS.
         An INVALID_VALUE error is generated if bindingindex is greater than or
         equal to the value of MAX_VERTEX_ATTRIB_BINDINGS.
         An INVALID_OPERATION error is generated if no vertex array object is
         bound.
         */
        glBindVertexArray(m_vao);
        GLint p;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &p);
        glVertexAttribBinding(p + 1, 0);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog() << tcu::TestLog::Message
                                                << "INVALID_VALUE should be generated (attribindex "
                                                   "greater than GL_MAX_VERTEX_ATTRIBS)."
                                                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glGetIntegerv(GL_MAX_VERTEX_ATTRIB_BINDINGS, &p);
        glVertexAttribBinding(0, p + 1);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (bindingIndex greater than "
                   "GL_MAX_VERTEX_ATTRIB_BINDINGS)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glBindVertexArray(0);
        glVertexAttribBinding(0, 0);
        if (glGetError() != GL_INVALID_OPERATION)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_OPERATION should be generated (default VAO)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        return NO_ERROR;
    }
};
//=============================================================================
// 4.4 NegativeVertexAttribDivisor
//-----------------------------------------------------------------------------
class NegativeVertexAttribDivisor : public VertexAttribBindingBase
{

    GLuint m_vao;

    virtual long Setup()
    {
        glGenVertexArrays(1, &m_vao);
        return NO_ERROR;
    }

    virtual long Cleanup()
    {
        glDeleteVertexArrays(1, &m_vao);
        return NO_ERROR;
    }

    virtual long Run()
    {
        /*
         Errors
         An INVALID_VALUE error is generated if index is greater than or equal to
         the value of MAX_VERTEX_ATTRIBS.
         An INVALID_OPERATION error is generated if no vertex array object is
         bound.
         */
        glBindVertexArray(m_vao);
        GLint p;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &p);
        glVertexBindingDivisor(p + 1, 1);
        if (glGetError() != GL_INVALID_VALUE)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message
                << "INVALID_VALUE should be generated (bindingIndex greater than "
                   "GL_MAX_VERTEX_ATTRIBS)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        glBindVertexArray(0);
        glVertexBindingDivisor(0, 1);
        if (glGetError() != GL_INVALID_OPERATION)
        {
            m_context.getTestContext().getLog()
                << tcu::TestLog::Message << "INVALID_OPERATION should be generated (default VAO)."
                << tcu::TestLog::EndMessage;
            return ERROR;
        }
        return NO_ERROR;
    }
};
//=============================================================================

}  // namespace
VertexAttribBindingTests::VertexAttribBindingTests(glcts::Context &context)
    : TestCaseGroup(context, "vertex_attrib_binding", "")
{}

VertexAttribBindingTests::~VertexAttribBindingTests(void) {}

void VertexAttribBindingTests::init()
{
    using namespace glcts;
    addChild(new TestSubcase(m_context, "basic-usage", TestSubcase::Create<BasicUsage>));
    addChild(new TestSubcase(m_context, "basic-input-case1", TestSubcase::Create<BasicInputCase1>));
    addChild(new TestSubcase(m_context, "basic-input-case2", TestSubcase::Create<BasicInputCase2>));
    addChild(new TestSubcase(m_context, "basic-input-case3", TestSubcase::Create<BasicInputCase3>));
    addChild(new TestSubcase(m_context, "basic-input-case4", TestSubcase::Create<BasicInputCase4>));
    addChild(new TestSubcase(m_context, "basic-input-case5", TestSubcase::Create<BasicInputCase5>));
    addChild(new TestSubcase(m_context, "basic-input-case6", TestSubcase::Create<BasicInputCase6>));
    addChild(new TestSubcase(m_context, "basic-input-case8", TestSubcase::Create<BasicInputCase8>));
    addChild(new TestSubcase(m_context, "basic-input-case9", TestSubcase::Create<BasicInputCase9>));
    addChild(
        new TestSubcase(m_context, "basic-input-case11", TestSubcase::Create<BasicInputCase11>));
    addChild(
        new TestSubcase(m_context, "basic-input-case12", TestSubcase::Create<BasicInputCase12>));
    addChild(
        new TestSubcase(m_context, "basic-inputI-case1", TestSubcase::Create<BasicInputICase1>));
    addChild(
        new TestSubcase(m_context, "basic-inputI-case2", TestSubcase::Create<BasicInputICase2>));
    addChild(
        new TestSubcase(m_context, "basic-inputI-case3", TestSubcase::Create<BasicInputICase3>));
    addChild(new TestSubcase(m_context, "basic-state1", TestSubcase::Create<BasicState1>));
    addChild(new TestSubcase(m_context, "basic-state2", TestSubcase::Create<BasicState2>));
    addChild(new TestSubcase(m_context, "advanced-bindingUpdate",
                             TestSubcase::Create<AdvancedBindingUpdate>));
    addChild(
        new TestSubcase(m_context, "advanced-iterations", TestSubcase::Create<AdvancedIterations>));
    addChild(new TestSubcase(m_context, "advanced-largeStrideAndOffsetsNewAndLegacyAPI",
                             TestSubcase::Create<AdvancedLargeStrideAndOffsetsNewAndLegacyAPI>));
    addChild(new TestSubcase(m_context, "negative-bindVertexBuffer",
                             TestSubcase::Create<NegativeBindVertexBuffer>));
    addChild(new TestSubcase(m_context, "negative-vertexAttribFormat",
                             TestSubcase::Create<NegativeVertexAttribFormat>));
    addChild(new TestSubcase(m_context, "negative-vertexAttribBinding",
                             TestSubcase::Create<NegativeVertexAttribBinding>));
    addChild(new TestSubcase(m_context, "negative-vertexAttribDivisor",
                             TestSubcase::Create<NegativeVertexAttribDivisor>));
}
}  // namespace glcts
