//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WebGLCompatibilityTest.cpp : Tests of the GL_ANGLE_webgl_compatibility extension.

#include "test_utils/ANGLETest.h"

#include "test_utils/gl_raii.h"

namespace angle
{

class WebGLCompatibilityTest : public ANGLETest
{
  protected:
    WebGLCompatibilityTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setWebGLCompatibilityEnabled(true);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();
        glRequestExtensionANGLE = reinterpret_cast<PFNGLREQUESTEXTENSIONANGLEPROC>(
            eglGetProcAddress("glRequestExtensionANGLE"));
    }

    void TearDown() override { ANGLETest::TearDown(); }

    PFNGLREQUESTEXTENSIONANGLEPROC glRequestExtensionANGLE = nullptr;
};

class WebGL2CompatibilityTest : public WebGLCompatibilityTest
{
};

// Context creation would fail if EGL_ANGLE_create_context_webgl_compatibility was not available so
// the GL extension should always be present
TEST_P(WebGLCompatibilityTest, ExtensionStringExposed)
{
    EXPECT_TRUE(extensionEnabled("GL_ANGLE_webgl_compatibility"));
}

// Verify that all extension entry points are available
TEST_P(WebGLCompatibilityTest, EntryPoints)
{
    if (extensionEnabled("GL_ANGLE_request_extension"))
    {
        EXPECT_NE(nullptr, eglGetProcAddress("glRequestExtensionANGLE"));
    }
}

// WebGL 1 allows GL_DEPTH_STENCIL_ATTACHMENT as a valid binding point.  Make sure it is usable,
// even in ES2 contexts.
TEST_P(WebGLCompatibilityTest, DepthStencilBindingPoint)
{
    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer.get());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 32, 32);

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              renderbuffer.get());

    EXPECT_GL_NO_ERROR();
}

// Test that attempting to enable an extension that doesn't exist generates GL_INVALID_OPERATION
TEST_P(WebGLCompatibilityTest, EnableExtensionValidation)
{
    glRequestExtensionANGLE("invalid_extension_string");
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Test enabling the GL_OES_element_index_uint extension
TEST_P(WebGLCompatibilityTest, EnableExtensionUintIndices)
{
    if (getClientMajorVersion() != 2)
    {
        // This test only works on ES2 where uint indices are not available by default
        return;
    }

    EXPECT_FALSE(extensionEnabled("GL_OES_element_index_uint"));

    GLBuffer indexBuffer;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.get());

    GLuint data[] = {0, 1, 2, 1, 3, 2};
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

    ANGLE_GL_PROGRAM(program, "void main() { gl_Position = vec4(0, 0, 0, 1); }",
                     "void main() { gl_FragColor = vec4(0, 1, 0, 1); }")
    glUseProgram(program.get());

    glDrawElements(GL_TRIANGLES, 2, GL_UNSIGNED_INT, nullptr);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);

    if (extensionRequestable("GL_OES_element_index_uint"))
    {
        glRequestExtensionANGLE("GL_OES_element_index_uint");
        EXPECT_GL_NO_ERROR();
        EXPECT_TRUE(extensionEnabled("GL_OES_element_index_uint"));

        glDrawElements(GL_TRIANGLES, 2, GL_UNSIGNED_INT, nullptr);
        EXPECT_GL_NO_ERROR();
    }
}

// Verify that shaders are of a compatible spec when the extension is enabled.
TEST_P(WebGLCompatibilityTest, ExtensionCompilerSpec)
{
    EXPECT_TRUE(extensionEnabled("GL_ANGLE_webgl_compatibility"));

    // Use of reserved _webgl prefix should fail when the shader specification is for WebGL.
    const std::string &vert =
        "struct Foo {\n"
        "    int _webgl_bar;\n"
        "};\n"
        "void main()\n"
        "{\n"
        "    Foo foo = Foo(1);\n"
        "}";

    // Default fragement shader.
    const std::string &frag =
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(1.0,0.0,0.0,1.0);\n"
        "}";

    GLuint program = CompileProgram(vert, frag);
    EXPECT_EQ(0u, program);
    glDeleteProgram(program);
}

// Test that client-side array buffers are forbidden in WebGL mode
TEST_P(WebGLCompatibilityTest, ForbidsClientSideArrayBuffer)
{
    const std::string &vert =
        "attribute vec3 a_pos;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_pos, 1.0);\n"
        "}\n";

    const std::string &frag =
        "precision highp float;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(1.0);\n"
        "}\n";

    ANGLE_GL_PROGRAM(program, vert, frag);

    GLint posLocation = glGetAttribLocation(program.get(), "a_pos");
    ASSERT_NE(-1, posLocation);
    glUseProgram(program.get());

    const auto &vertices = GetQuadVertices();
    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 4, vertices.data());
    glEnableVertexAttribArray(posLocation);

    ASSERT_GL_NO_ERROR();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Test that client-side element array buffers are forbidden in WebGL mode
