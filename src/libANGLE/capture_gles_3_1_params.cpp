//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// capture_gles31_params.cpp:
//   Pointer parameter capture functions for the OpenGL ES 3.1 entry points.

#include "libANGLE/capture_gles_3_1_autogen.h"

using namespace angle;

namespace gl
{

void CaptureCreateShaderProgramv_strings(Context *context,
                                         ShaderType typePacked,
                                         GLsizei count,
                                         const GLchar *const *strings,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteProgramPipelines_pipelines(Context *context,
                                             GLsizei n,
                                             const GLuint *pipelines,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawArraysIndirect_indirect(Context *context,
                                        PrimitiveMode modePacked,
                                        const void *indirect,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawElementsIndirect_indirect(Context *context,
                                          PrimitiveMode modePacked,
                                          DrawElementsType typePacked,
                                          const void *indirect,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenProgramPipelines_pipelines(Context *context,
                                          GLsizei n,
                                          GLuint *pipelines,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBooleani_v_data(Context *context,
                               GLenum target,
                               GLuint index,
                               GLboolean *data,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFramebufferParameteriv_params(Context *context,
                                             GLenum target,
                                             GLenum pname,
                                             GLint *params,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetMultisamplefv_val(Context *context,
                                 GLenum pname,
                                 GLuint index,
                                 GLfloat *val,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramInterfaceiv_params(Context *context,
                                         GLuint program,
                                         GLenum programInterface,
                                         GLenum pname,
                                         GLint *params,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramPipelineInfoLog_length(Context *context,
                                             GLuint pipeline,
                                             GLsizei bufSize,
                                             GLsizei *length,
                                             GLchar *infoLog,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramPipelineInfoLog_infoLog(Context *context,
                                              GLuint pipeline,
                                              GLsizei bufSize,
                                              GLsizei *length,
                                              GLchar *infoLog,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramPipelineiv_params(Context *context,
                                        GLuint pipeline,
                                        GLenum pname,
                                        GLint *params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceIndex_name(Context *context,
                                         GLuint program,
                                         GLenum programInterface,
                                         const GLchar *name,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceLocation_name(Context *context,
                                            GLuint program,
                                            GLenum programInterface,
                                            const GLchar *name,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceName_length(Context *context,
                                          GLuint program,
                                          GLenum programInterface,
                                          GLuint index,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLchar *name,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceName_name(Context *context,
                                        GLuint program,
                                        GLenum programInterface,
                                        GLuint index,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLchar *name,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceiv_props(Context *context,
                                       GLuint program,
                                       GLenum programInterface,
                                       GLuint index,
                                       GLsizei propCount,
                                       const GLenum *props,
                                       GLsizei bufSize,
                                       GLsizei *length,
                                       GLint *params,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceiv_length(Context *context,
                                        GLuint program,
                                        GLenum programInterface,
                                        GLuint index,
                                        GLsizei propCount,
                                        const GLenum *props,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLint *params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceiv_params(Context *context,
                                        GLuint program,
                                        GLenum programInterface,
                                        GLuint index,
                                        GLsizei propCount,
                                        const GLenum *props,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLint *params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameterfv_params(Context *context,
                                          TextureTarget targetPacked,
                                          GLint level,
                                          GLenum pname,
                                          GLfloat *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameteriv_params(Context *context,
                                          TextureTarget targetPacked,
                                          GLint level,
                                          GLenum pname,
                                          GLint *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform1fv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLfloat *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform1iv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLint *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform1uiv_value(Context *context,
                                     GLuint program,
                                     GLint location,
                                     GLsizei count,
                                     const GLuint *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform2fv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLfloat *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform2iv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLint *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform2uiv_value(Context *context,
                                     GLuint program,
                                     GLint location,
                                     GLsizei count,
                                     const GLuint *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform3fv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLfloat *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform3iv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLint *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform3uiv_value(Context *context,
                                     GLuint program,
                                     GLint location,
                                     GLsizei count,
                                     const GLuint *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform4fv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLfloat *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform4iv_value(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei count,
                                    const GLint *value,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniform4uiv_value(Context *context,
                                     GLuint program,
                                     GLint location,
                                     GLsizei count,
                                     const GLuint *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix2fv_value(Context *context,
                                          GLuint program,
                                          GLint location,
                                          GLsizei count,
                                          GLboolean transpose,
                                          const GLfloat *value,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix2x3fv_value(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei count,
                                            GLboolean transpose,
                                            const GLfloat *value,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix2x4fv_value(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei count,
                                            GLboolean transpose,
                                            const GLfloat *value,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix3fv_value(Context *context,
                                          GLuint program,
                                          GLint location,
                                          GLsizei count,
                                          GLboolean transpose,
                                          const GLfloat *value,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix3x2fv_value(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei count,
                                            GLboolean transpose,
                                            const GLfloat *value,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix3x4fv_value(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei count,
                                            GLboolean transpose,
                                            const GLfloat *value,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix4fv_value(Context *context,
                                          GLuint program,
                                          GLint location,
                                          GLsizei count,
                                          GLboolean transpose,
                                          const GLfloat *value,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix4x2fv_value(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei count,
                                            GLboolean transpose,
                                            const GLfloat *value,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramUniformMatrix4x3fv_value(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei count,
                                            GLboolean transpose,
                                            const GLfloat *value,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

}  // namespace gl
