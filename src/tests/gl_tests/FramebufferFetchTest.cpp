//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ExternalBufferTest:
//   Tests the correctness of EXT_shader_framebuffer_fetch_non_coherent extension.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"
#include "util/EGLWindow.h"

namespace angle
{

class FramebufferFetchNonCoherentES31 : public ANGLETest
{
  protected:
    static constexpr GLuint kMaxColorBuffer = 4u;
    static constexpr GLuint kViewportWidth  = 16u;
    static constexpr GLuint kViewportHeight = 16u;

    FramebufferFetchNonCoherentES31()
    {
        setWindowWidth(16);
        setWindowHeight(16);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
    }

    void render(GLuint coordLoc, GLboolean isFramebufferFetchProgram)
    {
        const GLfloat coords[] = {
            -1.0f, -1.0f, +1.0f, -1.0f, +1.0f, +1.0f, -1.0f, +1.0f,
        };

        const GLushort indices[] = {
            0, 1, 2, 2, 3, 0,
        };

        glViewport(0, 0, kViewportWidth, kViewportHeight);

        GLBuffer coordinatesBuffer;
        GLBuffer elementsBuffer;

        glBindBuffer(GL_ARRAY_BUFFER, coordinatesBuffer);
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(coords), coords, GL_STATIC_DRAW);
        glEnableVertexAttribArray(coordLoc);
        glVertexAttribPointer(coordLoc, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementsBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)sizeof(indices), &indices[0],
                     GL_STATIC_DRAW);

        if (isFramebufferFetchProgram)
        {
            glFramebufferFetchBarrierEXT();
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

        ASSERT_GL_NO_ERROR();
    }
};

// Testing EXT_shader_framebuffer_fetch_non_coherent with inout qualifier
TEST_P(FramebufferFetchNonCoherentES31, BasicInout)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color += u_color;
})";

    GLProgram program;
    program.makeRaster(kVS, kFS);
    glUseProgram(program);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex;
    glBindTexture(GL_TEXTURE_2D, colorBufferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0);

    ASSERT_GL_NO_ERROR();

    float color[4]      = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(program, "u_color");
    glUniform4fv(colorLocation, 1, color);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with gl_LastFragData
TEST_P(FramebufferFetchNonCoherentES31, BasicLastFragData)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 100
attribute vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS[] = R"(#version 100
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent) mediump vec4 gl_LastFragData[gl_MaxDrawBuffers];
uniform highp vec4 u_color;

void main (void)
{
    gl_FragColor = u_color + gl_LastFragData[0];
})";

    GLProgram program;
    program.makeRaster(kVS, kFS);
    glUseProgram(program);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex;
    glBindTexture(GL_TEXTURE_2D, colorBufferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0);

    ASSERT_GL_NO_ERROR();

    float color[4]      = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(program, "u_color");
    glUniform4fv(colorLocation, 1, color);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with multiple render target
TEST_P(FramebufferFetchNonCoherentES31, MultipleRenderTarget)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color0;
layout(noncoherent, location = 1) inout highp vec4 o_color1;
layout(noncoherent, location = 2) inout highp vec4 o_color2;
layout(noncoherent, location = 3) inout highp vec4 o_color3;
uniform highp vec4 u_color;

void main (void)
{
    o_color0 += u_color;
    o_color1 += u_color;
    o_color2 += u_color;
    o_color3 += u_color;
})";

    GLProgram program;
    program.makeRaster(kVS, kFS);
    glUseProgram(program);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> color0(kViewportWidth * kViewportHeight, GLColor::black);
    std::vector<GLColor> color1(kViewportWidth * kViewportHeight, GLColor::green);
    std::vector<GLColor> color2(kViewportWidth * kViewportHeight, GLColor::blue);
    std::vector<GLColor> color3(kViewportWidth * kViewportHeight, GLColor::cyan);
    GLTexture colorBufferTex[kMaxColorBuffer];
    GLenum colorAttachments[kMaxColorBuffer] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color0.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color3.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[i], GL_TEXTURE_2D,
                               colorBufferTex[i], 0);
    }
    glDrawBuffers(kMaxColorBuffer, &colorAttachments[0]);

    ASSERT_GL_NO_ERROR();

    float color[4]      = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(program, "u_color");
    glUniform4fv(colorLocation, 1, color);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::magenta);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::white);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with multiple render target using inout array
TEST_P(FramebufferFetchNonCoherentES31, MultipleRenderTargetWithInoutArray)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color[4];
uniform highp vec4 u_color;

