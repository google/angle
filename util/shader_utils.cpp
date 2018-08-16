//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "shader_utils.h"

#include <vector>
#include <iostream>
#include <fstream>

static std::string ReadFileToString(const std::string &source)
{
    std::ifstream stream(source.c_str());
    if (!stream)
    {
        std::cerr << "Failed to load shader file: " << source;
        return "";
    }

    std::string result;

    stream.seekg(0, std::ios::end);
    result.reserve(static_cast<unsigned int>(stream.tellg()));
    stream.seekg(0, std::ios::beg);

    result.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    return result;
}

GLuint CompileShader(GLenum type, const std::string &source)
{
    GLuint shader = glCreateShader(type);

    const char *sourceArray[1] = { source.c_str() };
    glShaderSource(shader, 1, sourceArray, nullptr);
    glCompileShader(shader);

    GLint compileResult;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compileResult);

    if (compileResult == 0)
    {
        GLint infoLogLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

        // Info log length includes the null terminator, so 1 means that the info log is an empty
        // string.
        if (infoLogLength > 1)
        {
            std::vector<GLchar> infoLog(infoLogLength);
            glGetShaderInfoLog(shader, static_cast<GLsizei>(infoLog.size()), nullptr, &infoLog[0]);
            std::cerr << "shader compilation failed: " << &infoLog[0];
        }
        else
        {
            std::cerr << "shader compilation failed. <Empty log message>";
        }

        std::cerr << std::endl;

        glDeleteShader(shader);
        shader = 0;
    }

    return shader;
}

GLuint CompileShaderFromFile(GLenum type, const std::string &sourcePath)
{
    std::string source = ReadFileToString(sourcePath);
    if (source.empty())
    {
        return 0;
    }

    return CompileShader(type, source);
}

GLuint CheckLinkStatusAndReturnProgram(GLuint program, bool outputErrorMessages)
{
    if (glGetError() != GL_NO_ERROR)
        return 0;

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == 0)
    {
        if (outputErrorMessages)
        {
            GLint infoLogLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

            // Info log length includes the null terminator, so 1 means that the info log is an
            // empty string.
            if (infoLogLength > 1)
            {
                std::vector<GLchar> infoLog(infoLogLength);
                glGetProgramInfoLog(program, static_cast<GLsizei>(infoLog.size()), nullptr,
                                    &infoLog[0]);

                std::cerr << "program link failed: " << &infoLog[0];
            }
            else
            {
                std::cerr << "program link failed. <Empty log message>";
            }
        }

        glDeleteProgram(program);
        return 0;
    }

    return program;
}

GLuint CompileProgramWithTransformFeedback(
    const std::string &vsSource,
    const std::string &fsSource,
    const std::vector<std::string> &transformFeedbackVaryings,
    GLenum bufferMode)
{
    return CompileProgramWithGSAndTransformFeedback(vsSource, "", fsSource,
                                                    transformFeedbackVaryings, bufferMode);
}

static GLuint CompileAndLinkProgram(const std::string &vsSource,
                                    const std::string &gsSource,
                                    const std::string &fsSource,
                                    const std::vector<std::string> &transformFeedbackVaryings,
                                    GLenum bufferMode)
{
    GLuint program = glCreateProgram();

    GLuint vs = CompileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fsSource);

    if (vs == 0 || fs == 0)
    {
        glDeleteShader(fs);
        glDeleteShader(vs);
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, vs);
    glDeleteShader(vs);

    glAttachShader(program, fs);
    glDeleteShader(fs);

    if (!gsSource.empty())
    {
        GLuint gs = CompileShader(GL_GEOMETRY_SHADER_EXT, gsSource);
        if (gs == 0)
        {
            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(program);
            return 0;
        }

        glAttachShader(program, gs);
        glDeleteShader(gs);
    }

    if (transformFeedbackVaryings.size() > 0)
    {
        std::vector<const char *> constCharTFVaryings;

        for (const std::string &transformFeedbackVarying : transformFeedbackVaryings)
        {
            constCharTFVaryings.push_back(transformFeedbackVarying.c_str());
        }

        glTransformFeedbackVaryings(program, static_cast<GLsizei>(transformFeedbackVaryings.size()),
                                    &constCharTFVaryings[0], bufferMode);
    }

    glLinkProgram(program);

    return program;
}

GLuint CompileProgramWithGSAndTransformFeedback(
    const std::string &vsSource,
    const std::string &gsSource,
    const std::string &fsSource,
    const std::vector<std::string> &transformFeedbackVaryings,
    GLenum bufferMode)
{
    GLuint program =
        CompileAndLinkProgram(vsSource, gsSource, fsSource, transformFeedbackVaryings, bufferMode);
    if (program == 0)
    {
        return 0;
    }
    return CheckLinkStatusAndReturnProgram(program, true);
}

GLuint CompileProgram(const std::string &vsSource, const std::string &fsSource)
{
    return CompileProgramWithGS(vsSource, "", fsSource);
}

GLuint CompileProgramWithGS(const std::string &vsSource,
                            const std::string &gsSource,
                            const std::string &fsSource)
{
    std::vector<std::string> emptyVector;
    return CompileProgramWithGSAndTransformFeedback(vsSource, gsSource, fsSource, emptyVector,
                                                    GL_NONE);
}

