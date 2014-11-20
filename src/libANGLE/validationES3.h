//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES3.h: Validation functions for OpenGL ES 3.0 entry point parameters

#ifndef LIBANGLE_VALIDATION_ES3_H_
#define LIBANGLE_VALIDATION_ES3_H_

#include "libANGLE/export.h"

#include <GLES3/gl3.h>

namespace gl
{

class Context;

ANGLE_EXPORT bool ValidateES3TexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat, bool isCompressed, bool isSubImage,
                                                GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                                GLint border, GLenum format, GLenum type, const GLvoid *pixels);

ANGLE_EXPORT bool ValidateES3CopyTexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat,
                                                    bool isSubImage, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y,
                                                    GLsizei width, GLsizei height, GLint border);

ANGLE_EXPORT bool ValidateES3TexStorageParameters(Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                                  GLsizei width, GLsizei height, GLsizei depth);

ANGLE_EXPORT bool ValidateFramebufferTextureLayer(Context *context, GLenum target, GLenum attachment,
                                                  GLuint texture, GLint level, GLint layer);

ANGLE_EXPORT bool ValidES3ReadFormatType(Context *context, GLenum internalFormat, GLenum format, GLenum type);

ANGLE_EXPORT bool ValidateES3RenderbufferStorageParameters(Context *context, GLenum target, GLsizei samples,
                                                           GLenum internalformat, GLsizei width, GLsizei height);

ANGLE_EXPORT bool ValidateInvalidateFramebufferParameters(Context *context, GLenum target, GLsizei numAttachments,
                                             const GLenum* attachments);

ANGLE_EXPORT bool ValidateClearBuffer(Context *context);

ANGLE_EXPORT bool ValidateGetUniformuiv(Context *context, GLuint program, GLint location, GLuint* params);

}

#endif // LIBANGLE_VALIDATION_ES3_H_
