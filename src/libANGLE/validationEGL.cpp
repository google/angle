//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationEGL.cpp: Validation functions for generic EGL entry point parameters

#include "libANGLE/validationEGL.h"

#include "libANGLE/Config.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"

namespace egl
{

Error ValidateDisplay(const Display *display)
{
    if (display == EGL_NO_DISPLAY)
    {
        return Error(EGL_BAD_DISPLAY);
    }

    if (!display->isInitialized())
    {
        return Error(EGL_NOT_INITIALIZED);
    }

    return Error(EGL_SUCCESS);
}

Error ValidateSurface(const Display *display, Surface *surface)
{
    Error error = ValidateDisplay(display);
    if (error.isError())
    {
        return error;
    }

    if (!display->isValidSurface(surface))
    {
        return Error(EGL_BAD_SURFACE);
    }

    return Error(EGL_SUCCESS);
}

Error ValidateConfig(const Display *display, const Config *config)
{
    Error error = ValidateDisplay(display);
    if (error.isError())
    {
        return error;
    }

    if (!display->isValidConfig(config))
    {
        return Error(EGL_BAD_CONFIG);
    }

    return Error(EGL_SUCCESS);
}

Error ValidateContext(const Display *display, gl::Context *context)
{
    Error error = ValidateDisplay(display);
    if (error.isError())
    {
        return error;
    }

    if (!display->isValidContext(context))
    {
        return Error(EGL_BAD_CONTEXT);
    }

    return Error(EGL_SUCCESS);
}

}
