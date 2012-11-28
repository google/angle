//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.cpp: Implements EGL dependencies for creating and destroying Renderer instances.

#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/Renderer9.h"

extern "C"
{

rx::Renderer *glCreateRenderer(egl::Display *display, HDC hDc, bool softwareDevice)
{
    return new rx::Renderer9(display, hDc, softwareDevice);
}

void glDestroyRenderer(rx::Renderer *renderer)
{
    delete renderer;
}

}