TEST_P(WebGLCompatibilityTest, ForbidsClientSideElementBuffer)
{
    const std::string &vert =
        "attribute vec3 a_pos;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_pos, 1.0);\n"
        "}\n";

    const std::string &frag =
        "precision highp float;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(1.0);\n"
        "}\n";

    ANGLE_GL_PROGRAM(program, vert, frag);

    GLint posLocation = glGetAttribLocation(program.get(), "a_pos");
    ASSERT_NE(-1, posLocation);
    glUseProgram(program.get());

    const auto &vertices = GetQuadVertices();

    GLBuffer vertexBuffer;
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer.get());
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices[0]) * vertices.size(), vertices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(posLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(posLocation);

    const GLubyte indices[] = {0, 1, 2, 3, 4, 5};

    ASSERT_GL_NO_ERROR();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, indices);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
}

// Tests the WebGL requirement of having the same stencil mask, writemask and ref for fron and back
TEST_P(WebGLCompatibilityTest, RequiresSameStencilMaskAndRef)
{
    // Run the test in an FBO to make sure we have some stencil bits.
    GLRenderbuffer renderbuffer;
    glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer.get());
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 32, 32);

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                              renderbuffer.get());

    ANGLE_GL_PROGRAM(program, "void main() { gl_Position = vec4(0, 0, 0, 1); }",
                     "void main() { gl_FragColor = vec4(0, 1, 0, 1); }")
    glUseProgram(program.get());
    ASSERT_GL_NO_ERROR();

    // Having ref and mask the same for front and back is valid.
    glStencilMask(255);
    glStencilFunc(GL_ALWAYS, 0, 255);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Having a different front - back write mask generates an error.
    glStencilMaskSeparate(GL_FRONT, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Setting both write masks separately to the same value is valid.
    glStencilMaskSeparate(GL_BACK, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Having a different stencil front - back mask generates an error
    glStencilFuncSeparate(GL_FRONT, GL_ALWAYS, 0, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Setting both masks separately to the same value is valid.
    glStencilFuncSeparate(GL_BACK, GL_ALWAYS, 0, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Having a different stencil front - back reference generates an error
    glStencilFuncSeparate(GL_FRONT, GL_ALWAYS, 255, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Setting both references separately to the same value is valid.
    glStencilFuncSeparate(GL_BACK, GL_ALWAYS, 255, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();

    // Using different stencil funcs, everything being equal is valid.
    glStencilFuncSeparate(GL_BACK, GL_NEVER, 255, 1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    ASSERT_GL_NO_ERROR();
}

// Test that GL_FIXED is forbidden
TEST_P(WebGLCompatibilityTest, ForbidsGLFixed)
{
    GLBuffer buffer;
    glBindBuffer(GL_ARRAY_BUFFER, buffer.get());
    glBufferData(GL_ARRAY_BUFFER, 16, nullptr, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
    ASSERT_GL_NO_ERROR();

    glVertexAttribPointer(0, 1, GL_FIXED, GL_FALSE, 0, nullptr);
    EXPECT_GL_ERROR(GL_INVALID_ENUM);
}

// Test the WebGL limit of 255 for the attribute stride
TEST_P(WebGLCompatibilityTest, MaxStride)
{
    GLBuffer buffer;
    glBindBuffer(GL_ARRAY_BUFFER, buffer.get());
    glBufferData(GL_ARRAY_BUFFER, 1024, nullptr, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 255, nullptr);
    ASSERT_GL_NO_ERROR();

    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 256, nullptr);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

// Test the checks for OOB reads in the vertex buffers, non-instanced version
TEST_P(WebGLCompatibilityTest, DrawArraysBufferOutOfBoundsNonInstanced)
{
    const std::string &vert =
        "attribute float a_pos;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_pos, a_pos, a_pos, 1.0);\n"
        "}\n";

    const std::string &frag =
        "precision highp float;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(1.0);\n"
        "}\n";

    ANGLE_GL_PROGRAM(program, vert, frag);

    GLint posLocation = glGetAttribLocation(program.get(), "a_pos");
    ASSERT_NE(-1, posLocation);
    glUseProgram(program.get());

    GLBuffer buffer;
    glBindBuffer(GL_ARRAY_BUFFER, buffer.get());
    glBufferData(GL_ARRAY_BUFFER, 16, nullptr, GL_STATIC_DRAW);

    glEnableVertexAttribArray(posLocation);

    const uint8_t* zeroOffset = nullptr;

    // Test touching the last element is valid.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0, zeroOffset + 12);
    glDrawArrays(GL_POINTS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Test touching the last element + 1 is invalid.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0, zeroOffset + 13);
    glDrawArrays(GL_POINTS, 0, 4);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Test touching the last element is valid, using a stride.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 2, zeroOffset + 9);
    glDrawArrays(GL_POINTS, 0, 4);
    ASSERT_GL_NO_ERROR();

    // Test touching the last element + 1 is invalid, using a stride.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 2, zeroOffset + 10);
    glDrawArrays(GL_POINTS, 0, 4);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Test any offset is valid if no vertices are drawn.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0, zeroOffset + 32);
    glDrawArrays(GL_POINTS, 0, 0);
    ASSERT_GL_NO_ERROR();
}

// Tests that NPOT is not enabled by default in WebGL 1 and that it can be enabled
TEST_P(WebGLCompatibilityTest, NPOT)
{
    EXPECT_FALSE(extensionEnabled("GL_OES_texture_npot"));

    // Create a texture and set an NPOT mip 0, should always be acceptable.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 10, 10, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    ASSERT_GL_NO_ERROR();

    // Try setting an NPOT mip 1 and verify the error if WebGL 1
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 5, 5, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (getClientMajorVersion() < 3)
    {
        ASSERT_GL_ERROR(GL_INVALID_VALUE);
    }
    else
    {
        ASSERT_GL_NO_ERROR();
    }

    if (extensionRequestable("GL_OES_texture_npot"))
    {
        glRequestExtensionANGLE("GL_OES_texture_npot");
        ASSERT_GL_NO_ERROR();

        // Try again to set NPOT mip 1
        glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 5, 5, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        ASSERT_GL_NO_ERROR();
    }
}

// Test the checks for OOB reads in the vertex buffers, instanced version
TEST_P(WebGL2CompatibilityTest, DrawArraysBufferOutOfBoundsInstanced)
{
    const std::string &vert =
        "attribute float a_pos;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(a_pos, a_pos, a_pos, 1.0);\n"
        "}\n";

    const std::string &frag =
        "precision highp float;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = vec4(1.0);\n"
        "}\n";

    ANGLE_GL_PROGRAM(program, vert, frag);

    GLint posLocation = glGetAttribLocation(program.get(), "a_pos");
    ASSERT_NE(-1, posLocation);
    glUseProgram(program.get());

    GLBuffer buffer;
    glBindBuffer(GL_ARRAY_BUFFER, buffer.get());
    glBufferData(GL_ARRAY_BUFFER, 16, nullptr, GL_STATIC_DRAW);

    glEnableVertexAttribArray(posLocation);
    glVertexAttribDivisor(posLocation, 1);

    const uint8_t* zeroOffset = nullptr;

    // Test touching the last element is valid.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0, zeroOffset + 12);
    glDrawArraysInstanced(GL_POINTS, 0, 1, 4);
    ASSERT_GL_NO_ERROR();

    // Test touching the last element + 1 is invalid.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0, zeroOffset + 13);
    glDrawArraysInstanced(GL_POINTS, 0, 1, 4);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Test touching the last element is valid, using a stride.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 2, zeroOffset + 9);
    glDrawArraysInstanced(GL_POINTS, 0, 1, 4);
    ASSERT_GL_NO_ERROR();

    // Test touching the last element + 1 is invalid, using a stride.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 2, zeroOffset + 10);
    glDrawArraysInstanced(GL_POINTS, 0, 1, 4);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Test any offset is valid if no vertices are drawn.
    glVertexAttribPointer(0, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0, zeroOffset + 32);
    glDrawArraysInstanced(GL_POINTS, 0, 1, 0);
    ASSERT_GL_NO_ERROR();
}

// Tests that a rendering feedback loop triggers a GL error under WebGL.
// Based on WebGL test conformance/renderbuffers/feedback-loop.html.
TEST_P(WebGLCompatibilityTest, RenderingFeedbackLoop)
{
    const std::string vertexShader =
        "attribute vec4 a_position;\n"
        "varying vec2 v_texCoord;\n"
        "void main() {\n"
        "    gl_Position = a_position;\n"
        "    v_texCoord = (a_position.xy * 0.5) + 0.5;\n"
        "}\n";

    const std::string fragmentShader =
        "precision mediump float;\n"
        "varying vec2 v_texCoord;\n"
        "uniform sampler2D u_texture;\n"
        "void main() {\n"
        "    // Shader swizzles color channels so we can tell if the draw succeeded.\n"
        "    gl_FragColor = texture2D(u_texture, v_texCoord).gbra;\n"
        "}\n";

    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &GLColor::red);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ASSERT_GL_NO_ERROR();

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.get(), 0);

    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));

    ANGLE_GL_PROGRAM(program, vertexShader, fragmentShader);

    GLint uniformLoc = glGetUniformLocation(program.get(), "u_texture");
    ASSERT_NE(-1, uniformLoc);

    glUseProgram(program.get());
    glUniform1i(uniformLoc, 0);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    ASSERT_GL_NO_ERROR();

    // Drawing with a texture that is also bound to the current framebuffer should fail
    glBindTexture(GL_TEXTURE_2D, texture.get());
    drawQuad(program.get(), "a_position", 0.5f, 1.0f, true);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Ensure that the texture contents did not change after the previous render
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawQuad(program.get(), "a_position", 0.5f, 1.0f, true);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::blue);

    // Drawing when texture is bound to an inactive uniform should succeed
    GLTexture texture2;
    glBindTexture(GL_TEXTURE_2D, texture2.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &GLColor::green);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture.get());
    drawQuad(program.get(), "a_position", 0.5f, 1.0f, true);
    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
}

