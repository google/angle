//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationEGL.h: Validation functions for generic EGL entry point parameters

#ifndef LIBANGLE_VALIDATIONEGL_H_
#define LIBANGLE_VALIDATIONEGL_H_

#include "libANGLE/Error.h"

namespace gl
{
class Context;
}

namespace egl
{

struct Config;
class Display;
class Surface;

// Object validation
Error ValidateDisplay(const Display *display);
Error ValidateSurface(const Display *display, Surface *surface);
Error ValidateConfig(const Display *display, const Config *config);
Error ValidateContext(const Display *display, gl::Context *context);

}

#endif // LIBANGLE_VALIDATIONEGL_H_
