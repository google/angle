//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// capture_gles3_params.cpp:
//   Pointer parameter capture functions for the OpenGL ES 3.0 entry points.

#include "libANGLE/capture_gles_3_0_autogen.h"

using namespace angle;

namespace gl
{
void CaptureClearBufferfv_value(Context *context,
                                GLenum buffer,
                                GLint drawbuffer,
                                const GLfloat *value,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureClearBufferiv_value(Context *context,
                                GLenum buffer,
                                GLint drawbuffer,
                                const GLint *value,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureClearBufferuiv_value(Context *context,
                                 GLenum buffer,
                                 GLint drawbuffer,
                                 const GLuint *value,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexImage3D_data(Context *context,
                                      TextureTarget targetPacked,
                                      GLint level,
                                      GLenum internalformat,
                                      GLsizei width,
                                      GLsizei height,
                                      GLsizei depth,
                                      GLint border,
                                      GLsizei imageSize,
                                      const void *data,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexSubImage3D_data(Context *context,
                                         TextureTarget targetPacked,
                                         GLint level,
                                         GLint xoffset,
                                         GLint yoffset,
                                         GLint zoffset,
                                         GLsizei width,
                                         GLsizei height,
                                         GLsizei depth,
                                         GLenum format,
                                         GLsizei imageSize,
                                         const void *data,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteQueries_ids(Context *context,
                              GLsizei n,
                              const GLuint *ids,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteSamplers_samplers(Context *context,
                                    GLsizei count,
                                    const GLuint *samplers,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteTransformFeedbacks_ids(Context *context,
                                         GLsizei n,
                                         const GLuint *ids,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteVertexArrays_arrays(Context *context,
                                      GLsizei n,
                                      const GLuint *arrays,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawBuffers_bufs(Context *context,
                             GLsizei n,
                             const GLenum *bufs,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawElementsInstanced_indices(Context *context,
                                          PrimitiveMode modePacked,
                                          GLsizei count,
                                          DrawElementsType typePacked,
                                          const void *indices,
                                          GLsizei instancecount,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawRangeElements_indices(Context *context,
                                      PrimitiveMode modePacked,
                                      GLuint start,
                                      GLuint end,
                                      GLsizei count,
                                      DrawElementsType typePacked,
                                      const void *indices,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenQueries_ids(Context *context,
                           GLsizei n,
                           GLuint *ids,
                           bool isCallValid,
                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenSamplers_samplers(Context *context,
                                 GLsizei count,
                                 GLuint *samplers,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenTransformFeedbacks_ids(Context *context,
                                      GLsizei n,
                                      GLuint *ids,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenVertexArrays_arrays(Context *context,
                                   GLsizei n,
                                   GLuint *arrays,
                                   bool isCallValid,
                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniformBlockName_length(Context *context,
                                             GLuint program,
                                             GLuint uniformBlockIndex,
                                             GLsizei bufSize,
                                             GLsizei *length,
                                             GLchar *uniformBlockName,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniformBlockName_uniformBlockName(Context *context,
                                                       GLuint program,
                                                       GLuint uniformBlockIndex,
                                                       GLsizei bufSize,
                                                       GLsizei *length,
                                                       GLchar *uniformBlockName,
                                                       bool isCallValid,
                                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniformBlockiv_params(Context *context,
                                           GLuint program,
                                           GLuint uniformBlockIndex,
                                           GLenum pname,
                                           GLint *params,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniformsiv_uniformIndices(Context *context,
                                               GLuint program,
                                               GLsizei uniformCount,
                                               const GLuint *uniformIndices,
                                               GLenum pname,
                                               GLint *params,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniformsiv_params(Context *context,
                                       GLuint program,
                                       GLsizei uniformCount,
                                       const GLuint *uniformIndices,
                                       GLenum pname,
                                       GLint *params,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferParameteri64v_params(Context *context,
                                          BufferBinding targetPacked,
                                          GLenum pname,
                                          GLint64 *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferPointerv_params(Context *context,
                                     BufferBinding targetPacked,
                                     GLenum pname,
                                     void **params,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFragDataLocation_name(Context *context,
                                     GLuint program,
                                     const GLchar *name,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInteger64i_v_data(Context *context,
                                 GLenum target,
                                 GLuint index,
                                 GLint64 *data,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInteger64v_data(Context *context,
                               GLenum pname,
                               GLint64 *data,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetIntegeri_v_data(Context *context,
                               GLenum target,
                               GLuint index,
                               GLint *data,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInternalformativ_params(Context *context,
                                       GLenum target,
                                       GLenum internalformat,
                                       GLenum pname,
                                       GLsizei bufSize,
                                       GLint *params,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramBinary_length(Context *context,
                                    GLuint program,
                                    GLsizei bufSize,
                                    GLsizei *length,
                                    GLenum *binaryFormat,
                                    void *binary,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramBinary_binaryFormat(Context *context,
                                          GLuint program,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLenum *binaryFormat,
                                          void *binary,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramBinary_binary(Context *context,
                                    GLuint program,
                                    GLsizei bufSize,
                                    GLsizei *length,
                                    GLenum *binaryFormat,
                                    void *binary,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectuiv_params(Context *context,
                                     GLuint id,
                                     GLenum pname,
                                     GLuint *params,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryiv_params(Context *context,
                              QueryType targetPacked,
                              GLenum pname,
                              GLint *params,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterfv_params(Context *context,
                                         GLuint sampler,
                                         GLenum pname,
                                         GLfloat *params,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameteriv_params(Context *context,
                                         GLuint sampler,
                                         GLenum pname,
                                         GLint *params,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSynciv_length(Context *context,
                             GLsync sync,
                             GLenum pname,
                             GLsizei bufSize,
                             GLsizei *length,
                             GLint *values,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSynciv_values(Context *context,
                             GLsync sync,
                             GLenum pname,
                             GLsizei bufSize,
                             GLsizei *length,
                             GLint *values,
                             bool isCallValid,
                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTransformFeedbackVarying_length(Context *context,
                                               GLuint program,
                                               GLuint index,
                                               GLsizei bufSize,
                                               GLsizei *length,
                                               GLsizei *size,
                                               GLenum *type,
                                               GLchar *name,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTransformFeedbackVarying_size(Context *context,
                                             GLuint program,
                                             GLuint index,
                                             GLsizei bufSize,
                                             GLsizei *length,
                                             GLsizei *size,
                                             GLenum *type,
                                             GLchar *name,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTransformFeedbackVarying_type(Context *context,
                                             GLuint program,
                                             GLuint index,
                                             GLsizei bufSize,
                                             GLsizei *length,
                                             GLsizei *size,
                                             GLenum *type,
                                             GLchar *name,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTransformFeedbackVarying_name(Context *context,
                                             GLuint program,
                                             GLuint index,
                                             GLsizei bufSize,
                                             GLsizei *length,
                                             GLsizei *size,
                                             GLenum *type,
                                             GLchar *name,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformBlockIndex_uniformBlockName(Context *context,
                                                  GLuint program,
                                                  const GLchar *uniformBlockName,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformIndices_uniformNames(Context *context,
                                           GLuint program,
                                           GLsizei uniformCount,
                                           const GLchar *const *uniformNames,
                                           GLuint *uniformIndices,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformIndices_uniformIndices(Context *context,
                                             GLuint program,
                                             GLsizei uniformCount,
                                             const GLchar *const *uniformNames,
                                             GLuint *uniformIndices,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformuiv_params(Context *context,
                                 GLuint program,
                                 GLint location,
                                 GLuint *params,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribIiv_params(Context *context,
                                      GLuint index,
                                      GLenum pname,
                                      GLint *params,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribIuiv_params(Context *context,
                                       GLuint index,
                                       GLenum pname,
                                       GLuint *params,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureInvalidateFramebuffer_attachments(Context *context,
                                              GLenum target,
                                              GLsizei numAttachments,
                                              const GLenum *attachments,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureInvalidateSubFramebuffer_attachments(Context *context,
                                                 GLenum target,
                                                 GLsizei numAttachments,
                                                 const GLenum *attachments,
                                                 GLint x,
                                                 GLint y,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramBinary_binary(Context *context,
                                 GLuint program,
                                 GLenum binaryFormat,
                                 const void *binary,
                                 GLsizei length,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameterfv_param(Context *context,
                                     GLuint sampler,
                                     GLenum pname,
                                     const GLfloat *param,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameteriv_param(Context *context,
                                     GLuint sampler,
                                     GLenum pname,
                                     const GLint *param,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexImage3D_pixels(Context *context,
                              TextureTarget targetPacked,
                              GLint level,
                              GLint internalformat,
                              GLsizei width,
                              GLsizei height,
                              GLsizei depth,
                              GLint border,
                              GLenum format,
                              GLenum type,
                              const void *pixels,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexSubImage3D_pixels(Context *context,
                                 TextureTarget targetPacked,
                                 GLint level,
                                 GLint xoffset,
                                 GLint yoffset,
                                 GLint zoffset,
                                 GLsizei width,
                                 GLsizei height,
                                 GLsizei depth,
                                 GLenum format,
                                 GLenum type,
                                 const void *pixels,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTransformFeedbackVaryings_varyings(Context *context,
                                               GLuint program,
                                               GLsizei count,
                                               const GLchar *const *varyings,
                                               GLenum bufferMode,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniform1uiv_value(Context *context,
                              GLint location,
                              GLsizei count,
                              const GLuint *value,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniform2uiv_value(Context *context,
                              GLint location,
                              GLsizei count,
                              const GLuint *value,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniform3uiv_value(Context *context,
                              GLint location,
                              GLsizei count,
                              const GLuint *value,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniform4uiv_value(Context *context,
                              GLint location,
                              GLsizei count,
                              const GLuint *value,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniformMatrix2x3fv_value(Context *context,
                                     GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniformMatrix2x4fv_value(Context *context,
                                     GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniformMatrix3x2fv_value(Context *context,
                                     GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniformMatrix3x4fv_value(Context *context,
                                     GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniformMatrix4x2fv_value(Context *context,
                                     GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureUniformMatrix4x3fv_value(Context *context,
                                     GLint location,
                                     GLsizei count,
                                     GLboolean transpose,
                                     const GLfloat *value,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexAttribI4iv_v(Context *context,
                               GLuint index,
                               const GLint *v,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexAttribI4uiv_v(Context *context,
                                GLuint index,
                                const GLuint *v,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureVertexAttribIPointer_pointer(Context *context,
                                         GLuint index,
                                         GLint size,
                                         VertexAttribType typePacked,
                                         GLsizei stride,
                                         const void *pointer,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

}  // namespace gl