// Test tests that texture copying feedback loops are properly rejected in WebGL.
// Based on the WebGL test conformance/textures/misc/texture-copying-feedback-loops.html
TEST_P(WebGLCompatibilityTest, TextureCopyingFeedbackLoops)
{
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLTexture texture2;
    glBindTexture(GL_TEXTURE_2D, texture2.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLFramebuffer framebuffer;
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.get());
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.get(), 0);

    // framebuffer should be FRAMEBUFFER_COMPLETE.
    ASSERT_GLENUM_EQ(GL_FRAMEBUFFER_COMPLETE, glCheckFramebufferStatus(GL_FRAMEBUFFER));
    ASSERT_GL_NO_ERROR();

    // testing copyTexImage2D

    // copyTexImage2D to same texture but different level
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glCopyTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 0, 0, 2, 2, 0);
    EXPECT_GL_NO_ERROR();

    // copyTexImage2D to same texture same level, invalid feedback loop
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 2, 2, 0);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // copyTexImage2D to different texture
    glBindTexture(GL_TEXTURE_2D, texture2.get());
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, 2, 2, 0);
    EXPECT_GL_NO_ERROR();

    // testing copyTexSubImage2D

    // copyTexSubImage2D to same texture but different level
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 1, 0, 0, 0, 0, 1, 1);
    EXPECT_GL_NO_ERROR();

    // copyTexSubImage2D to same texture same level, invalid feedback loop
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1, 1);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // copyTexSubImage2D to different texture
    glBindTexture(GL_TEXTURE_2D, texture2.get());
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, 1, 1);
    EXPECT_GL_NO_ERROR();
}