GLuint CompileProgramFromFiles(const std::string &vsPath, const std::string &fsPath)
{
    std::string vsSource = ReadFileToString(vsPath);
    std::string fsSource = ReadFileToString(fsPath);
    if (vsSource.empty() || fsSource.empty())
    {
        return 0;
    }

    return CompileProgram(vsSource, fsSource);
}

GLuint CompileComputeProgram(const std::string &csSource, bool outputErrorMessages)
{
    GLuint program = glCreateProgram();

    GLuint cs = CompileShader(GL_COMPUTE_SHADER, csSource);
    if (cs == 0)
    {
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, cs);

    glLinkProgram(program);

    return CheckLinkStatusAndReturnProgram(program, outputErrorMessages);
}

GLuint LoadBinaryProgramOES(const std::vector<uint8_t> &binary, GLenum binaryFormat)
{
    GLuint program = glCreateProgram();
    glProgramBinaryOES(program, binaryFormat, binary.data(), static_cast<GLint>(binary.size()));
    return CheckLinkStatusAndReturnProgram(program, true);
}

GLuint LoadBinaryProgramES3(const std::vector<uint8_t> &binary, GLenum binaryFormat)
{
    GLuint program = glCreateProgram();
    glProgramBinary(program, binaryFormat, binary.data(), static_cast<GLint>(binary.size()));
    return CheckLinkStatusAndReturnProgram(program, true);
}

bool LinkAttachedProgram(GLuint program)
{
    glLinkProgram(program);
    return (CheckLinkStatusAndReturnProgram(program, true) != 0);
}

namespace angle
{

namespace essl1_shaders
{

const char *PositionAttrib()
{
    return "a_position";
}
const char *ColorUniform()
{
    return "u_color";
}

namespace vs
{

// A shader that sets gl_Position to zero.
const char *Zero()
{
    return R"(void main()
{
    gl_Position = vec4(0);
})";
}

// A shader that sets gl_Position to attribute a_position.
const char *Simple()
{
    return R"(precision highp float;
attribute vec4 a_position;

void main()
{
    gl_Position = a_position;
})";
}

// A shader that simply passes through attribute a_position, setting it to gl_Position and varying
// v_position.
const char *Passthrough()
{
    return R"(precision highp float;
attribute vec4 a_position;
varying vec4 v_position;

void main()
{
    gl_Position = a_position;
    v_position = a_position;
})";
}

}  // namespace vs

namespace fs
{

// A shader that renders a simple checker pattern of red and green. X axis and y axis separate the
// different colors. Needs varying v_position.
const char *Checkered()
{
    return R"(precision highp float;
varying vec4 v_position;

void main()
{
    if (v_position.x * v_position.y > 0.0)
    {
        gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
    else
    {
        gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
})";
}

// A shader that fills with color taken from uniform named "color".
const char *UniformColor()
{
    return R"(uniform mediump vec4 u_color;
void main(void)
{
    gl_FragColor = u_color;
})";
}

// A shader that fills with 100% opaque red.
const char *Red()
{
    return R"(precision mediump float;

void main()
{
    gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})";
}

// A shader that fills with 100% opaque blue.
const char *Blue()
{
    return R"(precision mediump float;

void main()
{
    gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
})";
}

}  // namespace fs
}  // namespace essl1_shaders

namespace essl3_shaders
{

const char *PositionAttrib()
{
    return "a_position";
}

namespace vs
{

// A shader that sets gl_Position to zero.
const char *Zero()
{
    return R"(#version 300 es
void main()
{
    gl_Position = vec4(0);
})";
}

// A shader that sets gl_Position to attribute a_position.
const char *Simple()
{
    return R"(#version 300 es
in vec4 a_position;
void main()
{
    gl_Position = a_position;
})";
}

}  // namespace vs

namespace fs
{

// A shader that fills with 100% opaque red.
const char *Red()
{
    return R"(#version 300 es
precision highp float;
out vec4 my_FragColor;
void main()
{
    my_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})";
}

}  // namespace fs
}  // namespace essl3_shaders

namespace essl31_shaders
{

const char *PositionAttrib()
{
    return "a_position";
}

namespace vs
{

// A shader that sets gl_Position to zero.
const char *Zero()
{
    return R"(#version 310 es
void main()
{
    gl_Position = vec4(0);
})";
}

// A shader that sets gl_Position to attribute a_position.
const char *Simple()
{
    return R"(#version 310 es
in vec4 a_position;
void main()
{
    gl_Position = a_position;
})";
}

// A shader that simply passes through attribute a_position, setting it to gl_Position and varying
// v_position.
const char *Passthrough()
{
    return R"(#version 310 es
in vec4 a_position;
out vec4 v_position;
void main()
{
    gl_Position = a_position;
    v_position = a_position;
})";
}

}  // namespace vs

namespace fs
{

// A shader that fills with 100% opaque red.
const char *Red()
{
    return R"(#version 310 es
precision highp float;
out vec4 my_FragColor;
void main()
{
    my_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
})";
}

}  // namespace fs
}  // namespace essl31_shaders
}  // namespace angle
