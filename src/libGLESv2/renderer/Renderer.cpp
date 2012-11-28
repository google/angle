//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.cpp: Implements EGL dependencies for creating and destroying Renderer instances.

#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/Renderer9.h"
#include "libGLESv2/renderer/Renderer11.h"

#if !defined(ANGLE_ENABLE_D3D11)
// Enables use of the Direct3D 11 API, when available
#define ANGLE_ENABLE_D3D11 0
#endif

extern "C"
{

rx::Renderer *glCreateRenderer(egl::Display *display, HDC hDc, bool softwareDevice)
{
    rx::Renderer *renderer = NULL;
    EGLint status = EGL_BAD_ALLOC;
    
    #if ANGLE_ENABLE_D3D11
        renderer = new rx::Renderer11(display, hDc);
    
        if (renderer)
        {
            status = renderer->initialize();
        }

        if (status == EGL_SUCCESS)
        {
            return renderer;
        }

        // Failed to create a D3D11 renderer, try creating a D3D9 renderer
        delete renderer;
    #endif

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