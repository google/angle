//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// validationESEXT.cpp: Validation functions for OpenGL ES extension entry points.

#include "libANGLE/validationESEXT_autogen.h"

#include "libANGLE/Context.h"

namespace gl
{
bool ValidateGetTexImageANGLE(Context *context,
                              GLenum target,
                              GLint level,
                              GLenum format,
                              GLenum type,
                              void *pixels)
{
    return false;
}
bool ValidateGetRenderbufferImageANGLE(Context *context,
                                       GLenum target,
                                       GLint level,
                                       GLenum format,
                                       GLenum type,
                                       void *pixels)
{
    return false;
}
}  // namespace gl
