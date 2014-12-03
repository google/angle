//
// Copyright(c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// entry_points_gles_3_0.h : Defines the GLES 3.0 entry points.

#ifndef LIBGLESV2_ENTRYPOINTGLES30_H_
#define LIBGLESV2_ENTRYPOINTGLES30_H_

#include "libGLESv2/export.h"

#include <GLES3/gl3.h>

namespace gl
{

ANGLE_EXPORT void ReadBuffer(GLenum mode);
ANGLE_EXPORT void DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid* indices);
ANGLE_EXPORT void TexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
ANGLE_EXPORT void TexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid* pixels);
ANGLE_EXPORT void CopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
ANGLE_EXPORT void CompressedTexImage3D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
ANGLE_EXPORT void CompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
ANGLE_EXPORT void GenQueries(GLsizei n, GLuint* ids);
ANGLE_EXPORT void DeleteQueries(GLsizei n, const GLuint* ids);
ANGLE_EXPORT GLboolean IsQuery(GLuint id);
ANGLE_EXPORT void BeginQuery(GLenum target, GLuint id);
ANGLE_EXPORT void EndQuery(GLenum target);
ANGLE_EXPORT void GetQueryiv(GLenum target, GLenum pname, GLint* params);
ANGLE_EXPORT void GetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params);
ANGLE_EXPORT GLboolean UnmapBuffer(GLenum target);
ANGLE_EXPORT void GetBufferPointerv(GLenum target, GLenum pname, GLvoid** params);
ANGLE_EXPORT void DrawBuffers(GLsizei n, const GLenum* bufs);
ANGLE_EXPORT void UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
ANGLE_EXPORT void RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
ANGLE_EXPORT void FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
ANGLE_EXPORT GLvoid* MapBufferRange(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
ANGLE_EXPORT void FlushMappedBufferRange(GLenum target, GLintptr offset, GLsizeiptr length);
ANGLE_EXPORT void BindVertexArray(GLuint array);
ANGLE_EXPORT void DeleteVertexArrays(GLsizei n, const GLuint* arrays);
ANGLE_EXPORT void GenVertexArrays(GLsizei n, GLuint* arrays);
ANGLE_EXPORT GLboolean IsVertexArray(GLuint array);
ANGLE_EXPORT void GetIntegeri_v(GLenum target, GLuint index, GLint* data);
ANGLE_EXPORT void BeginTransformFeedback(GLenum primitiveMode);
ANGLE_EXPORT void EndTransformFeedback(void);
ANGLE_EXPORT void BindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
ANGLE_EXPORT void BindBufferBase(GLenum target, GLuint index, GLuint buffer);
ANGLE_EXPORT void TransformFeedbackVaryings(GLuint program, GLsizei count, const GLchar* const* varyings, GLenum bufferMode);
ANGLE_EXPORT void GetTransformFeedbackVarying(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLsizei* size, GLenum* type, GLchar* name);
ANGLE_EXPORT void VertexAttribIPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid* pointer);
ANGLE_EXPORT void GetVertexAttribIiv(GLuint index, GLenum pname, GLint* params);
ANGLE_EXPORT void GetVertexAttribIuiv(GLuint index, GLenum pname, GLuint* params);
ANGLE_EXPORT void VertexAttribI4i(GLuint index, GLint x, GLint y, GLint z, GLint w);
ANGLE_EXPORT void VertexAttribI4ui(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
ANGLE_EXPORT void VertexAttribI4iv(GLuint index, const GLint* v);
ANGLE_EXPORT void VertexAttribI4uiv(GLuint index, const GLuint* v);
ANGLE_EXPORT void GetUniformuiv(GLuint program, GLint location, GLuint* params);
ANGLE_EXPORT GLint GetFragDataLocation(GLuint program, const GLchar *name);
ANGLE_EXPORT void Uniform1ui(GLint location, GLuint v0);
ANGLE_EXPORT void Uniform2ui(GLint location, GLuint v0, GLuint v1);
ANGLE_EXPORT void Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);
ANGLE_EXPORT void Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
ANGLE_EXPORT void Uniform1uiv(GLint location, GLsizei count, const GLuint* value);
ANGLE_EXPORT void Uniform2uiv(GLint location, GLsizei count, const GLuint* value);
ANGLE_EXPORT void Uniform3uiv(GLint location, GLsizei count, const GLuint* value);
ANGLE_EXPORT void Uniform4uiv(GLint location, GLsizei count, const GLuint* value);
ANGLE_EXPORT void ClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value);
ANGLE_EXPORT void ClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value);
ANGLE_EXPORT void ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value);
ANGLE_EXPORT void ClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
ANGLE_EXPORT const GLubyte* GetStringi(GLenum name, GLuint index);
ANGLE_EXPORT void CopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
ANGLE_EXPORT void GetUniformIndices(GLuint program, GLsizei uniformCount, const GLchar* const* uniformNames, GLuint* uniformIndices);
ANGLE_EXPORT void GetActiveUniformsiv(GLuint program, GLsizei uniformCount, const GLuint* uniformIndices, GLenum pname, GLint* params);
ANGLE_EXPORT GLuint GetUniformBlockIndex(GLuint program, const GLchar* uniformBlockName);
ANGLE_EXPORT void GetActiveUniformBlockiv(GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint* params);
ANGLE_EXPORT void GetActiveUniformBlockName(GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei* length, GLchar* uniformBlockName);
ANGLE_EXPORT void UniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
ANGLE_EXPORT void DrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
ANGLE_EXPORT void DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices, GLsizei instanceCount);
ANGLE_EXPORT GLsync FenceSync_(GLenum condition, GLbitfield flags);
ANGLE_EXPORT GLboolean IsSync(GLsync sync);
ANGLE_EXPORT void DeleteSync(GLsync sync);
ANGLE_EXPORT GLenum ClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
ANGLE_EXPORT void WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);
ANGLE_EXPORT void GetInteger64v(GLenum pname, GLint64* params);
ANGLE_EXPORT void GetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei* length, GLint* values);
ANGLE_EXPORT void GetInteger64i_v(GLenum target, GLuint index, GLint64* data);
ANGLE_EXPORT void GetBufferParameteri64v(GLenum target, GLenum pname, GLint64* params);
ANGLE_EXPORT void GenSamplers(GLsizei count, GLuint* samplers);
ANGLE_EXPORT void DeleteSamplers(GLsizei count, const GLuint* samplers);
ANGLE_EXPORT GLboolean IsSampler(GLuint sampler);
ANGLE_EXPORT void BindSampler(GLuint unit, GLuint sampler);
ANGLE_EXPORT void SamplerParameteri(GLuint sampler, GLenum pname, GLint param);
ANGLE_EXPORT void SamplerParameteriv(GLuint sampler, GLenum pname, const GLint* param);
ANGLE_EXPORT void SamplerParameterf(GLuint sampler, GLenum pname, GLfloat param);
ANGLE_EXPORT void SamplerParameterfv(GLuint sampler, GLenum pname, const GLfloat* param);
ANGLE_EXPORT void GetSamplerParameteriv(GLuint sampler, GLenum pname, GLint* params);
ANGLE_EXPORT void GetSamplerParameterfv(GLuint sampler, GLenum pname, GLfloat* params);
ANGLE_EXPORT void VertexAttribDivisor(GLuint index, GLuint divisor);
ANGLE_EXPORT void BindTransformFeedback(GLenum target, GLuint id);
ANGLE_EXPORT void DeleteTransformFeedbacks(GLsizei n, const GLuint* ids);
ANGLE_EXPORT void GenTransformFeedbacks(GLsizei n, GLuint* ids);
ANGLE_EXPORT GLboolean IsTransformFeedback(GLuint id);
ANGLE_EXPORT void PauseTransformFeedback(void);
ANGLE_EXPORT void ResumeTransformFeedback(void);
ANGLE_EXPORT void GetProgramBinary(GLuint program, GLsizei bufSize, GLsizei* length, GLenum* binaryFormat, GLvoid* binary);
ANGLE_EXPORT void ProgramBinary(GLuint program, GLenum binaryFormat, const GLvoid* binary, GLsizei length);
ANGLE_EXPORT void ProgramParameteri(GLuint program, GLenum pname, GLint value);
ANGLE_EXPORT void InvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments);
ANGLE_EXPORT void InvalidateSubFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments, GLint x, GLint y, GLsizei width, GLsizei height);
ANGLE_EXPORT void TexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
ANGLE_EXPORT void TexStorage3D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
ANGLE_EXPORT void GetInternalformativ(GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint* params);

}

#endif // LIBGLESV2_ENTRYPOINTGLES30_H_