void main (void)
{
    o_color[0] += u_color;
    o_color[1] += u_color;
    o_color[2] += u_color;
    o_color[3] += u_color;
})";

    GLProgram program;
    program.makeRaster(kVS, kFS);
    glUseProgram(program);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> color0(kViewportWidth * kViewportHeight, GLColor::black);
    std::vector<GLColor> color1(kViewportWidth * kViewportHeight, GLColor::green);
    std::vector<GLColor> color2(kViewportWidth * kViewportHeight, GLColor::blue);
    std::vector<GLColor> color3(kViewportWidth * kViewportHeight, GLColor::cyan);
    GLTexture colorBufferTex[kMaxColorBuffer];
    GLenum colorAttachments[kMaxColorBuffer] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color0.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color3.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[i], GL_TEXTURE_2D,
                               colorBufferTex[i], 0);
    }
    glDrawBuffers(kMaxColorBuffer, &colorAttachments[0]);

    ASSERT_GL_NO_ERROR();

    float color[4]      = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(program, "u_color");
    glUniform4fv(colorLocation, 1, color);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::magenta);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::white);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with multiple draw
TEST_P(FramebufferFetchNonCoherentES31, MultipleDraw)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color += u_color;
})";

    GLProgram program;
    program.makeRaster(kVS, kFS);
    glUseProgram(program);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex;
    glBindTexture(GL_TEXTURE_2D, colorBufferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0);

    ASSERT_GL_NO_ERROR();

    float color1[4]     = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(program, "u_color");
    glUniform4fv(colorLocation, 1, color1);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    render(positionLocation, GL_TRUE);

    float color2[4] = {0.0f, 0.0f, 1.0f, 1.0f};
    glUniform4fv(colorLocation, 1, color2);

    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::white);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with the order of non-fetch program and fetch
