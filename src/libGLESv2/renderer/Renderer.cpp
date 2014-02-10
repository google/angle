#include "precompiled.h"
//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.cpp: Implements EGL dependencies for creating and destroying Renderer instances.

#include <EGL/eglext.h>
#include "libGLESv2/main.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/d3d9/Renderer9.h"
#include "libGLESv2/renderer/d3d11/Renderer11.h"
#include "common/utilities.h"
#include "third_party/trace_event/trace_event.h"

#if !defined(ANGLE_ENABLE_D3D11)
// Enables use of the Direct3D 11 API for a default display, when available
#define ANGLE_ENABLE_D3D11 1
#endif

namespace rx
{

Renderer::Renderer(egl::Display *display) : mDisplay(display)
{
    mCurrentClientVersion = 2;
}

}

extern "C"
{

rx::Renderer *glCreateRenderer(egl::Display *display, HDC hDc, EGLNativeDisplayType displayId)
{
    rx::Renderer *renderer = NULL;
    EGLint status = EGL_BAD_ALLOC;
    
    if (ANGLE_ENABLE_D3D11 ||
        displayId == EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE ||
        displayId == EGL_D3D11_ONLY_DISPLAY_ANGLE)
    {
        renderer = new rx::Renderer11(display, hDc);
    
        if (renderer)
        {
            status = renderer->initialize();
        }

        if (status == EGL_SUCCESS)
        {
            return renderer;
        }
        else if (displayId == EGL_D3D11_ONLY_DISPLAY_ANGLE)
        {
            return NULL;
        }

        // Failed to create a D3D11 renderer, try creating a D3D9 renderer
        delete renderer;
    }

    bool softwareDevice = (displayId == EGL_SOFTWARE_DISPLAY_ANGLE);
    renderer = new rx::Renderer9(display, hDc, softwareDevice);
    
    if (renderer)
    {
        status = renderer->initialize();
    }

    if (status == EGL_SUCCESS)
    {
        return renderer;
    }

    return NULL;
}

void glDestroyRenderer(rx::Renderer *renderer)
{
    delete renderer;
}

}