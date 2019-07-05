//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// capture_gles2_params.cpp:
//   Pointer parameter capture functions for the OpenGL ES 2.0 entry points.

#include "libANGLE/capture_gles_2_0_autogen.h"

#include "libANGLE/Context.h"
#include "libANGLE/FrameCapture.h"
#include "libANGLE/Shader.h"
#include "libANGLE/formatutils.h"

using namespace angle;

namespace gl
{
// Parameter Captures

void CaptureBindAttribLocation_name(Context *context,
                                    GLuint program,
                                    GLuint index,
                                    const GLchar *name,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    CaptureString(name, paramCapture);
}

void CaptureBufferData_data(Context *context,
                            BufferBinding targetPacked,
                            GLsizeiptr size,
                            const void *data,
                            BufferUsage usagePacked,
                            bool isCallValid,
                            ParamCapture *paramCapture)
{
    if (data)
    {
        CaptureMemory(data, size, paramCapture);
    }
}

void CaptureBufferSubData_data(Context *context,
                               BufferBinding targetPacked,
                               GLintptr offset,
                               GLsizeiptr size,
                               const void *data,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    CaptureMemory(data, size, paramCapture);
}

void CaptureCompressedTexImage2D_data(Context *context,
                                      TextureTarget targetPacked,
                                      GLint level,
                                      GLenum internalformat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLint border,
                                      GLsizei imageSize,
                                      const void *data,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    if (context->getState().getTargetBuffer(gl::BufferBinding::PixelUnpack))
    {
        return;
    }

    if (!data)
    {
        return;
    }

    CaptureMemory(data, imageSize, paramCapture);
}

void CaptureCompressedTexSubImage2D_data(Context *context,
                                         TextureTarget targetPacked,
                                         GLint level,
                                         GLint xoffset,
                                         GLint yoffset,
                                         GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLsizei imageSize,
                                         const void *data,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteBuffers_buffers(Context *context,
                                  GLsizei n,
                                  const GLuint *buffers,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteFramebuffers_framebuffers(Context *context,
                                            GLsizei n,
                                            const GLuint *framebuffers,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteRenderbuffers_renderbuffers(Context *context,
                                              GLsizei n,
                                              const GLuint *renderbuffers,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteTextures_textures(Context *context,
                                    GLsizei n,
                                    const GLuint *textures,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    CaptureMemory(textures, sizeof(GLuint) * n, paramCapture);
}

void CaptureDrawElements_indices(Context *context,
                                 PrimitiveMode modePacked,
                                 GLsizei count,
                                 DrawElementsType typePacked,
                                 const void *indices,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    if (context->getState().getVertexArray()->getElementArrayBuffer())
    {
        paramCapture->value.voidConstPointerVal = indices;
    }
    else
    {
        GLuint typeSize = gl::GetDrawElementsTypeSize(typePacked);
        CaptureMemory(indices, typeSize * count, paramCapture);
        paramCapture->value.voidConstPointerVal = paramCapture->data[0].data();
    }
}

void CaptureGenBuffers_buffers(Context *context,
                               GLsizei n,
                               GLuint *buffers,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    paramCapture->readBufferSize = sizeof(GLuint) * n;
}

void CaptureGenFramebuffers_framebuffers(Context *context,
                                         GLsizei n,
                                         GLuint *framebuffers,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    paramCapture->readBufferSize = sizeof(GLuint) * n;
}

void CaptureGenRenderbuffers_renderbuffers(Context *context,
                                           GLsizei n,
                                           GLuint *renderbuffers,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    paramCapture->readBufferSize = sizeof(GLuint) * n;
}

void CaptureGenTextures_textures(Context *context,
                                 GLsizei n,
                                 GLuint *textures,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    paramCapture->readBufferSize = sizeof(GLuint) * n;
}

void CaptureGetActiveAttrib_length(Context *context,
                                   GLuint program,
                                   GLuint index,
                                   GLsizei bufSize,
                                   GLsizei *length,
                                   GLint *size,
                                   GLenum *type,
                                   GLchar *name,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveAttrib_size(Context *context,
                                 GLuint program,
                                 GLuint index,
                                 GLsizei bufSize,
                                 GLsizei *length,
                                 GLint *size,
                                 GLenum *type,
                                 GLchar *name,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveAttrib_type(Context *context,
                                 GLuint program,
                                 GLuint index,
                                 GLsizei bufSize,
                                 GLsizei *length,
                                 GLint *size,
                                 GLenum *type,
                                 GLchar *name,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveAttrib_name(Context *context,
                                 GLuint program,
                                 GLuint index,
                                 GLsizei bufSize,
                                 GLsizei *length,
                                 GLint *size,
                                 GLenum *type,
                                 GLchar *name,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniform_length(Context *context,
                                    GLuint program,
                                    GLuint index,
                                    GLsizei bufSize,
                                    GLsizei *length,
                                    GLint *size,
                                    GLenum *type,
                                    GLchar *name,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniform_size(Context *context,
                                  GLuint program,
                                  GLuint index,
                                  GLsizei bufSize,
                                  GLsizei *length,
                                  GLint *size,
                                  GLenum *type,
                                  GLchar *name,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniform_type(Context *context,
                                  GLuint program,
                                  GLuint index,
                                  GLsizei bufSize,
                                  GLsizei *length,
                                  GLint *size,
                                  GLenum *type,
                                  GLchar *name,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniform_name(Context *context,
                                  GLuint program,
                                  GLuint index,
                                  GLsizei bufSize,
                                  GLsizei *length,
                                  GLint *size,
                                  GLenum *type,
                                  GLchar *name,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetAttachedShaders_count(Context *context,
                                     GLuint program,
                                     GLsizei maxCount,
                                     GLsizei *count,
                                     GLuint *shaders,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetAttachedShaders_shaders(Context *context,
                                       GLuint program,
                                       GLsizei maxCount,
                                       GLsizei *count,
                                       GLuint *shaders,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetAttribLocation_name(Context *context,
                                   GLuint program,
                                   const GLchar *name,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    CaptureString(name, paramCapture);
}

void CaptureGetBooleanv_data(Context *context,
                             GLenum pname,
                             GLboolean *data,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    GLenum type;
    unsigned int numParams;
    context->getQueryParameterInfo(pname, &type, &numParams);
    paramCapture->readBufferSize = sizeof(GLboolean) * numParams;
}

void CaptureGetBufferParameteriv_params(Context *context,
                                        BufferBinding targetPacked,
                                        GLenum pname,
                                        GLint *params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFloatv_data(Context *context,
                           GLenum pname,
                           GLfloat *data,
                           bool isCallValid,
                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFramebufferAttachmentParameteriv_params(Context *context,
                                                       GLenum target,
                                                       GLenum attachment,
                                                       GLenum pname,
                                                       GLint *params,
                                                       bool isCallValid,
                                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetIntegerv_data(Context *context,
                             GLenum pname,
                             GLint *data,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    GLenum type;
    unsigned int numParams;
    context->getQueryParameterInfo(pname, &type, &numParams);
    paramCapture->readBufferSize = sizeof(GLint) * numParams;
}

void CaptureGetProgramInfoLog_length(Context *context,
                                     GLuint program,
                                     GLsizei bufSize,
                                     GLsizei *length,
                                     GLchar *infoLog,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    paramCapture->readBufferSize = sizeof(GLsizei);
}

void CaptureGetProgramInfoLog_infoLog(Context *context,
                                      GLuint program,
                                      GLsizei bufSize,
                                      GLsizei *length,
                                      GLchar *infoLog,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    gl::Program *programObj = context->getProgramResolveLink(program);
    ASSERT(programObj);
    paramCapture->readBufferSize = programObj->getInfoLogLength() + 1;
}

void CaptureGetProgramiv_params(Context *context,
                                GLuint program,
                                GLenum pname,
                                GLint *params,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    if (params)
    {
        paramCapture->readBufferSize = sizeof(GLint);
    }
}

void CaptureGetRenderbufferParameteriv_params(Context *context,
                                              GLenum target,
                                              GLenum pname,
                                              GLint *params,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetShaderInfoLog_length(Context *context,
                                    GLuint shader,
                                    GLsizei bufSize,
                                    GLsizei *length,
                                    GLchar *infoLog,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    paramCapture->readBufferSize = sizeof(GLsizei);
}

void CaptureGetShaderInfoLog_infoLog(Context *context,
                                     GLuint shader,
                                     GLsizei bufSize,
                                     GLsizei *length,
                                     GLchar *infoLog,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    gl::Shader *shaderObj = context->getShader(shader);
    ASSERT(shaderObj);
    paramCapture->readBufferSize = shaderObj->getInfoLogLength() + 1;
}

void CaptureGetShaderPrecisionFormat_range(Context *context,
                                           GLenum shadertype,
                                           GLenum precisiontype,
                                           GLint *range,
                                           GLint *precision,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetShaderPrecisionFormat_precision(Context *context,
                                               GLenum shadertype,
                                               GLenum precisiontype,
                                               GLint *range,
                                               GLint *precision,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetShaderSource_length(Context *context,
                                   GLuint shader,
                                   GLsizei bufSize,
                                   GLsizei *length,
                                   GLchar *source,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetShaderSource_source(Context *context,
                                   GLuint shader,
                                   GLsizei bufSize,
                                   GLsizei *length,
                                   GLchar *source,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetShaderiv_params(Context *context,
                               GLuint shader,
                               GLenum pname,
                               GLint *params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    if (params)
    {
        paramCapture->readBufferSize = sizeof(GLint);
    }
}

void CaptureGetTexParameterfv_params(Context *context,
                                     TextureType targetPacked,
                                     GLenum pname,
                                     GLfloat *params,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameteriv_params(Context *context,
                                     TextureType targetPacked,
                                     GLenum pname,
                                     GLint *params,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformLocation_name(Context *context,
                                    GLuint program,
                                    const GLchar *name,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    CaptureString(name, paramCapture);
}

void CaptureGetUniformfv_params(Context *context,
                                GLuint program,
                                GLint location,
                                GLfloat *params,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformiv_params(Context *context,
                                GLuint program,
                                GLint location,
                                GLint *params,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribPointerv_pointer(Context *context,
                                            GLuint index,
                                            GLenum pname,
                                            void **pointer,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    paramCapture->readBufferSize = sizeof(void *);
}

void CaptureGetVertexAttribfv_params(Context *context,
                                     GLuint index,
                                     GLenum pname,
                                     GLfloat *params,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    // Can be up to 4 current state values.
    paramCapture->readBufferSize = sizeof(GLfloat) * 4;
}

void CaptureGetVertexAttribiv_params(Context *context,
                                     GLuint index,
                                     GLenum pname,
                                     GLint *params,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    // Can be up to 4 current state values.
    paramCapture->readBufferSize = sizeof(GLint) * 4;
}

void CaptureReadPixels_pixels(Context *context,
                              GLint x,
                              GLint y,
                              GLsizei width,
                              GLsizei height,
                              GLenum format,
                              GLenum type,
                              void *pixels,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureShaderBinary_shaders(Context *context,
                                 GLsizei count,
                                 const GLuint *shaders,
                                 GLenum binaryformat,
                                 const void *binary,
                                 GLsizei length,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureShaderBinary_binary(Context *context,
                                GLsizei count,
                                const GLuint *shaders,
                                GLenum binaryformat,
                                const void *binary,
                                GLsizei length,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureShaderSource_string(Context *context,
                                GLuint shader,
                                GLsizei count,
                                const GLchar *const *string,
                                const GLint *length,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    for (GLsizei index = 0; index < count; ++index)
    {
        size_t len = length ? length[index] : strlen(string[index]);
        CaptureMemory(string[index], len, paramCapture);
    }
}

void CaptureShaderSource_length(Context *context,
                                GLuint shader,
                                GLsizei count,
                                const GLchar *const *string,
                                const GLint *length,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    if (!length)
        return;

    for (GLsizei index = 0; index < count; ++index)
    {
        CaptureMemory(&length[index], sizeof(GLint), paramCapture);
    }
}

void CaptureTexImage2D_pixels(Context *context,
                              TextureTarget targetPacked,
                              GLint level,
                              GLint internalformat,
                              GLsizei width,
                              GLsizei height,
                              GLint border,
                              GLenum format,
                              GLenum type,
                              const void *pixels,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    if (context->getState().getTargetBuffer(gl::BufferBinding::PixelUnpack))
    {
        return;
    }

    if (!pixels)
    {
        return;
    }

    const gl::InternalFormat &internalFormatInfo = gl::GetInternalFormatInfo(format, type);
    const gl::PixelUnpackState &unpack           = context->getState().getUnpackState();

    GLuint srcRowPitch = 0;
    (void)internalFormatInfo.computeRowPitch(type, width, unpack.alignment, unpack.rowLength,
                                             &srcRowPitch);
    GLuint srcDepthPitch = 0;
    (void)internalFormatInfo.computeDepthPitch(height, unpack.imageHeight, srcRowPitch,
                                               &srcDepthPitch);
    GLuint srcSkipBytes = 0;
    (void)internalFormatInfo.computeSkipBytes(type, srcRowPitch, srcDepthPitch, unpack, false,
                                              &srcSkipBytes);

    size_t captureSize = srcRowPitch * height + srcSkipBytes;
    CaptureMemory(pixels, captureSize, paramCapture);
}

void CaptureTexParameterfv_params(Context *context,
                                  TextureType targetPacked,
                                  GLenum pname,
                                  const GLfloat *params,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameteriv_params(Context *context,
                                  TextureType targetPacked,
                                  GLenum pname,
                                  const GLint *params,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexSubImage2D_pixels(Context *context,
                                 TextureTarget targetPacked,
                                 GLint level,
                                 GLint xoffset,
                                 GLint yoffset,
                                 GLsizei width,
                                 GLsizei height,
                                 GLenum format,
                                 GLenum type,
                                 const void *pixels,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniform1fv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLfloat), paramCapture);
}

void CaptureUniform1iv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLint *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLint), paramCapture);
}

void CaptureUniform2fv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLfloat) * 2, paramCapture);
}

void CaptureUniform2iv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLint *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLint) * 2, paramCapture);
}

void CaptureUniform3fv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLfloat) * 3, paramCapture);
}

void CaptureUniform3iv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLint *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLint) * 3, paramCapture);
}

void CaptureUniform4fv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLfloat *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLfloat) * 4, paramCapture);
}

void CaptureUniform4iv_value(Context *context,
                             GLint location,
                             GLsizei count,
                             const GLint *value,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLint) * 4, paramCapture);
}

void CaptureUniformMatrix2fv_value(Context *context,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLfloat) * 4, paramCapture);
}

void CaptureUniformMatrix3fv_value(Context *context,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLfloat) * 9, paramCapture);
}

void CaptureUniformMatrix4fv_value(Context *context,
                                   GLint location,
                                   GLsizei count,
                                   GLboolean transpose,
                                   const GLfloat *value,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    CaptureMemory(value, count * sizeof(GLfloat) * 16, paramCapture);
}

void CaptureVertexAttrib1fv_v(Context *context,
                              GLuint index,
                              const GLfloat *v,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexAttrib2fv_v(Context *context,
                              GLuint index,
                              const GLfloat *v,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexAttrib3fv_v(Context *context,
                              GLuint index,
                              const GLfloat *v,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexAttrib4fv_v(Context *context,
                              GLuint index,
                              const GLfloat *v,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexAttribPointer_pointer(Context *context,
                                        GLuint index,
                                        GLint size,
                                        VertexAttribType typePacked,
                                        GLboolean normalized,
                                        GLsizei stride,
                                        const void *pointer,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    paramCapture->value.voidConstPointerVal = pointer;
    if (!context->getState().getTargetBuffer(gl::BufferBinding::Array))
    {
        paramCapture->arrayClientPointerIndex = index;
    }
}
}  // namespace gl
