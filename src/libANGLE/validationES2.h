//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES2.h: Validation functions for OpenGL ES 2.0 entry point parameters

#ifndef LIBANGLE_VALIDATION_ES2_H_
#define LIBANGLE_VALIDATION_ES2_H_

#include "libANGLE/export.h"

#include <GLES2/gl2.h>

namespace gl
{

class Context;

ANGLE_EXPORT bool ValidateES2TexImageParameters(Context *context, GLenum target, GLint level, GLenum internalformat, bool isCompressed, bool isSubImage,
                                                GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                                GLint border, GLenum format, GLenum type, const GLvoid *pixels);

ANGLE_EXPORT bool ValidateES2CopyTexImageParameters(Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                                    GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                                    GLint border);

ANGLE_EXPORT bool ValidateES2TexStorageParameters(Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                                   GLsizei width, GLsizei height);

ANGLE_EXPORT bool ValidES2ReadFormatType(Context *context, GLenum format, GLenum type);

}

#endif // LIBANGLE_VALIDATION_ES2_H_
