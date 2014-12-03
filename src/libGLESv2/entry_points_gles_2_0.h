//
// Copyright(c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// entry_points_gles_2_0.h : Defines the GLES 2.0 entry points.

#ifndef LIBGLESV2_ENTRYPOINTGLES20_H_
#define LIBGLESV2_ENTRYPOINTGLES20_H_

#include "libGLESv2/export.h"

#include <GLES2/gl2.h>

namespace gl
{

ANGLE_EXPORT void ActiveTexture(GLenum texture);
ANGLE_EXPORT void AttachShader(GLuint program, GLuint shader);
ANGLE_EXPORT void BindAttribLocation(GLuint program, GLuint index, const GLchar* name);
ANGLE_EXPORT void BindBuffer(GLenum target, GLuint buffer);
ANGLE_EXPORT void BindFramebuffer(GLenum target, GLuint framebuffer);
ANGLE_EXPORT void BindRenderbuffer(GLenum target, GLuint renderbuffer);
ANGLE_EXPORT void BindTexture(GLenum target, GLuint texture);
ANGLE_EXPORT void BlendColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
ANGLE_EXPORT void BlendEquation(GLenum mode);
ANGLE_EXPORT void BlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha);
ANGLE_EXPORT void BlendFunc(GLenum sfactor, GLenum dfactor);
ANGLE_EXPORT void BlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
ANGLE_EXPORT void BufferData(GLenum target, GLsizeiptr size, const GLvoid* data, GLenum usage);
ANGLE_EXPORT void BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid* data);
ANGLE_EXPORT GLenum CheckFramebufferStatus(GLenum target);
ANGLE_EXPORT void Clear(GLbitfield mask);
ANGLE_EXPORT void ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
ANGLE_EXPORT void ClearDepthf(GLfloat depth);
ANGLE_EXPORT void ClearStencil(GLint s);
ANGLE_EXPORT void ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
ANGLE_EXPORT void CompileShader(GLuint shader);
ANGLE_EXPORT void CompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
ANGLE_EXPORT void CompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
ANGLE_EXPORT void CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
ANGLE_EXPORT void CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
ANGLE_EXPORT GLuint CreateProgram(void);
ANGLE_EXPORT GLuint CreateShader(GLenum type);
ANGLE_EXPORT void CullFace(GLenum mode);
ANGLE_EXPORT void DeleteBuffers(GLsizei n, const GLuint* buffers);
ANGLE_EXPORT void DeleteFramebuffers(GLsizei n, const GLuint* framebuffers);
ANGLE_EXPORT void DeleteProgram(GLuint program);
ANGLE_EXPORT void DeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers);
ANGLE_EXPORT void DeleteShader(GLuint shader);
ANGLE_EXPORT void DeleteTextures(GLsizei n, const GLuint* textures);
ANGLE_EXPORT void DepthFunc(GLenum func);
ANGLE_EXPORT void DepthMask(GLboolean flag);
ANGLE_EXPORT void DepthRangef(GLfloat n, GLfloat f);
ANGLE_EXPORT void DetachShader(GLuint program, GLuint shader);
ANGLE_EXPORT void Disable(GLenum cap);
ANGLE_EXPORT void DisableVertexAttribArray(GLuint index);
ANGLE_EXPORT void DrawArrays(GLenum mode, GLint first, GLsizei count);
ANGLE_EXPORT void DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices);
ANGLE_EXPORT void Enable(GLenum cap);
ANGLE_EXPORT void EnableVertexAttribArray(GLuint index);
ANGLE_EXPORT void Finish(void);
ANGLE_EXPORT void Flush(void);
ANGLE_EXPORT void FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
ANGLE_EXPORT void FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
ANGLE_EXPORT void FrontFace(GLenum mode);
ANGLE_EXPORT void GenBuffers(GLsizei n, GLuint* buffers);
ANGLE_EXPORT void GenerateMipmap(GLenum target);
ANGLE_EXPORT void GenFramebuffers(GLsizei n, GLuint* framebuffers);
ANGLE_EXPORT void GenRenderbuffers(GLsizei n, GLuint* renderbuffers);
ANGLE_EXPORT void GenTextures(GLsizei n, GLuint* textures);
ANGLE_EXPORT void GetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
ANGLE_EXPORT void GetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, GLchar* name);
ANGLE_EXPORT void GetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders);
ANGLE_EXPORT GLint GetAttribLocation(GLuint program, const GLchar* name);
ANGLE_EXPORT void GetBooleanv(GLenum pname, GLboolean* params);
ANGLE_EXPORT void GetBufferParameteriv(GLenum target, GLenum pname, GLint* params);
ANGLE_EXPORT GLenum GetError(void);
ANGLE_EXPORT void GetFloatv(GLenum pname, GLfloat* params);
ANGLE_EXPORT void GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params);
ANGLE_EXPORT void GetIntegerv(GLenum pname, GLint* params);
ANGLE_EXPORT void GetProgramiv(GLuint program, GLenum pname, GLint* params);
ANGLE_EXPORT void GetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, GLchar* infolog);
ANGLE_EXPORT void GetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params);
ANGLE_EXPORT void GetShaderiv(GLuint shader, GLenum pname, GLint* params);
ANGLE_EXPORT void GetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* infolog);
ANGLE_EXPORT void GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision);
ANGLE_EXPORT void GetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, GLchar* source);
ANGLE_EXPORT const GLubyte* GetString(GLenum name);
ANGLE_EXPORT void GetTexParameterfv(GLenum target, GLenum pname, GLfloat* params);
ANGLE_EXPORT void GetTexParameteriv(GLenum target, GLenum pname, GLint* params);
ANGLE_EXPORT void GetUniformfv(GLuint program, GLint location, GLfloat* params);
ANGLE_EXPORT void GetUniformiv(GLuint program, GLint location, GLint* params);
ANGLE_EXPORT GLint GetUniformLocation(GLuint program, const GLchar* name);
ANGLE_EXPORT void GetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params);
ANGLE_EXPORT void GetVertexAttribiv(GLuint index, GLenum pname, GLint* params);
ANGLE_EXPORT void GetVertexAttribPointerv(GLuint index, GLenum pname, GLvoid** pointer);
ANGLE_EXPORT void Hint(GLenum target, GLenum mode);
ANGLE_EXPORT GLboolean IsBuffer(GLuint buffer);
ANGLE_EXPORT GLboolean IsEnabled(GLenum cap);
ANGLE_EXPORT GLboolean IsFramebuffer(GLuint framebuffer);
ANGLE_EXPORT GLboolean IsProgram(GLuint program);
ANGLE_EXPORT GLboolean IsRenderbuffer(GLuint renderbuffer);
ANGLE_EXPORT GLboolean IsShader(GLuint shader);
ANGLE_EXPORT GLboolean IsTexture(GLuint texture);
ANGLE_EXPORT void LineWidth(GLfloat width);
ANGLE_EXPORT void LinkProgram(GLuint program);
ANGLE_EXPORT void PixelStorei(GLenum pname, GLint param);
ANGLE_EXPORT void PolygonOffset(GLfloat factor, GLfloat units);
ANGLE_EXPORT void ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid* pixels);
ANGLE_EXPORT void ReleaseShaderCompiler(void);
ANGLE_EXPORT void RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
ANGLE_EXPORT void SampleCoverage(GLfloat value, GLboolean invert);
ANGLE_EXPORT void Scissor(GLint x, GLint y, GLsizei width, GLsizei height);
ANGLE_EXPORT void ShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const GLvoid* binary, GLsizei length);
ANGLE_EXPORT void ShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
ANGLE_EXPORT void StencilFunc(GLenum func, GLint ref, GLuint mask);
ANGLE_EXPORT void StencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask);
ANGLE_EXPORT void StencilMask(GLuint mask);
ANGLE_EXPORT void StencilMaskSeparate(GLenum face, GLuint mask);
ANGLE_EXPORT void StencilOp(GLenum fail, GLenum zfail, GLenum zpass);
ANGLE_EXPORT void StencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass);
ANGLE_EXPORT void TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid* pixels);
ANGLE_EXPORT void TexParameterf(GLenum target, GLenum pname, GLfloat param);
ANGLE_EXPORT void TexParameterfv(GLenum target, GLenum pname, const GLfloat* params);
ANGLE_EXPORT void TexParameteri(GLenum target, GLenum pname, GLint param);
ANGLE_EXPORT void TexParameteriv(GLenum target, GLenum pname, const GLint* params);
ANGLE_EXPORT void TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid* pixels);
ANGLE_EXPORT void Uniform1f(GLint location, GLfloat x);
ANGLE_EXPORT void Uniform1fv(GLint location, GLsizei count, const GLfloat* v);
ANGLE_EXPORT void Uniform1i(GLint location, GLint x);
ANGLE_EXPORT void Uniform1iv(GLint location, GLsizei count, const GLint* v);
ANGLE_EXPORT void Uniform2f(GLint location, GLfloat x, GLfloat y);
ANGLE_EXPORT void Uniform2fv(GLint location, GLsizei count, const GLfloat* v);
ANGLE_EXPORT void Uniform2i(GLint location, GLint x, GLint y);
ANGLE_EXPORT void Uniform2iv(GLint location, GLsizei count, const GLint* v);
ANGLE_EXPORT void Uniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z);
ANGLE_EXPORT void Uniform3fv(GLint location, GLsizei count, const GLfloat* v);
ANGLE_EXPORT void Uniform3i(GLint location, GLint x, GLint y, GLint z);
ANGLE_EXPORT void Uniform3iv(GLint location, GLsizei count, const GLint* v);
ANGLE_EXPORT void Uniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
ANGLE_EXPORT void Uniform4fv(GLint location, GLsizei count, const GLfloat* v);
ANGLE_EXPORT void Uniform4i(GLint location, GLint x, GLint y, GLint z, GLint w);
ANGLE_EXPORT void Uniform4iv(GLint location, GLsizei count, const GLint* v);
ANGLE_EXPORT void UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
ANGLE_EXPORT void UseProgram(GLuint program);
ANGLE_EXPORT void ValidateProgram(GLuint program);
ANGLE_EXPORT void VertexAttrib1f(GLuint indx, GLfloat x);
ANGLE_EXPORT void VertexAttrib1fv(GLuint indx, const GLfloat* values);
ANGLE_EXPORT void VertexAttrib2f(GLuint indx, GLfloat x, GLfloat y);
ANGLE_EXPORT void VertexAttrib2fv(GLuint indx, const GLfloat* values);
ANGLE_EXPORT void VertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z);
ANGLE_EXPORT void VertexAttrib3fv(GLuint indx, const GLfloat* values);
ANGLE_EXPORT void VertexAttrib4f(GLuint indx, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
ANGLE_EXPORT void VertexAttrib4fv(GLuint indx, const GLfloat* values);
ANGLE_EXPORT void VertexAttribPointer(GLuint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
ANGLE_EXPORT void Viewport(GLint x, GLint y, GLsizei width, GLsizei height);

}

#endif // LIBGLESV2_ENTRYPOINTGLES20_H_
