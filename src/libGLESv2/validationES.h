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

bool validateRenderbufferStorageParameters(const gl::Context *context, GLenum target, GLsizei samples,
                                           GLenum internalformat, GLsizei width, GLsizei height,
                                           bool angleExtension);

bool validateBlitFramebufferParameters(gl::Context *context, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask,
                                       GLenum filter, bool fromAngleExtension);

bool validateGetVertexAttribParameters(GLenum pname, int clientVersion);

bool validateTexParamParameters(gl::Context *context, GLenum pname, GLint param);

bool validateSamplerObjectParameter(GLenum pname);

}

#endif // LIBGLESV2_VALIDATION_ES_H
