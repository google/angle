//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FunctionsWGL.h: Defines the FuntionsWGL class to contain loaded WGL functions

#include "common/debug.h"
#include "common/platform.h"
#include "libANGLE/Error.h"

#include <GL/wglext.h>

namespace rx
{

typedef HGLRC(WINAPI *PFNWGLCREATECONTEXTPROC)(HDC);
typedef BOOL(WINAPI *PFNWGLDELETECONTEXTPROC)(HGLRC);
typedef BOOL(WINAPI *PFNWGLMAKECURRENTPROC)(HDC, HGLRC);

class FunctionsWGL
{
  public:
    FunctionsWGL();

    // Loads all available wgl functions, may be called multiple times
    void intialize(HMODULE glModule, HDC context);

    // Base WGL functions
    PFNWGLCREATECONTEXTPROC createContext;
    PFNWGLDELETECONTEXTPROC deleteContext;
    PFNWGLMAKECURRENTPROC makeCurrent;

    // Extension functions, may be NULL
    PFNWGLCREATECONTEXTATTRIBSARBPROC createContextAttribsARB;
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC getPixelFormatAttribivARB;
    PFNWGLSWAPINTERVALEXTPROC swapIntervalEXT;

  private:
    DISALLOW_COPY_AND_ASSIGN(FunctionsWGL);
};

}