// Test for the max draw buffers and color attachments.
TEST_P(WebGLCompatibilityTest, MaxDrawBuffersAttachmentPoints)
{
    // This test only applies to ES2.
    if (getClientMajorVersion() != 2)
    {
        return;
    }

    GLFramebuffer fbo[2];
    glBindFramebuffer(GL_FRAMEBUFFER, fbo[0].get());

    // Test that is valid when we bind with a single attachment point.
    GLTexture texture;
    glBindTexture(GL_TEXTURE_2D, texture.get());
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.get(), 0);
    ASSERT_GL_NO_ERROR();

    // Test that enabling the draw buffers extension will allow us to bind with a non-zero
    // attachment point.
    if (extensionRequestable("GL_EXT_draw_buffers"))
    {
        glRequestExtensionANGLE("GL_EXT_draw_buffers");
        EXPECT_GL_NO_ERROR();
        EXPECT_TRUE(extensionEnabled("GL_EXT_draw_buffers"));

        glBindFramebuffer(GL_FRAMEBUFFER, fbo[1].get());

        GLTexture texture2;
        glBindTexture(GL_TEXTURE_2D, texture2.get());
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texture2.get(),
                               0);
        ASSERT_GL_NO_ERROR();
    }
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST(WebGLCompatibilityTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES3_OPENGL(),
                       ES2_OPENGLES(),
                       ES3_OPENGLES());

ANGLE_INSTANTIATE_TEST(WebGL2CompatibilityTest,
                       ES3_D3D11(),
                       ES3_OPENGL(),
                       ES3_OPENGLES());

}  // namespace
