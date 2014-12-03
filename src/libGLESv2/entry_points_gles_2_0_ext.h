//
// Copyright(c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// entry_points_gles_2_0_ext.h : Defines the GLES 2.0 extension entry points.

#ifndef LIBGLESV2_ENTRYPOINTGLES20EXT_H_
#define LIBGLESV2_ENTRYPOINTGLES20EXT_H_

#include "libGLESv2/export.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace gl
{

// GL_ANGLE_framebuffer_blit
ANGLE_EXPORT void BlitFramebufferANGLE(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

// GL_ANGLE_framebuffer_multisample
ANGLE_EXPORT void RenderbufferStorageMultisampleANGLE(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);

// GL_NV_fence
ANGLE_EXPORT void DeleteFencesNV(GLsizei n, const GLuint* fences);
ANGLE_EXPORT void GenFencesNV(GLsizei n, GLuint* fences);
ANGLE_EXPORT GLboolean IsFenceNV(GLuint fence);
ANGLE_EXPORT GLboolean TestFenceNV(GLuint fence);
ANGLE_EXPORT void GetFenceivNV(GLuint fence, GLenum pname, GLint *params);
ANGLE_EXPORT void FinishFenceNV(GLuint fence);
ANGLE_EXPORT void SetFenceNV(GLuint fence, GLenum condition);

// GL_ANGLE_translated_shader_source
ANGLE_EXPORT void GetTranslatedShaderSourceANGLE(GLuint shader, GLsizei bufsize, GLsizei *length, GLchar *source);

// GL_EXT_texture_storage
ANGLE_EXPORT void TexStorage2DEXT(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);

// GL_EXT_robustness
ANGLE_EXPORT GLenum GetGraphicsResetStatusEXT(void);
ANGLE_EXPORT void ReadnPixelsEXT(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
ANGLE_EXPORT void GetnUniformfvEXT(GLuint program, GLint location, GLsizei bufSize, float *params);
ANGLE_EXPORT void GetnUniformivEXT(GLuint program, GLint location, GLsizei bufSize, GLint *params);

// GL_EXT_occlusion_query_boolean
ANGLE_EXPORT void GenQueriesEXT(GLsizei n, GLuint *ids);
ANGLE_EXPORT void DeleteQueriesEXT(GLsizei n, const GLuint *ids);
ANGLE_EXPORT GLboolean IsQueryEXT(GLuint id);
ANGLE_EXPORT void BeginQueryEXT(GLenum target, GLuint id);
ANGLE_EXPORT void EndQueryEXT(GLenum target);
ANGLE_EXPORT void GetQueryivEXT(GLenum target, GLenum pname, GLint *params);
ANGLE_EXPORT void GetQueryObjectuivEXT(GLuint id, GLenum pname, GLuint *params);

// GL_EXT_draw_buffers
ANGLE_EXPORT void DrawBuffersEXT(GLsizei n, const GLenum *bufs);

// GL_ANGLE_instanced_arrays
ANGLE_EXPORT void DrawArraysInstancedANGLE(GLenum mode, GLint first, GLsizei count, GLsizei primcount);
ANGLE_EXPORT void DrawElementsInstancedANGLE(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount);
ANGLE_EXPORT void VertexAttribDivisorANGLE(GLuint index, GLuint divisor);

// GL_OES_get_program_binary
ANGLE_EXPORT void GetProgramBinaryOES(GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, GLvoid *binary);
ANGLE_EXPORT void ProgramBinaryOES(GLuint program, GLenum binaryFormat, const GLvoid *binary, GLint length);

// GL_OES_mapbuffer
ANGLE_EXPORT void *MapBufferOES(GLenum target, GLenum access);
ANGLE_EXPORT GLboolean UnmapBufferOES(GLenum target);
ANGLE_EXPORT void GetBufferPointervOES(GLenum target, GLenum pname, GLvoid **params);

// GL_EXT_map_buffer_range
ANGLE_EXPORT void *MapBufferRangeEXT(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
ANGLE_EXPORT void FlushMappedBufferRangeEXT(GLenum target, GLintptr offset, GLsizeiptr length);

}

#endif // LIBGLESV2_ENTRYPOINTGLES20EXT_H_
