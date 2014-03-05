//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES.h: Validation functions for generic OpenGL ES entry point parameters

#ifndef LIBGLESV2_VALIDATION_ES_H
#define LIBGLESV2_VALIDATION_ES_H

namespace gl
{

class Context;

bool ValidCap(const Context *context, GLenum cap);
bool ValidTextureTarget(const Context *context, GLenum target);
bool ValidFramebufferTarget(GLenum target);
bool ValidBufferTarget(const Context *context, GLenum target);
bool ValidMipLevel(const Context *context, GLenum target, GLint level);
bool ValidImageSize(const gl::Context *context, GLenum target, GLint level, GLsizei width, GLsizei height, GLsizei depth);
bool ValidCompressedImageSize(const gl::Context *context, GLenum internalFormat, GLsizei width, GLsizei height);
bool ValidQueryType(const gl::Context *context, GLenum queryType);

bool ValidateRenderbufferStorageParameters(const gl::Context *context, GLenum target, GLsizei samples,
                                           GLenum internalformat, GLsizei width, GLsizei height,
                                           bool angleExtension);
bool ValidateFramebufferRenderbufferParameters(gl::Context *context, GLenum target, GLenum attachment,
                                               GLenum renderbuffertarget, GLuint renderbuffer);

bool ValidateBlitFramebufferParameters(gl::Context *context, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                                       GLenum filter, bool fromAngleExtension);

bool ValidateGetVertexAttribParameters(GLenum pname, int clientVersion);

bool ValidateTexParamParameters(gl::Context *context, GLenum pname, GLint param);

bool ValidateSamplerObjectParameter(GLenum pname);

}

#endif // LIBGLESV2_VALIDATION_ES_H
