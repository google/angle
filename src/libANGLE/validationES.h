//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES.h: Validation functions for generic OpenGL ES entry point parameters

#ifndef LIBANGLE_VALIDATION_ES_H_
#define LIBANGLE_VALIDATION_ES_H_

#include "libANGLE/export.h"

#include "common/mathutil.h"

#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

namespace gl
{

class Context;

ANGLE_EXPORT bool ValidCap(const Context *context, GLenum cap);
ANGLE_EXPORT bool ValidTextureTarget(const Context *context, GLenum target);
ANGLE_EXPORT bool ValidTexture2DDestinationTarget(const Context *context, GLenum target);
ANGLE_EXPORT bool ValidFramebufferTarget(GLenum target);
ANGLE_EXPORT bool ValidBufferTarget(const Context *context, GLenum target);
ANGLE_EXPORT bool ValidBufferParameter(const Context *context, GLenum pname);
ANGLE_EXPORT bool ValidMipLevel(const Context *context, GLenum target, GLint level);
ANGLE_EXPORT bool ValidImageSize(const Context *context, GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth);
ANGLE_EXPORT bool ValidCompressedImageSize(const Context *context, GLenum internalFormat, GLsizei width, GLsizei height);
ANGLE_EXPORT bool ValidQueryType(const Context *context, GLenum queryType);
ANGLE_EXPORT bool ValidProgram(Context *context, GLuint id);

ANGLE_EXPORT bool ValidateAttachmentTarget(Context *context, GLenum attachment);
ANGLE_EXPORT bool ValidateRenderbufferStorageParametersBase(Context *context, GLenum target, GLsizei samples,
                                                            GLenum internalformat, GLsizei width, GLsizei height);
ANGLE_EXPORT bool ValidateRenderbufferStorageParametersANGLE(Context *context, GLenum target, GLsizei samples,
                                                             GLenum internalformat, GLsizei width, GLsizei height);

ANGLE_EXPORT bool ValidateFramebufferRenderbufferParameters(Context *context, GLenum target, GLenum attachment,
                                                            GLenum renderbuffertarget, GLuint renderbuffer);

ANGLE_EXPORT bool ValidateBlitFramebufferParameters(Context *context, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                                    GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                                                    GLenum filter, bool fromAngleExtension);

ANGLE_EXPORT bool ValidateGetVertexAttribParameters(Context *context, GLenum pname);

ANGLE_EXPORT bool ValidateTexParamParameters(Context *context, GLenum pname, GLint param);

ANGLE_EXPORT bool ValidateSamplerObjectParameter(Context *context, GLenum pname);

ANGLE_EXPORT bool ValidateReadPixelsParameters(Context *context, GLint x, GLint y, GLsizei width, GLsizei height,
                                               GLenum format, GLenum type, GLsizei *bufSize, GLvoid *pixels);

ANGLE_EXPORT bool ValidateBeginQuery(Context *context, GLenum target, GLuint id);
ANGLE_EXPORT bool ValidateEndQuery(Context *context, GLenum target);

ANGLE_EXPORT bool ValidateUniform(Context *context, GLenum uniformType, GLint location, GLsizei count);
ANGLE_EXPORT bool ValidateUniformMatrix(Context *context, GLenum matrixType, GLint location, GLsizei count,
                                        GLboolean transpose);

ANGLE_EXPORT bool ValidateStateQuery(Context *context, GLenum pname, GLenum *nativeType, unsigned int *numParams);

ANGLE_EXPORT bool ValidateCopyTexImageParametersBase(Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                                     GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                                     GLint border, GLenum *textureInternalFormatOut);

ANGLE_EXPORT bool ValidateDrawArrays(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount);
ANGLE_EXPORT bool ValidateDrawArraysInstanced(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount);
ANGLE_EXPORT bool ValidateDrawArraysInstancedANGLE(Context *context, GLenum mode, GLint first, GLsizei count, GLsizei primcount);

ANGLE_EXPORT bool ValidateDrawElements(Context *context, GLenum mode, GLsizei count, GLenum type,
                                       const GLvoid* indices, GLsizei primcount, rx::RangeUI *indexRangeOut);

ANGLE_EXPORT bool ValidateDrawElementsInstanced(Context *context, GLenum mode, GLsizei count, GLenum type,
                                                const GLvoid *indices, GLsizei primcount, rx::RangeUI *indexRangeOut);
ANGLE_EXPORT bool ValidateDrawElementsInstancedANGLE(Context *context, GLenum mode, GLsizei count, GLenum type,
                                                     const GLvoid *indices, GLsizei primcount, rx::RangeUI *indexRangeOut);

ANGLE_EXPORT bool ValidateFramebufferTextureBase(Context *context, GLenum target, GLenum attachment,
                                                 GLuint texture, GLint level);
ANGLE_EXPORT bool ValidateFramebufferTexture2D(Context *context, GLenum target, GLenum attachment,
                                               GLenum textarget, GLuint texture, GLint level);

ANGLE_EXPORT bool ValidateGetUniformBase(Context *context, GLuint program, GLint location);
ANGLE_EXPORT bool ValidateGetUniformfv(Context *context, GLuint program, GLint location, GLfloat* params);
ANGLE_EXPORT bool ValidateGetUniformiv(Context *context, GLuint program, GLint location, GLint* params);
ANGLE_EXPORT bool ValidateGetnUniformfvEXT(Context *context, GLuint program, GLint location, GLsizei bufSize, GLfloat* params);
ANGLE_EXPORT bool ValidateGetnUniformivEXT(Context *context, GLuint program, GLint location, GLsizei bufSize, GLint* params);

}

#endif // LIBANGLE_VALIDATION_ES_H_
