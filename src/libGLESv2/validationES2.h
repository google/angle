//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES2.h: Validation functions for OpenGL ES 2.0 entry point parameters

#ifndef LIBGLESV2_VALIDATION_ES2_H
#define LIBGLESV2_VALIDATION_ES2_H

namespace gl
{

class Context;

bool validateES2TexImageParameters(gl::Context *context, GLenum target, GLint level, GLint internalformat, bool isCompressed, bool isSubImage,
                                   GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                   GLint border, GLenum format, GLenum type, const GLvoid *pixels);

bool validateES2CopyTexImageParameters(gl::Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                       GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                       GLint border);

bool validateES2TexStorageParameters(gl::Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                     GLsizei width, GLsizei height);

bool validateES2FramebufferTextureParameters(gl::Context *context, GLenum target, GLenum attachment,
                                             GLenum textarget, GLuint texture, GLint level);

bool validES2ReadFormatType(GLenum format, GLenum type);

}

#endif // LIBGLESV2_VALIDATION_ES2_H
