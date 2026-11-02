#include "CapturedTest_ActiveTextures_ES3_Vulkan.h"
#include "angle_trace_gl.h"

const char *const glShaderSource_string_0[] = { 
"attribute vec4 a_position;\n"
"attribute vec2 a_texCoord;\n"
"varying vec2 v_texCoord;\n"
"void main()\n"
"{\n"
"    gl_Position = a_position;\n"
"    v_texCoord = a_texCoord;\n"
"}",
};
const char *const glShaderSource_string_1[] = { 
"precision mediump float;\n"
"varying vec2 v_texCoord;\n"
"uniform sampler2D s_texture1;\n"
"uniform sampler2D s_texture2;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(s_texture1, v_texCoord) + texture2D(s_texture2, v_texCoord);\n"
"}",
};

// Private Functions

void SetupReplayContext3(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[3]);
    UpdateCurrentContext(3);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[2]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[4]);
    glActiveTexture(GL_TEXTURE1);
    glGenFramebuffers(1, (GLuint *)gReadBuffer);
    UpdateFramebufferID2(3, 3, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][3]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gTextureMap[4], 0);
    glGenFramebuffers(1, (GLuint *)gReadBuffer);
    UpdateFramebufferID2(3, 4, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][4]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gTextureMap[5], 0);
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][0]);
    glUseProgram(gShaderProgramMap[4]);
    UpdateCurrentProgramPerContext(4);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformFeedbackMap[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glViewport(0, 0, 128, 128);
    glScissor(0, 0, 128, 128);
}

void ReplayFrame1(void)
{
    eglGetError();
    glGetError();
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)gReadBuffer);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)gReadBuffer);
    glGetAttribLocation(gShaderProgramMap[4], "a_position");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, gClientArrays[0]);
    glEnableVertexAttribArray(0);
    UpdateClientArrayPointer(0, (const GLubyte *)GetBinaryData(0), 72);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glGetError();
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void *)gReadBuffer);
    glGetError();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[2]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[3]);
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)gReadBuffer);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)gReadBuffer);
    glGetAttribLocation(gShaderProgramMap[4], "a_position");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, gClientArrays[0]);
    glEnableVertexAttribArray(0);
    UpdateClientArrayPointer(0, (const GLubyte *)GetBinaryData(80), 72);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glGetError();
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void *)gReadBuffer);
    glGetError();
    glUseProgram(gShaderProgramMap[1]);
    UpdateCurrentProgramPerContext(1);
    glUniform1i(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][0], 1);
    glDeleteProgram(gShaderProgramMap[1]);
    DeleteUniformLocations(gShaderProgramMap[1]);
    glUseProgram(gShaderProgramMap[4]);
    UpdateCurrentProgramPerContext(4);
    glUniform1i(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][0], 1);
    glUniform1i(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][1], 0);
    glUniform1i(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][1], 1);
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint *)gReadBuffer);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint *)gReadBuffer);
    glGetAttribLocation(gShaderProgramMap[4], "a_position");
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, gClientArrays[0]);
    glEnableVertexAttribArray(0);
    UpdateClientArrayPointer(0, (const GLubyte *)GetBinaryData(160), 72);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glGetError();
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void *)gReadBuffer);
    glGetError();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9729);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(240));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9985);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9729);
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][3]);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][4]);
    glClear(GL_COLOR_BUFFER_BIT);
    glGenTextures(1, (GLuint *)gReadBuffer);
    UpdateTextureID(6, 0);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[6]);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glGenFramebuffers(1, (GLuint *)gReadBuffer);
    UpdateFramebufferID2(3, 5, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][5]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gTextureMap[6], 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][0]);
    glGetError();
}

void ReplayFrame2(void)
{
    eglGetError();
}

void ReplayFrame3(void)
{
    eglGetError();
}

void ReplayFrame4(void)
{
    eglGetError();
}

void ResetReplayContextShared(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    CreateProgram(1);
    CreateShader(GL_VERTEX_SHADER, 7);
    glShaderSource(gShaderProgramMap[7], 1, glShaderSource_string_0, 0);
    glCompileShader(gShaderProgramMap[7]);
    glAttachShader(gShaderProgramMap[1], gShaderProgramMap[7]);
    CreateShader(GL_FRAGMENT_SHADER, 8);
    glShaderSource(gShaderProgramMap[8], 1, glShaderSource_string_1, 0);
    glCompileShader(gShaderProgramMap[8]);
    glAttachShader(gShaderProgramMap[1], gShaderProgramMap[8]);
    glBindAttribLocation(gShaderProgramMap[1], 0, "a_position");
    glBindAttribLocation(gShaderProgramMap[1], 1, "a_texCoord");
    glLinkProgram(gShaderProgramMap[1]);
    UpdateUniformLocation(1, "s_texture1", 0, 1);
    UpdateUniformLocation(1, "s_texture2", 1, 1);
    glUseProgram(gShaderProgramMap[1]);
    UpdateCurrentProgramPerContext(1);
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][0], 1, (const GLint *)GetBinaryData(304));
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][1], 1, (const GLint *)GetBinaryData(320));
    glDeleteShader(gShaderProgramMap[7]);
    glDeleteShader(gShaderProgramMap[8]);
    UpdateResourceIDBuffer(0, gTextureMap[6]);
    glDeleteTextures(1, gResourceIDBuffer);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(336));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[4]);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(400));
    glBindTexture(GL_TEXTURE_2D, gTextureMap[5]);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(464));
    glBindTexture(GL_TEXTURE_2D, gTextureMap[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728);
    glUseProgram(gShaderProgramMap[4]);
    UpdateCurrentProgramPerContext(4);
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][0], 1, (const GLint *)GetBinaryData(528));
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][1], 1, (const GLint *)GetBinaryData(544));
}

void ResetReplayContext3(void)
{
    UpdateResourceIDBuffer(0, gFramebufferMapPerContext[3][5]);
    glDeleteFramebuffers(1, gResourceIDBuffer);
}

void ReplayFrame5(void)
{
    eglGetError();
}

// Public Functions

void SetupReplay(void)
{
    InitReplay();
    SetupReplayContextShared();
    if (gReplayResourceMode == angle::ReplayResourceMode::All)
    {
        SetupReplayContextSharedInactive();
    }
    SetCurrentContextID(3);
    SetupReplayContext3();

}

void ResetReplay(void)
{
    ResetReplayContextShared();
    ResetReplayContext3();

    // Reset main context state
    glBindFramebuffer(GL_FRAMEBUFFER, gFramebufferMapPerContext[3][0]);
    glUseProgram(gShaderProgramMap[4]);
    UpdateCurrentProgramPerContext(4);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[1]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[2]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[4]);
    glActiveTexture(GL_TEXTURE1);
}

