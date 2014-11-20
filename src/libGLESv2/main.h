//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// main.h: Management of thread-local data.

#ifndef LIBGLESV2_MAIN_H_
#define LIBGLESV2_MAIN_H_

#include <GLES2/gl2.h>
#include <EGL/egl.h>

namespace egl
{
class Display;
class Surface;
class AttributeMap;
}

namespace gl
{
class Context;

void makeCurrent(Context *context, egl::Display *display, egl::Surface *surface);

Context *getContext();
Context *getNonLostContext();
egl::Display *getDisplay();

}

extern "C"
{
// Exported functions for use by EGL
void glMakeCurrent(gl::Context *context, egl::Display *display, egl::Surface *surface);
gl::Context *glGetCurrentContext();

}

#endif   // LIBGLESV2_MAIN_H_
