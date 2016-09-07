//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// queryutils.h: Utilities for querying values from GL objects

#ifndef LIBANGLE_QUERYUTILS_H_
#define LIBANGLE_QUERYUTILS_H_

#include "angle_gl.h"
#include "common/angleutils.h"

namespace gl
{
class Buffer;
class Framebuffer;
class Program;

void QueryFramebufferAttachmentParameteriv(const Framebuffer *framebuffer,
                                           GLenum attachment,
                                           GLenum pname,
                                           GLint *params);
void QueryBufferParameteriv(const Buffer *buffer, GLenum pname, GLint *params);
void QueryProgramiv(const Program *program, GLenum pname, GLint *params);
}

#endif  // LIBANGLE_QUERYUTILS_H_