// program
TEST_P(FramebufferFetchNonCoherentES31, DrawNonFetchDrawFetch)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS1[] = R"(#version 310 es
layout(location = 0) out highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color = u_color;
})";

    constexpr char kFS2[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color += u_color;
})";

    GLProgram programNonFetch, programFetch;
    programNonFetch.makeRaster(kVS, kFS1);
    glUseProgram(programNonFetch);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex;
    glBindTexture(GL_TEXTURE_2D, colorBufferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0);

    ASSERT_GL_NO_ERROR();

    float colorRed[4]           = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocationNonFetch = glGetUniformLocation(programNonFetch, "u_color");
    glUniform4fv(colorLocationNonFetch, 1, colorRed);

    GLint positionLocationNonFetch = glGetAttribLocation(programNonFetch, "a_position");
    render(positionLocationNonFetch, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    programFetch.makeRaster(kVS, kFS2);
    glUseProgram(programFetch);

    float colorGreen[4]      = {0.0f, 1.0f, 0.0f, 1.0f};
    GLint colorLocationFetch = glGetUniformLocation(programFetch, "u_color");
    glUniform4fv(colorLocationFetch, 1, colorGreen);

    GLint positionLocationFetch = glGetAttribLocation(programFetch, "a_position");
    render(positionLocationFetch, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glUseProgram(programNonFetch);
    glUniform4fv(colorLocationNonFetch, 1, colorRed);
    render(positionLocationNonFetch, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    glUseProgram(programFetch);
    glUniform4fv(colorLocationFetch, 1, colorGreen);
    render(positionLocationFetch, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with the order of fetch program and non-fetch
// program
TEST_P(FramebufferFetchNonCoherentES31, DrawFetchDrawNonFetch)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS1[] = R"(#version 310 es
layout(location = 0) out highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color = u_color;
})";

    constexpr char kFS2[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color += u_color;
})";

    GLProgram programNonFetch, programFetch;
    programFetch.makeRaster(kVS, kFS2);
    glUseProgram(programFetch);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex;
    glBindTexture(GL_TEXTURE_2D, colorBufferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0);

    ASSERT_GL_NO_ERROR();

    float colorRed[4]        = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocationFetch = glGetUniformLocation(programFetch, "u_color");
    glUniform4fv(colorLocationFetch, 1, colorRed);

    GLint positionLocationFetch = glGetAttribLocation(programFetch, "a_position");
    render(positionLocationFetch, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    programNonFetch.makeRaster(kVS, kFS1);
    glUseProgram(programNonFetch);

    GLint colorLocationNonFetch = glGetUniformLocation(programNonFetch, "u_color");
    glUniform4fv(colorLocationNonFetch, 1, colorRed);

    GLint positionLocationNonFetch = glGetAttribLocation(programNonFetch, "a_position");
    render(positionLocationNonFetch, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    float colorGreen[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    glUseProgram(programFetch);
    glUniform4fv(colorLocationFetch, 1, colorGreen);
    render(positionLocationFetch, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glUseProgram(programNonFetch);
    glUniform4fv(colorLocationNonFetch, 1, colorRed);
    render(positionLocationNonFetch, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with the order of non-fetch program and fetch
// program with different attachments
TEST_P(FramebufferFetchNonCoherentES31, DrawNonFetchDrawFetchWithDifferentAttachments)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS1[] = R"(#version 310 es
layout(location = 0) out highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color = u_color;
})";

    constexpr char kFS2[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color0;
layout(location = 1) out highp vec4 o_color1;
layout(noncoherent, location = 2) inout highp vec4 o_color2;
layout(location = 3) out highp vec4 o_color3;
uniform highp vec4 u_color;

void main (void)
{
    o_color0 += u_color;
    o_color1 = u_color;
    o_color2 += u_color;
    o_color3 = u_color;
})";

    GLProgram programNonFetch, programFetch1;
    programNonFetch.makeRaster(kVS, kFS1);
    glUseProgram(programNonFetch);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorTex;
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    ASSERT_GL_NO_ERROR();

    float colorRed[4]           = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocationNonFetch = glGetUniformLocation(programNonFetch, "u_color");
    glUniform4fv(colorLocationNonFetch, 1, colorRed);

    GLint positionLocationNonFetch = glGetAttribLocation(programNonFetch, "a_position");
    render(positionLocationNonFetch, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    programFetch1.makeRaster(kVS, kFS2);
    glUseProgram(programFetch1);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebufferMRT1;
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferMRT1);
    std::vector<GLColor> color1(kViewportWidth * kViewportHeight, GLColor::green);
    std::vector<GLColor> color2(kViewportWidth * kViewportHeight, GLColor::blue);
    GLTexture colorBufferTex1[kMaxColorBuffer];
    GLenum colorAttachments[kMaxColorBuffer] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[i], GL_TEXTURE_2D,
                               colorBufferTex1[i], 0);
    }
    glDrawBuffers(kMaxColorBuffer, &colorAttachments[0]);

    ASSERT_GL_NO_ERROR();

    GLint colorLocation = glGetUniformLocation(programFetch1, "u_color");
    glUniform4fv(colorLocation, 1, colorRed);

    GLint positionLocation = glGetAttribLocation(programFetch1, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::magenta);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    GLFramebuffer framebufferMRT2;
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferMRT2);
    GLTexture colorBufferTex2[kMaxColorBuffer];
    glBindTexture(GL_TEXTURE_2D, colorBufferTex2[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex2[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex2[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex2[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[i], GL_TEXTURE_2D,
                               colorBufferTex2[i], 0);
    }
    glDrawBuffers(kMaxColorBuffer, &colorAttachments[0]);

    ASSERT_GL_NO_ERROR();

    glUniform4fv(colorLocation, 1, colorRed);
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::magenta);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with the order of non-fetch program and fetch
// with different programs
TEST_P(FramebufferFetchNonCoherentES31, DrawNonFetchDrawFetchWithDifferentPrograms)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS1[] = R"(#version 310 es
layout(location = 0) out highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color = u_color;
})";

    constexpr char kFS2[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color0;
layout(location = 1) out highp vec4 o_color1;
layout(noncoherent, location = 2) inout highp vec4 o_color2;
layout(location = 3) out highp vec4 o_color3;
uniform highp vec4 u_color;

void main (void)
{
    o_color0 += u_color;
    o_color1 = u_color;
    o_color2 += u_color;
    o_color3 = u_color;
})";

    constexpr char kFS3[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color0;
layout(location = 1) out highp vec4 o_color1;
layout(location = 2) out highp vec4 o_color2;
layout(noncoherent, location = 3) inout highp vec4 o_color3;
uniform highp vec4 u_color;

void main (void)
{
    o_color0 += u_color;
    o_color1 = u_color;
    o_color2 = u_color;
    o_color3 += u_color;
})";

    GLProgram programNonFetch, programFetch1, programFetch2;
    programNonFetch.makeRaster(kVS, kFS1);
    glUseProgram(programNonFetch);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorTex;
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    ASSERT_GL_NO_ERROR();

    float colorRed[4]           = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocationNonFetch = glGetUniformLocation(programNonFetch, "u_color");
    glUniform4fv(colorLocationNonFetch, 1, colorRed);

    GLint positionLocationNonFetch = glGetAttribLocation(programNonFetch, "a_position");
    render(positionLocationNonFetch, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    programFetch1.makeRaster(kVS, kFS2);
    glUseProgram(programFetch1);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebufferMRT1;
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferMRT1);
    std::vector<GLColor> color1(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex1[kMaxColorBuffer];
    GLenum colorAttachments[kMaxColorBuffer] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[i], GL_TEXTURE_2D,
                               colorBufferTex1[i], 0);
    }
    glDrawBuffers(kMaxColorBuffer, &colorAttachments[0]);

    ASSERT_GL_NO_ERROR();

    GLint colorLocation = glGetUniformLocation(programFetch1, "u_color");
    glUniform4fv(colorLocation, 1, colorRed);

    GLint positionLocation = glGetAttribLocation(programFetch1, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    programFetch2.makeRaster(kVS, kFS3);
    glUseProgram(programFetch2);

    ASSERT_GL_NO_ERROR();

    glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    GLint colorLocation1 = glGetUniformLocation(programFetch2, "u_color");
    glUniform4fv(colorLocation1, 1, colorRed);

    GLint positionLocation1 = glGetAttribLocation(programFetch2, "a_position");
    render(positionLocation1, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with the order of draw fetch, blit and draw
// fetch
TEST_P(FramebufferFetchNonCoherentES31, DrawFetchBlitDrawFetch)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS1[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color0;
layout(location = 1) out highp vec4 o_color1;
layout(noncoherent, location = 2) inout highp vec4 o_color2;
layout(location = 3) out highp vec4 o_color3;
uniform highp vec4 u_color;

void main (void)
{
    o_color0 += u_color;
    o_color1 = u_color;
    o_color2 += u_color;
    o_color3 = u_color;
})";

    GLProgram programFetch;
    programFetch.makeRaster(kVS, kFS1);
    glUseProgram(programFetch);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebufferMRT1;
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferMRT1);
    std::vector<GLColor> color1(kViewportWidth * kViewportHeight, GLColor::green);
    std::vector<GLColor> color2(kViewportWidth * kViewportHeight, GLColor::blue);
    GLTexture colorBufferTex1[kMaxColorBuffer];
    GLenum colorAttachments[kMaxColorBuffer] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex1[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[i], GL_TEXTURE_2D,
                               colorBufferTex1[i], 0);
    }
    glDrawBuffers(kMaxColorBuffer, &colorAttachments[0]);

    ASSERT_GL_NO_ERROR();

    float colorRed[4]   = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(programFetch, "u_color");
    glUniform4fv(colorLocation, 1, colorRed);

    GLint positionLocation = glGetAttribLocation(programFetch, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::magenta);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    GLFramebuffer framebufferColor;
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferColor);

    GLTexture colorTex;
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    glBindFramebuffer(GL_READ_FRAMEBUFFER_ANGLE, framebufferColor);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER_ANGLE, framebufferMRT1);

    glBlitFramebuffer(0, 0, kViewportWidth, kViewportHeight, 0, 0, kViewportWidth, kViewportHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    ASSERT_GL_NO_ERROR();

    glBindFramebuffer(GL_FRAMEBUFFER, framebufferMRT1);
    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::blue);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::blue);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::blue);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::blue);

    float colorGreen[4] = {0.0f, 1.0f, 0.0f, 1.0f};
    glUniform4fv(colorLocation, 1, colorGreen);

    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    glReadBuffer(colorAttachments[0]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::cyan);
    glReadBuffer(colorAttachments[1]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::green);
    glReadBuffer(colorAttachments[2]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::cyan);
    glReadBuffer(colorAttachments[3]);
    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::green);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing EXT_shader_framebuffer_fetch_non_coherent with program pipeline
