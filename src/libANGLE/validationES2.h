//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// validationES2.h:
//  Inlined validation functions for OpenGL ES 2.0 entry points.

#ifndef LIBANGLE_VALIDATION_ES2_H_
#define LIBANGLE_VALIDATION_ES2_H_

#include "libANGLE/ErrorStrings.h"
#include "libANGLE/validationES.h"
#include "libANGLE/validationES2_autogen.h"

namespace gl
{
ANGLE_INLINE bool ValidateDrawArrays(Context *context,
                                     PrimitiveMode mode,
                                     GLint first,
                                     GLsizei count)
{
    return ValidateDrawArraysCommon(context, mode, first, count, 1);
}

ANGLE_INLINE bool ValidateUniform2f(Context *context, GLint location, GLfloat x, GLfloat y)
{
    return ValidateUniform(context, GL_FLOAT_VEC2, location, 1);
}

ANGLE_INLINE bool ValidateBindBuffer(Context *context, BufferBinding target, GLuint buffer)
{
    if (!context->isValidBufferBinding(target))
    {
        context->validationError(GL_INVALID_ENUM, kErrorInvalidBufferTypes);
        return false;
    }

    if (!context->getGLState().isBindGeneratesResourceEnabled() &&
        !context->isBufferGenerated(buffer))
    {
        context->validationError(GL_INVALID_OPERATION, kErrorObjectNotGenerated);
        return false;
    }

    return true;
}
}  // namespace gl

#endif  // LIBANGLE_VALIDATION_ES2_H_
