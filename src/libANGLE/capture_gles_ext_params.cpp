//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// capture_glesext_params.cpp:
//   Pointer parameter capture functions for the OpenGL ES extension entry points.

#include "libANGLE/capture_gles_ext_autogen.h"

#include "libANGLE/FrameCapture.h"

using namespace angle;

namespace gl
{

void CaptureDrawElementsInstancedANGLE_indices(Context *context,
                                               PrimitiveMode modePacked,
                                               GLsizei count,
                                               DrawElementsType typePacked,
                                               const void *indices,
                                               GLsizei primcount,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawArraysANGLE_firsts(Context *context,
                                        PrimitiveMode modePacked,
                                        const GLint *firsts,
                                        const GLsizei *counts,
                                        GLsizei drawcount,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawArraysANGLE_counts(Context *context,
                                        PrimitiveMode modePacked,
                                        const GLint *firsts,
                                        const GLsizei *counts,
                                        GLsizei drawcount,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawArraysInstancedANGLE_firsts(Context *context,
                                                 PrimitiveMode modePacked,
                                                 const GLint *firsts,
                                                 const GLsizei *counts,
                                                 const GLsizei *instanceCounts,
                                                 GLsizei drawcount,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawArraysInstancedANGLE_counts(Context *context,
                                                 PrimitiveMode modePacked,
                                                 const GLint *firsts,
                                                 const GLsizei *counts,
                                                 const GLsizei *instanceCounts,
                                                 GLsizei drawcount,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawArraysInstancedANGLE_instanceCounts(Context *context,
                                                         PrimitiveMode modePacked,
                                                         const GLint *firsts,
                                                         const GLsizei *counts,
                                                         const GLsizei *instanceCounts,
                                                         GLsizei drawcount,
                                                         bool isCallValid,
                                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawElementsANGLE_counts(Context *context,
                                          PrimitiveMode modePacked,
                                          const GLsizei *counts,
                                          DrawElementsType typePacked,
                                          const GLvoid *const *indices,
                                          GLsizei drawcount,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawElementsANGLE_indices(Context *context,
                                           PrimitiveMode modePacked,
                                           const GLsizei *counts,
                                           DrawElementsType typePacked,
                                           const GLvoid *const *indices,
                                           GLsizei drawcount,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawElementsInstancedANGLE_counts(Context *context,
                                                   PrimitiveMode modePacked,
                                                   const GLsizei *counts,
                                                   DrawElementsType typePacked,
                                                   const GLvoid *const *indices,
                                                   const GLsizei *instanceCounts,
                                                   GLsizei drawcount,
                                                   bool isCallValid,
                                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawElementsInstancedANGLE_indices(Context *context,
                                                    PrimitiveMode modePacked,
                                                    const GLsizei *counts,
                                                    DrawElementsType typePacked,
                                                    const GLvoid *const *indices,
                                                    const GLsizei *instanceCounts,
                                                    GLsizei drawcount,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMultiDrawElementsInstancedANGLE_instanceCounts(Context *context,
                                                           PrimitiveMode modePacked,
                                                           const GLsizei *counts,
                                                           DrawElementsType typePacked,
                                                           const GLvoid *const *indices,
                                                           const GLsizei *instanceCounts,
                                                           GLsizei drawcount,
                                                           bool isCallValid,
                                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureRequestExtensionANGLE_name(Context *context,
                                       const GLchar *name,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBooleanvRobustANGLE_length(Context *context,
                                          GLenum pname,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLboolean *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBooleanvRobustANGLE_params(Context *context,
                                          GLenum pname,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLboolean *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferParameterivRobustANGLE_length(Context *context,
                                                   BufferBinding targetPacked,
                                                   GLenum pname,
                                                   GLsizei bufSize,
                                                   GLsizei *length,
                                                   GLint *params,
                                                   bool isCallValid,
                                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferParameterivRobustANGLE_params(Context *context,
                                                   BufferBinding targetPacked,
                                                   GLenum pname,
                                                   GLsizei bufSize,
                                                   GLsizei *length,
                                                   GLint *params,
                                                   bool isCallValid,
                                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFloatvRobustANGLE_length(Context *context,
                                        GLenum pname,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLfloat *params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFloatvRobustANGLE_params(Context *context,
                                        GLenum pname,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLfloat *params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFramebufferAttachmentParameterivRobustANGLE_length(Context *context,
                                                                  GLenum target,
                                                                  GLenum attachment,
                                                                  GLenum pname,
                                                                  GLsizei bufSize,
                                                                  GLsizei *length,
                                                                  GLint *params,
                                                                  bool isCallValid,
                                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFramebufferAttachmentParameterivRobustANGLE_params(Context *context,
                                                                  GLenum target,
                                                                  GLenum attachment,
                                                                  GLenum pname,
                                                                  GLsizei bufSize,
                                                                  GLsizei *length,
                                                                  GLint *params,
                                                                  bool isCallValid,
                                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetIntegervRobustANGLE_length(Context *context,
                                          GLenum pname,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLint *data,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetIntegervRobustANGLE_data(Context *context,
                                        GLenum pname,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLint *data,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramivRobustANGLE_length(Context *context,
                                           GLuint program,
                                           GLenum pname,
                                           GLsizei bufSize,
                                           GLsizei *length,
                                           GLint *params,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramivRobustANGLE_params(Context *context,
                                           GLuint program,
                                           GLenum pname,
                                           GLsizei bufSize,
                                           GLsizei *length,
                                           GLint *params,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetRenderbufferParameterivRobustANGLE_length(Context *context,
                                                         GLenum target,
                                                         GLenum pname,
                                                         GLsizei bufSize,
                                                         GLsizei *length,
                                                         GLint *params,
                                                         bool isCallValid,
                                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetRenderbufferParameterivRobustANGLE_params(Context *context,
                                                         GLenum target,
                                                         GLenum pname,
                                                         GLsizei bufSize,
                                                         GLsizei *length,
                                                         GLint *params,
                                                         bool isCallValid,
                                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetShaderivRobustANGLE_length(Context *context,
                                          GLuint shader,
                                          GLenum pname,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLint *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetShaderivRobustANGLE_params(Context *context,
                                          GLuint shader,
                                          GLenum pname,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLint *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterfvRobustANGLE_length(Context *context,
                                                TextureType targetPacked,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLfloat *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterfvRobustANGLE_params(Context *context,
                                                TextureType targetPacked,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLfloat *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterivRobustANGLE_length(Context *context,
                                                TextureType targetPacked,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLint *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterivRobustANGLE_params(Context *context,
                                                TextureType targetPacked,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLint *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformfvRobustANGLE_length(Context *context,
                                           GLuint program,
                                           GLint location,
                                           GLsizei bufSize,
                                           GLsizei *length,
                                           GLfloat *params,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformfvRobustANGLE_params(Context *context,
                                           GLuint program,
                                           GLint location,
                                           GLsizei bufSize,
                                           GLsizei *length,
                                           GLfloat *params,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformivRobustANGLE_length(Context *context,
                                           GLuint program,
                                           GLint location,
                                           GLsizei bufSize,
                                           GLsizei *length,
                                           GLint *params,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformivRobustANGLE_params(Context *context,
                                           GLuint program,
                                           GLint location,
                                           GLsizei bufSize,
                                           GLsizei *length,
                                           GLint *params,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribfvRobustANGLE_length(Context *context,
                                                GLuint index,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLfloat *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribfvRobustANGLE_params(Context *context,
                                                GLuint index,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLfloat *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribivRobustANGLE_length(Context *context,
                                                GLuint index,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLint *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribivRobustANGLE_params(Context *context,
                                                GLuint index,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLint *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribPointervRobustANGLE_length(Context *context,
                                                      GLuint index,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei *length,
                                                      void **pointer,
                                                      bool isCallValid,
                                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribPointervRobustANGLE_pointer(Context *context,
                                                       GLuint index,
                                                       GLenum pname,
                                                       GLsizei bufSize,
                                                       GLsizei *length,
                                                       void **pointer,
                                                       bool isCallValid,
                                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadPixelsRobustANGLE_length(Context *context,
                                         GLint x,
                                         GLint y,
                                         GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLenum type,
                                         GLsizei bufSize,
                                         GLsizei *length,
                                         GLsizei *columns,
                                         GLsizei *rows,
                                         void *pixels,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadPixelsRobustANGLE_columns(Context *context,
                                          GLint x,
                                          GLint y,
                                          GLsizei width,
                                          GLsizei height,
                                          GLenum format,
                                          GLenum type,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLsizei *columns,
                                          GLsizei *rows,
                                          void *pixels,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadPixelsRobustANGLE_rows(Context *context,
                                       GLint x,
                                       GLint y,
                                       GLsizei width,
                                       GLsizei height,
                                       GLenum format,
                                       GLenum type,
                                       GLsizei bufSize,
                                       GLsizei *length,
                                       GLsizei *columns,
                                       GLsizei *rows,
                                       void *pixels,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadPixelsRobustANGLE_pixels(Context *context,
                                         GLint x,
                                         GLint y,
                                         GLsizei width,
                                         GLsizei height,
                                         GLenum format,
                                         GLenum type,
                                         GLsizei bufSize,
                                         GLsizei *length,
                                         GLsizei *columns,
                                         GLsizei *rows,
                                         void *pixels,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexImage2DRobustANGLE_pixels(Context *context,
                                         TextureTarget targetPacked,
                                         GLint level,
                                         GLint internalformat,
                                         GLsizei width,
                                         GLsizei height,
                                         GLint border,
                                         GLenum format,
                                         GLenum type,
                                         GLsizei bufSize,
                                         const void *pixels,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameterfvRobustANGLE_params(Context *context,
                                             TextureType targetPacked,
                                             GLenum pname,
                                             GLsizei bufSize,
                                             const GLfloat *params,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameterivRobustANGLE_params(Context *context,
                                             TextureType targetPacked,
                                             GLenum pname,
                                             GLsizei bufSize,
                                             const GLint *params,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexSubImage2DRobustANGLE_pixels(Context *context,
                                            TextureTarget targetPacked,
                                            GLint level,
                                            GLint xoffset,
                                            GLint yoffset,
                                            GLsizei width,
                                            GLsizei height,
                                            GLenum format,
                                            GLenum type,
                                            GLsizei bufSize,
                                            const void *pixels,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexImage3DRobustANGLE_pixels(Context *context,
                                         TextureTarget targetPacked,
                                         GLint level,
                                         GLint internalformat,
                                         GLsizei width,
                                         GLsizei height,
                                         GLsizei depth,
                                         GLint border,
                                         GLenum format,
                                         GLenum type,
                                         GLsizei bufSize,
                                         const void *pixels,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexSubImage3DRobustANGLE_pixels(Context *context,
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
                                            GLsizei bufSize,
                                            const void *pixels,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexImage2DRobustANGLE_data(Context *context,
                                                 TextureTarget targetPacked,
                                                 GLint level,
                                                 GLenum internalformat,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLint border,
                                                 GLsizei imageSize,
                                                 GLsizei dataSize,
                                                 const GLvoid *data,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexSubImage2DRobustANGLE_data(Context *context,
                                                    TextureTarget targetPacked,
                                                    GLint level,
                                                    GLsizei xoffset,
                                                    GLsizei yoffset,
                                                    GLsizei width,
                                                    GLsizei height,
                                                    GLenum format,
                                                    GLsizei imageSize,
                                                    GLsizei dataSize,
                                                    const GLvoid *data,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexImage3DRobustANGLE_data(Context *context,
                                                 TextureTarget targetPacked,
                                                 GLint level,
                                                 GLenum internalformat,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLsizei depth,
                                                 GLint border,
                                                 GLsizei imageSize,
                                                 GLsizei dataSize,
                                                 const GLvoid *data,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexSubImage3DRobustANGLE_data(Context *context,
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
                                                    GLsizei dataSize,
                                                    const GLvoid *data,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryivRobustANGLE_length(Context *context,
                                         QueryType targetPacked,
                                         GLenum pname,
                                         GLsizei bufSize,
                                         GLsizei *length,
                                         GLint *params,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryivRobustANGLE_params(Context *context,
                                         QueryType targetPacked,
                                         GLenum pname,
                                         GLsizei bufSize,
                                         GLsizei *length,
                                         GLint *params,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectuivRobustANGLE_length(Context *context,
                                                GLuint id,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLuint *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectuivRobustANGLE_params(Context *context,
                                                GLuint id,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                GLuint *params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferPointervRobustANGLE_length(Context *context,
                                                BufferBinding targetPacked,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                void **params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferPointervRobustANGLE_params(Context *context,
                                                BufferBinding targetPacked,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                GLsizei *length,
                                                void **params,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetIntegeri_vRobustANGLE_length(Context *context,
                                            GLenum target,
                                            GLuint index,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLint *data,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetIntegeri_vRobustANGLE_data(Context *context,
                                          GLenum target,
                                          GLuint index,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLint *data,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInternalformativRobustANGLE_length(Context *context,
                                                  GLenum target,
                                                  GLenum internalformat,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLint *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInternalformativRobustANGLE_params(Context *context,
                                                  GLenum target,
                                                  GLenum internalformat,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLint *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribIivRobustANGLE_length(Context *context,
                                                 GLuint index,
                                                 GLenum pname,
                                                 GLsizei bufSize,
                                                 GLsizei *length,
                                                 GLint *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribIivRobustANGLE_params(Context *context,
                                                 GLuint index,
                                                 GLenum pname,
                                                 GLsizei bufSize,
                                                 GLsizei *length,
                                                 GLint *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribIuivRobustANGLE_length(Context *context,
                                                  GLuint index,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLuint *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetVertexAttribIuivRobustANGLE_params(Context *context,
                                                  GLuint index,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLuint *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformuivRobustANGLE_length(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLuint *params,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUniformuivRobustANGLE_params(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLuint *params,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniformBlockivRobustANGLE_length(Context *context,
                                                      GLuint program,
                                                      GLuint uniformBlockIndex,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei *length,
                                                      GLint *params,
                                                      bool isCallValid,
                                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetActiveUniformBlockivRobustANGLE_params(Context *context,
                                                      GLuint program,
                                                      GLuint uniformBlockIndex,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei *length,
                                                      GLint *params,
                                                      bool isCallValid,
                                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInteger64vRobustANGLE_length(Context *context,
                                            GLenum pname,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLint64 *data,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInteger64vRobustANGLE_data(Context *context,
                                          GLenum pname,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLint64 *data,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInteger64i_vRobustANGLE_length(Context *context,
                                              GLenum target,
                                              GLuint index,
                                              GLsizei bufSize,
                                              GLsizei *length,
                                              GLint64 *data,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetInteger64i_vRobustANGLE_data(Context *context,
                                            GLenum target,
                                            GLuint index,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLint64 *data,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferParameteri64vRobustANGLE_length(Context *context,
                                                     BufferBinding targetPacked,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLint64 *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferParameteri64vRobustANGLE_params(Context *context,
                                                     BufferBinding targetPacked,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLint64 *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameterivRobustANGLE_param(Context *context,
                                                GLuint sampler,
                                                GLuint pname,
                                                GLsizei bufSize,
                                                const GLint *param,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameterfvRobustANGLE_param(Context *context,
                                                GLuint sampler,
                                                GLenum pname,
                                                GLsizei bufSize,
                                                const GLfloat *param,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterivRobustANGLE_length(Context *context,
                                                    GLuint sampler,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLsizei *length,
                                                    GLint *params,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterivRobustANGLE_params(Context *context,
                                                    GLuint sampler,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLsizei *length,
                                                    GLint *params,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterfvRobustANGLE_length(Context *context,
                                                    GLuint sampler,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLsizei *length,
                                                    GLfloat *params,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterfvRobustANGLE_params(Context *context,
                                                    GLuint sampler,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLsizei *length,
                                                    GLfloat *params,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFramebufferParameterivRobustANGLE_length(Context *context,
                                                        GLuint sampler,
                                                        GLenum pname,
                                                        GLsizei bufSize,
                                                        GLsizei *length,
                                                        GLint *params,
                                                        bool isCallValid,
                                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFramebufferParameterivRobustANGLE_params(Context *context,
                                                        GLuint sampler,
                                                        GLenum pname,
                                                        GLsizei bufSize,
                                                        GLsizei *length,
                                                        GLint *params,
                                                        bool isCallValid,
                                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramInterfaceivRobustANGLE_length(Context *context,
                                                    GLuint program,
                                                    GLenum programInterface,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLsizei *length,
                                                    GLint *params,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramInterfaceivRobustANGLE_params(Context *context,
                                                    GLuint program,
                                                    GLenum programInterface,
                                                    GLenum pname,
                                                    GLsizei bufSize,
                                                    GLsizei *length,
                                                    GLint *params,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBooleani_vRobustANGLE_length(Context *context,
                                            GLenum target,
                                            GLuint index,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLboolean *data,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBooleani_vRobustANGLE_data(Context *context,
                                          GLenum target,
                                          GLuint index,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLboolean *data,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetMultisamplefvRobustANGLE_length(Context *context,
                                               GLenum pname,
                                               GLuint index,
                                               GLsizei bufSize,
                                               GLsizei *length,
                                               GLfloat *val,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetMultisamplefvRobustANGLE_val(Context *context,
                                            GLenum pname,
                                            GLuint index,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLfloat *val,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameterivRobustANGLE_length(Context *context,
                                                     TextureTarget targetPacked,
                                                     GLint level,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLint *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameterivRobustANGLE_params(Context *context,
                                                     TextureTarget targetPacked,
                                                     GLint level,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLint *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameterfvRobustANGLE_length(Context *context,
                                                     TextureTarget targetPacked,
                                                     GLint level,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLfloat *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameterfvRobustANGLE_params(Context *context,
                                                     TextureTarget targetPacked,
                                                     GLint level,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLfloat *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetPointervRobustANGLERobustANGLE_length(Context *context,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     void **params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetPointervRobustANGLERobustANGLE_params(Context *context,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     void **params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadnPixelsRobustANGLE_length(Context *context,
                                          GLint x,
                                          GLint y,
                                          GLsizei width,
                                          GLsizei height,
                                          GLenum format,
                                          GLenum type,
                                          GLsizei bufSize,
                                          GLsizei *length,
                                          GLsizei *columns,
                                          GLsizei *rows,
                                          void *data,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadnPixelsRobustANGLE_columns(Context *context,
                                           GLint x,
                                           GLint y,
                                           GLsizei width,
                                           GLsizei height,
                                           GLenum format,
                                           GLenum type,
                                           GLsizei bufSize,
                                           GLsizei *length,
                                           GLsizei *columns,
                                           GLsizei *rows,
                                           void *data,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadnPixelsRobustANGLE_rows(Context *context,
                                        GLint x,
                                        GLint y,
                                        GLsizei width,
                                        GLsizei height,
                                        GLenum format,
                                        GLenum type,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLsizei *columns,
                                        GLsizei *rows,
                                        void *data,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadnPixelsRobustANGLE_data(Context *context,
                                        GLint x,
                                        GLint y,
                                        GLsizei width,
                                        GLsizei height,
                                        GLenum format,
                                        GLenum type,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLsizei *columns,
                                        GLsizei *rows,
                                        void *data,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformfvRobustANGLE_length(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLfloat *params,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformfvRobustANGLE_params(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLfloat *params,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformivRobustANGLE_length(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLint *params,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformivRobustANGLE_params(Context *context,
                                            GLuint program,
                                            GLint location,
                                            GLsizei bufSize,
                                            GLsizei *length,
                                            GLint *params,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformuivRobustANGLE_length(Context *context,
                                             GLuint program,
                                             GLint location,
                                             GLsizei bufSize,
                                             GLsizei *length,
                                             GLuint *params,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformuivRobustANGLE_params(Context *context,
                                             GLuint program,
                                             GLint location,
                                             GLsizei bufSize,
                                             GLsizei *length,
                                             GLuint *params,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameterIivRobustANGLE_params(Context *context,
                                              TextureType targetPacked,
                                              GLenum pname,
                                              GLsizei bufSize,
                                              const GLint *params,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameterIuivRobustANGLE_params(Context *context,
                                               TextureType targetPacked,
                                               GLenum pname,
                                               GLsizei bufSize,
                                               const GLuint *params,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterIivRobustANGLE_length(Context *context,
                                                 TextureType targetPacked,
                                                 GLenum pname,
                                                 GLsizei bufSize,
                                                 GLsizei *length,
                                                 GLint *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterIivRobustANGLE_params(Context *context,
                                                 TextureType targetPacked,
                                                 GLenum pname,
                                                 GLsizei bufSize,
                                                 GLsizei *length,
                                                 GLint *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterIuivRobustANGLE_length(Context *context,
                                                  TextureType targetPacked,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLuint *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterIuivRobustANGLE_params(Context *context,
                                                  TextureType targetPacked,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLuint *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameterIivRobustANGLE_param(Context *context,
                                                 GLuint sampler,
                                                 GLenum pname,
                                                 GLsizei bufSize,
                                                 const GLint *param,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameterIuivRobustANGLE_param(Context *context,
                                                  GLuint sampler,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  const GLuint *param,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterIivRobustANGLE_length(Context *context,
                                                     GLuint sampler,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLint *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterIivRobustANGLE_params(Context *context,
                                                     GLuint sampler,
                                                     GLenum pname,
                                                     GLsizei bufSize,
                                                     GLsizei *length,
                                                     GLint *params,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterIuivRobustANGLE_length(Context *context,
                                                      GLuint sampler,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei *length,
                                                      GLuint *params,
                                                      bool isCallValid,
                                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterIuivRobustANGLE_params(Context *context,
                                                      GLuint sampler,
                                                      GLenum pname,
                                                      GLsizei bufSize,
                                                      GLsizei *length,
                                                      GLuint *params,
                                                      bool isCallValid,
                                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectivRobustANGLE_length(Context *context,
                                               GLuint id,
                                               GLenum pname,
                                               GLsizei bufSize,
                                               GLsizei *length,
                                               GLint *params,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectivRobustANGLE_params(Context *context,
                                               GLuint id,
                                               GLenum pname,
                                               GLsizei bufSize,
                                               GLsizei *length,
                                               GLint *params,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjecti64vRobustANGLE_length(Context *context,
                                                 GLuint id,
                                                 GLenum pname,
                                                 GLsizei bufSize,
                                                 GLsizei *length,
                                                 GLint64 *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjecti64vRobustANGLE_params(Context *context,
                                                 GLuint id,
                                                 GLenum pname,
                                                 GLsizei bufSize,
                                                 GLsizei *length,
                                                 GLint64 *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectui64vRobustANGLE_length(Context *context,
                                                  GLuint id,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLuint64 *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectui64vRobustANGLE_params(Context *context,
                                                  GLuint id,
                                                  GLenum pname,
                                                  GLsizei bufSize,
                                                  GLsizei *length,
                                                  GLuint64 *params,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameterivANGLE_params(Context *context,
                                               TextureTarget targetPacked,
                                               GLint level,
                                               GLenum pname,
                                               GLint *params,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexLevelParameterfvANGLE_params(Context *context,
                                               TextureTarget targetPacked,
                                               GLint level,
                                               GLenum pname,
                                               GLfloat *params,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetMultisamplefvANGLE_val(Context *context,
                                      GLenum pname,
                                      GLuint index,
                                      GLfloat *val,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTranslatedShaderSourceANGLE_length(Context *context,
                                                  GLuint shader,
                                                  GLsizei bufsize,
                                                  GLsizei *length,
                                                  GLchar *source,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTranslatedShaderSourceANGLE_source(Context *context,
                                                  GLuint shader,
                                                  GLsizei bufsize,
                                                  GLsizei *length,
                                                  GLchar *source,
                                                  bool isCallValid,
                                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureBindUniformLocationCHROMIUM_name(Context *context,
                                             GLuint program,
                                             GLint location,
                                             const GLchar *name,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMatrixLoadfCHROMIUM_matrix(Context *context,
                                       GLenum matrixMode,
                                       const GLfloat *matrix,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CapturePathCommandsCHROMIUM_commands(Context *context,
                                          GLuint path,
                                          GLsizei numCommands,
                                          const GLubyte *commands,
                                          GLsizei numCoords,
                                          GLenum coordType,
                                          const void *coords,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CapturePathCommandsCHROMIUM_coords(Context *context,
                                        GLuint path,
                                        GLsizei numCommands,
                                        const GLubyte *commands,
                                        GLsizei numCoords,
                                        GLenum coordType,
                                        const void *coords,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetPathParameterfvCHROMIUM_value(Context *context,
                                             GLuint path,
                                             GLenum pname,
                                             GLfloat *value,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetPathParameterivCHROMIUM_value(Context *context,
                                             GLuint path,
                                             GLenum pname,
                                             GLint *value,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCoverFillPathInstancedCHROMIUM_paths(Context *context,
                                                 GLsizei numPath,
                                                 GLenum pathNameType,
                                                 const void *paths,
                                                 GLuint pathBase,
                                                 GLenum coverMode,
                                                 GLenum transformType,
                                                 const GLfloat *transformValues,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCoverFillPathInstancedCHROMIUM_transformValues(Context *context,
                                                           GLsizei numPath,
                                                           GLenum pathNameType,
                                                           const void *paths,
                                                           GLuint pathBase,
                                                           GLenum coverMode,
                                                           GLenum transformType,
                                                           const GLfloat *transformValues,
                                                           bool isCallValid,
                                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCoverStrokePathInstancedCHROMIUM_paths(Context *context,
                                                   GLsizei numPath,
                                                   GLenum pathNameType,
                                                   const void *paths,
                                                   GLuint pathBase,
                                                   GLenum coverMode,
                                                   GLenum transformType,
                                                   const GLfloat *transformValues,
                                                   bool isCallValid,
                                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCoverStrokePathInstancedCHROMIUM_transformValues(Context *context,
                                                             GLsizei numPath,
                                                             GLenum pathNameType,
                                                             const void *paths,
                                                             GLuint pathBase,
                                                             GLenum coverMode,
                                                             GLenum transformType,
                                                             const GLfloat *transformValues,
                                                             bool isCallValid,
                                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilStrokePathInstancedCHROMIUM_paths(Context *context,
                                                     GLsizei numPath,
                                                     GLenum pathNameType,
                                                     const void *paths,
                                                     GLuint pathBase,
                                                     GLint reference,
                                                     GLuint mask,
                                                     GLenum transformType,
                                                     const GLfloat *transformValues,
                                                     bool isCallValid,
                                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilStrokePathInstancedCHROMIUM_transformValues(Context *context,
                                                               GLsizei numPath,
                                                               GLenum pathNameType,
                                                               const void *paths,
                                                               GLuint pathBase,
                                                               GLint reference,
                                                               GLuint mask,
                                                               GLenum transformType,
                                                               const GLfloat *transformValues,
                                                               bool isCallValid,
                                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilFillPathInstancedCHROMIUM_paths(Context *context,
                                                   GLsizei numPaths,
                                                   GLenum pathNameType,
                                                   const void *paths,
                                                   GLuint pathBase,
                                                   GLenum fillMode,
                                                   GLuint mask,
                                                   GLenum transformType,
                                                   const GLfloat *transformValues,
                                                   bool isCallValid,
                                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilFillPathInstancedCHROMIUM_transformValues(Context *context,
                                                             GLsizei numPaths,
                                                             GLenum pathNameType,
                                                             const void *paths,
                                                             GLuint pathBase,
                                                             GLenum fillMode,
                                                             GLuint mask,
                                                             GLenum transformType,
                                                             const GLfloat *transformValues,
                                                             bool isCallValid,
                                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilThenCoverFillPathInstancedCHROMIUM_paths(Context *context,
                                                            GLsizei numPaths,
                                                            GLenum pathNameType,
                                                            const void *paths,
                                                            GLuint pathBase,
                                                            GLenum fillMode,
                                                            GLuint mask,
                                                            GLenum coverMode,
                                                            GLenum transformType,
                                                            const GLfloat *transformValues,
                                                            bool isCallValid,
                                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilThenCoverFillPathInstancedCHROMIUM_transformValues(
    Context *context,
    GLsizei numPaths,
    GLenum pathNameType,
    const void *paths,
    GLuint pathBase,
    GLenum fillMode,
    GLuint mask,
    GLenum coverMode,
    GLenum transformType,
    const GLfloat *transformValues,
    bool isCallValid,
    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilThenCoverStrokePathInstancedCHROMIUM_paths(Context *context,
                                                              GLsizei numPaths,
                                                              GLenum pathNameType,
                                                              const void *paths,
                                                              GLuint pathBase,
                                                              GLint reference,
                                                              GLuint mask,
                                                              GLenum coverMode,
                                                              GLenum transformType,
                                                              const GLfloat *transformValues,
                                                              bool isCallValid,
                                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureStencilThenCoverStrokePathInstancedCHROMIUM_transformValues(
    Context *context,
    GLsizei numPaths,
    GLenum pathNameType,
    const void *paths,
    GLuint pathBase,
    GLint reference,
    GLuint mask,
    GLenum coverMode,
    GLenum transformType,
    const GLfloat *transformValues,
    bool isCallValid,
    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureBindFragmentInputLocationCHROMIUM_name(Context *context,
                                                   GLuint programs,
                                                   GLint location,
                                                   const GLchar *name,
                                                   bool isCallValid,
                                                   ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureProgramPathFragmentInputGenCHROMIUM_coeffs(Context *context,
                                                       GLuint program,
                                                       GLint location,
                                                       GLenum genMode,
                                                       GLint components,
                                                       const GLfloat *coeffs,
                                                       bool isCallValid,
                                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureBindFragDataLocationEXT_name(Context *context,
                                         GLuint program,
                                         GLuint color,
                                         const GLchar *name,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureBindFragDataLocationIndexedEXT_name(Context *context,
                                                GLuint program,
                                                GLuint colorNumber,
                                                GLuint index,
                                                const GLchar *name,
                                                bool isCallValid,
                                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFragDataIndexEXT_name(Context *context,
                                     GLuint program,
                                     const GLchar *name,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramResourceLocationIndexEXT_name(Context *context,
                                                    GLuint program,
                                                    GLenum programInterface,
                                                    const GLchar *name,
                                                    bool isCallValid,
                                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureInsertEventMarkerEXT_marker(Context *context,
                                        GLsizei length,
                                        const GLchar *marker,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CapturePushGroupMarkerEXT_marker(Context *context,
                                      GLsizei length,
                                      const GLchar *marker,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDiscardFramebufferEXT_attachments(Context *context,
                                              GLenum target,
                                              GLsizei numAttachments,
                                              const GLenum *attachments,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    CaptureMemory(attachments, sizeof(GLenum) * numAttachments, paramCapture);
}

void CaptureDeleteQueriesEXT_ids(Context *context,
                                 GLsizei n,
                                 const GLuint *ids,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenQueriesEXT_ids(Context *context,
                              GLsizei n,
                              GLuint *ids,
                              bool isCallValid,
                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjecti64vEXT_params(Context *context,
                                         GLuint id,
                                         GLenum pname,
                                         GLint64 *params,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectivEXT_params(Context *context,
                                       GLuint id,
                                       GLenum pname,
                                       GLint *params,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectui64vEXT_params(Context *context,
                                          GLuint id,
                                          GLenum pname,
                                          GLuint64 *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryObjectuivEXT_params(Context *context,
                                        GLuint id,
                                        GLenum pname,
                                        GLuint *params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetQueryivEXT_params(Context *context,
                                 QueryType targetPacked,
                                 GLenum pname,
                                 GLint *params,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawBuffersEXT_bufs(Context *context,
                                GLsizei n,
                                const GLenum *bufs,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawElementsInstancedEXT_indices(Context *context,
                                             PrimitiveMode modePacked,
                                             GLsizei count,
                                             DrawElementsType typePacked,
                                             const void *indices,
                                             GLsizei primcount,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCreateMemoryObjectsEXT_memoryObjects(Context *context,
                                                 GLsizei n,
                                                 GLuint *memoryObjects,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteMemoryObjectsEXT_memoryObjects(Context *context,
                                                 GLsizei n,
                                                 const GLuint *memoryObjects,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetMemoryObjectParameterivEXT_params(Context *context,
                                                 GLuint memoryObject,
                                                 GLenum pname,
                                                 GLint *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUnsignedBytevEXT_data(Context *context,
                                     GLenum pname,
                                     GLubyte *data,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetUnsignedBytei_vEXT_data(Context *context,
                                       GLenum target,
                                       GLuint index,
                                       GLubyte *data,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMemoryObjectParameterivEXT_params(Context *context,
                                              GLuint memoryObject,
                                              GLenum pname,
                                              const GLint *params,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformfvEXT_params(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei bufSize,
                                    GLfloat *params,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetnUniformivEXT_params(Context *context,
                                    GLuint program,
                                    GLint location,
                                    GLsizei bufSize,
                                    GLint *params,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureReadnPixelsEXT_data(Context *context,
                                GLint x,
                                GLint y,
                                GLsizei width,
                                GLsizei height,
                                GLenum format,
                                GLenum type,
                                GLsizei bufSize,
                                void *data,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteSemaphoresEXT_semaphores(Context *context,
                                           GLsizei n,
                                           const GLuint *semaphores,
                                           bool isCallValid,
                                           ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenSemaphoresEXT_semaphores(Context *context,
                                        GLsizei n,
                                        GLuint *semaphores,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSemaphoreParameterui64vEXT_params(Context *context,
                                                 GLuint semaphore,
                                                 GLenum pname,
                                                 GLuint64 *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSemaphoreParameterui64vEXT_params(Context *context,
                                              GLuint semaphore,
                                              GLenum pname,
                                              const GLuint64 *params,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSignalSemaphoreEXT_buffers(Context *context,
                                       GLuint semaphore,
                                       GLuint numBufferBarriers,
                                       const GLuint *buffers,
                                       GLuint numTextureBarriers,
                                       const GLuint *textures,
                                       const GLenum *dstLayouts,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSignalSemaphoreEXT_textures(Context *context,
                                        GLuint semaphore,
                                        GLuint numBufferBarriers,
                                        const GLuint *buffers,
                                        GLuint numTextureBarriers,
                                        const GLuint *textures,
                                        const GLenum *dstLayouts,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSignalSemaphoreEXT_dstLayouts(Context *context,
                                          GLuint semaphore,
                                          GLuint numBufferBarriers,
                                          const GLuint *buffers,
                                          GLuint numTextureBarriers,
                                          const GLuint *textures,
                                          const GLenum *dstLayouts,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureWaitSemaphoreEXT_buffers(Context *context,
                                     GLuint semaphore,
                                     GLuint numBufferBarriers,
                                     const GLuint *buffers,
                                     GLuint numTextureBarriers,
                                     const GLuint *textures,
                                     const GLenum *srcLayouts,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureWaitSemaphoreEXT_textures(Context *context,
                                      GLuint semaphore,
                                      GLuint numBufferBarriers,
                                      const GLuint *buffers,
                                      GLuint numTextureBarriers,
                                      const GLuint *textures,
                                      const GLenum *srcLayouts,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureWaitSemaphoreEXT_srcLayouts(Context *context,
                                        GLuint semaphore,
                                        GLuint numBufferBarriers,
                                        const GLuint *buffers,
                                        GLuint numTextureBarriers,
                                        const GLuint *textures,
                                        const GLenum *srcLayouts,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDebugMessageCallbackKHR_userParam(Context *context,
                                              GLDEBUGPROCKHR callback,
                                              const void *userParam,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDebugMessageControlKHR_ids(Context *context,
                                       GLenum source,
                                       GLenum type,
                                       GLenum severity,
                                       GLsizei count,
                                       const GLuint *ids,
                                       GLboolean enabled,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDebugMessageInsertKHR_buf(Context *context,
                                      GLenum source,
                                      GLenum type,
                                      GLuint id,
                                      GLenum severity,
                                      GLsizei length,
                                      const GLchar *buf,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetDebugMessageLogKHR_sources(Context *context,
                                          GLuint count,
                                          GLsizei bufSize,
                                          GLenum *sources,
                                          GLenum *types,
                                          GLuint *ids,
                                          GLenum *severities,
                                          GLsizei *lengths,
                                          GLchar *messageLog,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetDebugMessageLogKHR_types(Context *context,
                                        GLuint count,
                                        GLsizei bufSize,
                                        GLenum *sources,
                                        GLenum *types,
                                        GLuint *ids,
                                        GLenum *severities,
                                        GLsizei *lengths,
                                        GLchar *messageLog,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetDebugMessageLogKHR_ids(Context *context,
                                      GLuint count,
                                      GLsizei bufSize,
                                      GLenum *sources,
                                      GLenum *types,
                                      GLuint *ids,
                                      GLenum *severities,
                                      GLsizei *lengths,
                                      GLchar *messageLog,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetDebugMessageLogKHR_severities(Context *context,
                                             GLuint count,
                                             GLsizei bufSize,
                                             GLenum *sources,
                                             GLenum *types,
                                             GLuint *ids,
                                             GLenum *severities,
                                             GLsizei *lengths,
                                             GLchar *messageLog,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetDebugMessageLogKHR_lengths(Context *context,
                                          GLuint count,
                                          GLsizei bufSize,
                                          GLenum *sources,
                                          GLenum *types,
                                          GLuint *ids,
                                          GLenum *severities,
                                          GLsizei *lengths,
                                          GLchar *messageLog,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetDebugMessageLogKHR_messageLog(Context *context,
                                             GLuint count,
                                             GLsizei bufSize,
                                             GLenum *sources,
                                             GLenum *types,
                                             GLuint *ids,
                                             GLenum *severities,
                                             GLsizei *lengths,
                                             GLchar *messageLog,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetObjectLabelKHR_length(Context *context,
                                     GLenum identifier,
                                     GLuint name,
                                     GLsizei bufSize,
                                     GLsizei *length,
                                     GLchar *label,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetObjectLabelKHR_label(Context *context,
                                    GLenum identifier,
                                    GLuint name,
                                    GLsizei bufSize,
                                    GLsizei *length,
                                    GLchar *label,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetObjectPtrLabelKHR_ptr(Context *context,
                                     const void *ptr,
                                     GLsizei bufSize,
                                     GLsizei *length,
                                     GLchar *label,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetObjectPtrLabelKHR_length(Context *context,
                                        const void *ptr,
                                        GLsizei bufSize,
                                        GLsizei *length,
                                        GLchar *label,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetObjectPtrLabelKHR_label(Context *context,
                                       const void *ptr,
                                       GLsizei bufSize,
                                       GLsizei *length,
                                       GLchar *label,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetPointervKHR_params(Context *context,
                                  GLenum pname,
                                  void **params,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureObjectLabelKHR_label(Context *context,
                                 GLenum identifier,
                                 GLuint name,
                                 GLsizei length,
                                 const GLchar *label,
                                 bool isCallValid,
                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureObjectPtrLabelKHR_ptr(Context *context,
                                  const void *ptr,
                                  GLsizei length,
                                  const GLchar *label,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureObjectPtrLabelKHR_label(Context *context,
                                    const void *ptr,
                                    GLsizei length,
                                    const GLchar *label,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CapturePushDebugGroupKHR_message(Context *context,
                                      GLenum source,
                                      GLuint id,
                                      GLsizei length,
                                      const GLchar *message,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteFencesNV_fences(Context *context,
                                  GLsizei n,
                                  const GLuint *fences,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenFencesNV_fences(Context *context,
                               GLsizei n,
                               GLuint *fences,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFenceivNV_params(Context *context,
                                GLuint fence,
                                GLenum pname,
                                GLint *params,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawTexfvOES_coords(Context *context,
                                const GLfloat *coords,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawTexivOES_coords(Context *context,
                                const GLint *coords,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawTexsvOES_coords(Context *context,
                                const GLshort *coords,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDrawTexxvOES_coords(Context *context,
                                const GLfixed *coords,
                                bool isCallValid,
                                ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteFramebuffersOES_framebuffers(Context *context,
                                               GLsizei n,
                                               const GLuint *framebuffers,
                                               bool isCallValid,
                                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteRenderbuffersOES_renderbuffers(Context *context,
                                                 GLsizei n,
                                                 const GLuint *renderbuffers,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenFramebuffersOES_framebuffers(Context *context,
                                            GLsizei n,
                                            GLuint *framebuffers,
                                            bool isCallValid,
                                            ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenRenderbuffersOES_renderbuffers(Context *context,
                                              GLsizei n,
                                              GLuint *renderbuffers,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetFramebufferAttachmentParameterivOES_params(Context *context,
                                                          GLenum target,
                                                          GLenum attachment,
                                                          GLenum pname,
                                                          GLint *params,
                                                          bool isCallValid,
                                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetRenderbufferParameterivOES_params(Context *context,
                                                 GLenum target,
                                                 GLenum pname,
                                                 GLint *params,
                                                 bool isCallValid,
                                                 ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetProgramBinaryOES_length(Context *context,
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

void CaptureGetProgramBinaryOES_binaryFormat(Context *context,
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

void CaptureGetProgramBinaryOES_binary(Context *context,
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

void CaptureProgramBinaryOES_binary(Context *context,
                                    GLuint program,
                                    GLenum binaryFormat,
                                    const void *binary,
                                    GLint length,
                                    bool isCallValid,
                                    ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetBufferPointervOES_params(Context *context,
                                        BufferBinding targetPacked,
                                        GLenum pname,
                                        void **params,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureMatrixIndexPointerOES_pointer(Context *context,
                                          GLint size,
                                          GLenum type,
                                          GLsizei stride,
                                          const void *pointer,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureWeightPointerOES_pointer(Context *context,
                                     GLint size,
                                     GLenum type,
                                     GLsizei stride,
                                     const void *pointer,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CapturePointSizePointerOES_pointer(Context *context,
                                        VertexAttribType typePacked,
                                        GLsizei stride,
                                        const void *pointer,
                                        bool isCallValid,
                                        ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureQueryMatrixxOES_mantissa(Context *context,
                                     GLfixed *mantissa,
                                     GLint *exponent,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureQueryMatrixxOES_exponent(Context *context,
                                     GLfixed *mantissa,
                                     GLint *exponent,
                                     bool isCallValid,
                                     ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexImage3DOES_data(Context *context,
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
                                         angle::ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureCompressedTexSubImage3DOES_data(Context *context,
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
                                            angle::ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexImage3DOES_pixels(Context *context,
                                 TextureTarget targetPacked,
                                 GLint level,
                                 GLenum internalformat,
                                 GLsizei width,
                                 GLsizei height,
                                 GLsizei depth,
                                 GLint border,
                                 GLenum format,
                                 GLenum type,
                                 const void *pixels,
                                 bool isCallValid,
                                 angle::ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexSubImage3DOES_pixels(Context *context,
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
                                    angle::ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterIivOES_params(Context *context,
                                             GLuint sampler,
                                             GLenum pname,
                                             GLint *params,
                                             bool isCallValid,
                                             ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetSamplerParameterIuivOES_params(Context *context,
                                              GLuint sampler,
                                              GLenum pname,
                                              GLuint *params,
                                              bool isCallValid,
                                              ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterIivOES_params(Context *context,
                                         TextureType targetPacked,
                                         GLenum pname,
                                         GLint *params,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexParameterIuivOES_params(Context *context,
                                          TextureType targetPacked,
                                          GLenum pname,
                                          GLuint *params,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameterIivOES_param(Context *context,
                                         GLuint sampler,
                                         GLenum pname,
                                         const GLint *param,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureSamplerParameterIuivOES_param(Context *context,
                                          GLuint sampler,
                                          GLenum pname,
                                          const GLuint *param,
                                          bool isCallValid,
                                          ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameterIivOES_params(Context *context,
                                      TextureType targetPacked,
                                      GLenum pname,
                                      const GLint *params,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexParameterIuivOES_params(Context *context,
                                       TextureType targetPacked,
                                       GLenum pname,
                                       const GLuint *params,
                                       bool isCallValid,
                                       ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexGenfvOES_params(Context *context,
                                  GLenum coord,
                                  GLenum pname,
                                  GLfloat *params,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexGenivOES_params(Context *context,
                                  GLenum coord,
                                  GLenum pname,
                                  GLint *params,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGetTexGenxvOES_params(Context *context,
                                  GLenum coord,
                                  GLenum pname,
                                  GLfixed *params,
                                  bool isCallValid,
                                  ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexGenfvOES_params(Context *context,
                               GLenum coord,
                               GLenum pname,
                               const GLfloat *params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexGenivOES_params(Context *context,
                               GLenum coord,
                               GLenum pname,
                               const GLint *params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureTexGenxvOES_params(Context *context,
                               GLenum coord,
                               GLenum pname,
                               const GLfixed *params,
                               bool isCallValid,
                               ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureDeleteVertexArraysOES_arrays(Context *context,
                                         GLsizei n,
                                         const GLuint *arrays,
                                         bool isCallValid,
                                         ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

void CaptureGenVertexArraysOES_arrays(Context *context,
                                      GLsizei n,
                                      GLuint *arrays,
                                      bool isCallValid,
                                      ParamCapture *paramCapture)
{
    UNIMPLEMENTED();
}

}  // namespace gl