TEST_P(FramebufferFetchNonCoherentES31, ProgramPipeline)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    const char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS1[] = R"(#version 310 es
layout(location = 0) out highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color = u_color;
})";

    constexpr char kFS2[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require

layout(noncoherent, location = 0) inout highp vec4 o_color;

uniform highp vec4 u_color;
void main (void)
{
    o_color += u_color;
})";

    GLProgram programVert, programNonFetch, programFetch;
    const char *sourceArray[3] = {kVS, kFS1, kFS2};

    GLShader vertShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &sourceArray[0], nullptr);
    glCompileShader(vertShader);
    glProgramParameteri(programVert, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(programVert, vertShader);
    glLinkProgram(programVert);
    ASSERT_GL_NO_ERROR();

    GLShader fragShader1(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader1, 1, &sourceArray[1], nullptr);
    glCompileShader(fragShader1);
    glProgramParameteri(programNonFetch, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(programNonFetch, fragShader1);
    glLinkProgram(programNonFetch);
    ASSERT_GL_NO_ERROR();

    GLShader fragShader2(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader2, 1, &sourceArray[2], nullptr);
    glCompileShader(fragShader2);
    glProgramParameteri(programFetch, GL_PROGRAM_SEPARABLE, GL_TRUE);
    glAttachShader(programFetch, fragShader2);
    glLinkProgram(programFetch);
    ASSERT_GL_NO_ERROR();

    GLProgramPipeline pipeline1, pipeline2, pipeline3, pipeline4;
    glUseProgramStages(pipeline1, GL_VERTEX_SHADER_BIT, programVert);
    glUseProgramStages(pipeline1, GL_FRAGMENT_SHADER_BIT, programNonFetch);
    glBindProgramPipeline(pipeline1);
    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex;
    glBindTexture(GL_TEXTURE_2D, colorBufferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0);

    ASSERT_GL_NO_ERROR();

    glActiveShaderProgram(pipeline1, programNonFetch);
    float colorRed[4]           = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocationNonFetch = glGetUniformLocation(programNonFetch, "u_color");
    glUniform4fv(colorLocationNonFetch, 1, colorRed);

    ASSERT_GL_NO_ERROR();

    glActiveShaderProgram(pipeline1, programVert);
    GLint positionLocation = glGetAttribLocation(programVert, "a_position");
    render(positionLocation, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    glUseProgramStages(pipeline2, GL_VERTEX_SHADER_BIT, programVert);
    glUseProgramStages(pipeline2, GL_FRAGMENT_SHADER_BIT, programFetch);
    glBindProgramPipeline(pipeline2);
    ASSERT_GL_NO_ERROR();

    glActiveShaderProgram(pipeline2, programFetch);
    float colorGreen[4]      = {0.0f, 1.0f, 0.0f, 1.0f};
    GLint colorLocationFetch = glGetUniformLocation(programFetch, "u_color");
    glUniform4fv(colorLocationFetch, 1, colorGreen);

    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glUseProgramStages(pipeline3, GL_VERTEX_SHADER_BIT, programVert);
    glUseProgramStages(pipeline3, GL_FRAGMENT_SHADER_BIT, programNonFetch);
    glBindProgramPipeline(pipeline3);
    ASSERT_GL_NO_ERROR();

    glActiveShaderProgram(pipeline3, programNonFetch);
    colorLocationNonFetch = glGetUniformLocation(programNonFetch, "u_color");
    glUniform4fv(colorLocationNonFetch, 1, colorRed);

    ASSERT_GL_NO_ERROR();

    render(positionLocation, GL_FALSE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::red);

    glUseProgramStages(pipeline4, GL_VERTEX_SHADER_BIT, programVert);
    glUseProgramStages(pipeline4, GL_FRAGMENT_SHADER_BIT, programFetch);
    glBindProgramPipeline(pipeline4);
    ASSERT_GL_NO_ERROR();

    glActiveShaderProgram(pipeline4, programFetch);
    colorLocationFetch = glGetUniformLocation(programFetch, "u_color");
    glUniform4fv(colorLocationFetch, 1, colorGreen);
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

TEST_P(FramebufferFetchNonCoherentES31, UniformUsageCombinations)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;
out highp vec2 texCoord;

void main()
{
    gl_Position = a_position;
    texCoord = (a_position.xy * 0.5) + 0.5;
})";

    constexpr char kFS[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require

layout(binding=0, offset=0) uniform atomic_uint atDiff;
uniform sampler2D tex;

layout(noncoherent, location = 0) inout highp vec4 o_color[4];
in highp vec2 texCoord;

void main()
{
    highp vec4 texColor = texture(tex, texCoord);

    if (texColor != o_color[0])
    {
        atomicCounterIncrement(atDiff);
        o_color[0] = texColor;
    }
    else
    {
        if (atomicCounter(atDiff) > 0u)
        {
            atomicCounterDecrement(atDiff);
        }
    }

    if (texColor != o_color[1])
    {
        atomicCounterIncrement(atDiff);
        o_color[1] = texColor;
    }
    else
    {
        if (atomicCounter(atDiff) > 0u)
        {
            atomicCounterDecrement(atDiff);
        }
    }

    if (texColor != o_color[2])
    {
        atomicCounterIncrement(atDiff);
        o_color[2] = texColor;
    }
    else
    {
        if (atomicCounter(atDiff) > 0u)
        {
            atomicCounterDecrement(atDiff);
        }
    }

    if (texColor != o_color[3])
    {
        atomicCounterIncrement(atDiff);
        o_color[3] = texColor;
    }
    else
    {
        if (atomicCounter(atDiff) > 0u)
        {
            atomicCounterDecrement(atDiff);
        }
    }
})";

    GLProgram program;
    program.makeRaster(kVS, kFS);
    glUseProgram(program);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> color0(kViewportWidth * kViewportHeight, GLColor::cyan);
    std::vector<GLColor> color1(kViewportWidth * kViewportHeight, GLColor::green);
    std::vector<GLColor> color2(kViewportWidth * kViewportHeight, GLColor::blue);
    std::vector<GLColor> color3(kViewportWidth * kViewportHeight, GLColor::black);
    GLTexture colorBufferTex[kMaxColorBuffer];
    GLenum colorAttachments[kMaxColorBuffer] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
                                                GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3};
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color0.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color1.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color2.data());
    glBindTexture(GL_TEXTURE_2D, colorBufferTex[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, color3.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, colorAttachments[i], GL_TEXTURE_2D,
                               colorBufferTex[i], 0);
    }
    glDrawBuffers(kMaxColorBuffer, &colorAttachments[0]);

    ASSERT_GL_NO_ERROR();

    GLBuffer atomicBuffer;
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
    glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);

    // Reset atomic counter buffer
    GLuint *userCounters;
    userCounters = static_cast<GLuint *>(glMapBufferRange(
        GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint),
        GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT));
    memset(userCounters, 0, sizeof(GLuint));
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, atomicBuffer);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    float color[4]      = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(program, "u_color");
    glUniform4fv(colorLocation, 1, color);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    for (unsigned int i = 0; i < kMaxColorBuffer; i++)
    {
        glReadBuffer(colorAttachments[i]);
        EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::black);
    }

    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, atomicBuffer);
    userCounters = static_cast<GLuint *>(
        glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_READ_BIT));
    EXPECT_EQ(*userCounters, kViewportWidth * kViewportHeight * 2);
    glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
    glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Testing that binding the location value using GLES API is conflicted to the location value of the
