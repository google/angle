#include "angle_trace_gl.h"
#include "CapturedTest_ActiveTextures_ES3_Vulkan.h"

// Private Functions

void SetupReplayContextShared(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, (GLuint *)gReadBuffer);
    UpdateTextureID(1, 0);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(560));
    glGenTextures(1, (GLuint *)gReadBuffer);
    UpdateTextureID(2, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[2]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(624));
    glGenTextures(1, (GLuint *)gReadBuffer);
    UpdateTextureID(3, 0);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[3]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(688));
    glGenTextures(1, (GLuint *)gReadBuffer);
    UpdateTextureID(4, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[4]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(752));
    glGenTextures(1, (GLuint *)gReadBuffer);
    UpdateTextureID(5, 0);
    glBindTexture(GL_TEXTURE_2D, gTextureMap[5]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 9728);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 9728);
    glTexImage2D(GL_TEXTURE_2D, 0, 6408, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, (const GLubyte *)GetBinaryData(816));
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
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][0], 1, (const GLint *)GetBinaryData(880));
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][1], 1, (const GLint *)GetBinaryData(896));
    glDeleteShader(gShaderProgramMap[7]);
    glDeleteShader(gShaderProgramMap[8]);
    CreateProgram(4);
    CreateShader(GL_VERTEX_SHADER, 7);
    glShaderSource(gShaderProgramMap[7], 1, glShaderSource_string_0, 0);
    glCompileShader(gShaderProgramMap[7]);
    glAttachShader(gShaderProgramMap[4], gShaderProgramMap[7]);
    CreateShader(GL_FRAGMENT_SHADER, 8);
    glShaderSource(gShaderProgramMap[8], 1, glShaderSource_string_1, 0);
    glCompileShader(gShaderProgramMap[8]);
    glAttachShader(gShaderProgramMap[4], gShaderProgramMap[8]);
    glBindAttribLocation(gShaderProgramMap[4], 0, "a_position");
    glBindAttribLocation(gShaderProgramMap[4], 1, "a_texCoord");
    glLinkProgram(gShaderProgramMap[4]);
    UpdateUniformLocation(4, "s_texture1", 0, 1);
    UpdateUniformLocation(4, "s_texture2", 1, 1);
    glUseProgram(gShaderProgramMap[4]);
    UpdateCurrentProgramPerContext(4);
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][0], 1, (const GLint *)GetBinaryData(912));
    glUniform1iv(gUniformLocations[gCurrentProgramPerContext[gCurrentContext]][1], 1, (const GLint *)GetBinaryData(928));
    glDeleteShader(gShaderProgramMap[7]);
    glDeleteShader(gShaderProgramMap[8]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void SetupReplayContextSharedInactive(void)
{
}

// Public Functions

