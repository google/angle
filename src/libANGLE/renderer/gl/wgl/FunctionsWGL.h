//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FunctionsWGL.h: Defines the FuntionsWGL class to contain loaded WGL functions

#include "common/angleutils.h"
#include "libANGLE/renderer/gl/wgl/functionswgl_typedefs.h"

namespace rx
{

class FunctionsWGL
{
  public:
    FunctionsWGL();

    // Loads all available wgl functions, may be called multiple times
    void intialize(HMODULE glModule, HDC context);

    // Base WGL functions
    PFNWGLCOPYCONTEXTPROC copyContext;
    PFNWGLCREATECONTEXTPROC createContext;
    PFNWGLCREATELAYERCONTEXTPROC createLayerContext;
    PFNWGLDELETECONTEXTPROC deleteContext;
    PFNWGLGETCURRENTCONTEXTPROC getCurrentContext;
    PFNWGLGETCURRENTDCPROC getCurrentDC;
    PFNWGLGETPROCADDRESSPROC getProcAddress;
    PFNWGLMAKECURRENTPROC makeCurrent;
    PFNWGLSHARELISTSPROC shareLists;
    PFNWGLUSEFONTBITMAPSAPROC useFontBitmapsA;
    PFNWGLUSEFONTBITMAPSWPROC useFontBitmapsW;
    PFNSWAPBUFFERSPROC swapBuffers;
    PFNWGLUSEFONTOUTLINESAPROC useFontOutlinesA;
    PFNWGLUSEFONTOUTLINESWPROC useFontOutlinesW;
    PFNWGLDESCRIBELAYERPLANEPROC describeLayerPlane;
    PFNWGLSETLAYERPALETTEENTRIESPROC setLayerPaletteEntries;
    PFNWGLGETLAYERPALETTEENTRIESPROC getLayerPaletteEntries;
    PFNWGLREALIZELAYERPALETTEPROC realizeLayerPalette;
    PFNWGLSWAPLAYERBUFFERSPROC swapLayerBuffers;
    PFNWGLSWAPMULTIPLEBUFFERSPROC swapMultipleBuffers;

    // Extension functions, may be NULL
    PFNWGLCREATECONTEXTATTRIBSARBPROC createContextAttribsARB;
    PFNWGLGETPIXELFORMATATTRIBIVARBPROC getPixelFormatAttribivARB;
    PFNWGLGETEXTENSIONSSTRINGEXTPROC getExtensionStringEXT;
    PFNWGLGETEXTENSIONSSTRINGARBPROC getExtensionStringARB;
    PFNWGLSWAPINTERVALEXTPROC swapIntervalEXT;

  private:
    DISALLOW_COPY_AND_ASSIGN(FunctionsWGL);
};

}