// fragment inout.
TEST_P(FramebufferFetchNonCoherentES31, FixedUniformLocation)
{
    ANGLE_SKIP_TEST_IF(!IsGLExtensionEnabled("GL_EXT_shader_framebuffer_fetch_non_coherent"));

    constexpr char kVS[] = R"(#version 310 es
in highp vec4 a_position;

void main (void)
{
    gl_Position = a_position;
})";

    constexpr char kFS[] = R"(#version 310 es
#extension GL_EXT_shader_framebuffer_fetch_non_coherent : require
layout(noncoherent, location = 0) inout highp vec4 o_color;

layout(location = 0) uniform highp vec4 u_color;
void main (void)
{
    o_color += u_color;
})";

    GLProgram program;
    program.makeRaster(kVS, kFS);
    glUseProgram(program);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    std::vector<GLColor> greenColor(kViewportWidth * kViewportHeight, GLColor::green);
    GLTexture colorBufferTex;
    glBindTexture(GL_TEXTURE_2D, colorBufferTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kViewportWidth, kViewportHeight, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, greenColor.data());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTex, 0);

    ASSERT_GL_NO_ERROR();

    float color[4]      = {1.0f, 0.0f, 0.0f, 1.0f};
    GLint colorLocation = glGetUniformLocation(program, "u_color");
    glUniform4fv(colorLocation, 1, color);

    GLint positionLocation = glGetAttribLocation(program, "a_position");
    render(positionLocation, GL_TRUE);

    ASSERT_GL_NO_ERROR();

    EXPECT_PIXEL_COLOR_EQ(kViewportWidth / 2, kViewportHeight / 2, GLColor::yellow);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

ANGLE_INSTANTIATE_TEST_ES31(FramebufferFetchNonCoherentES31);
}  // namespace angle
