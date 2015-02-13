//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FunctionsWGL.h: Implements the FuntionsWGL class.

#include "libANGLE/renderer/gl/wgl/FunctionsWGL.h"

namespace rx
{

typedef PROC(WINAPI *PFNWGLGETPROCADDRESSPROC)(LPCSTR);

template <typename T>
static void GetWGLProcAddress(HMODULE glModule, PFNWGLGETPROCADDRESSPROC getProcAddressWGL,
                              const std::string &procName, T *outProcAddress)
{
    T proc = nullptr;
    if (getProcAddressWGL)
    {
        proc = reinterpret_cast<T>(getProcAddressWGL(procName.c_str()));
    }

    if (!proc)
    {
        proc = reinterpret_cast<T>(GetProcAddress(glModule, procName.c_str()));
    }

    *outProcAddress = proc;
}

template <typename T>
static void GetWGLExtensionProcAddress(HMODULE glModule, PFNWGLGETPROCADDRESSPROC getProcAddressWGL,
                                       const std::string &extensions, const std::string &extensionName,
                                       const std::string &procName, T *outProcAddress)
{
    T proc = nullptr;
    if (extensions.find(extensionName) != std::string::npos)
    {
        GetWGLProcAddress(glModule, getProcAddressWGL, procName, &proc);
    }

    *outProcAddress = proc;
}

FunctionsWGL::FunctionsWGL()
    : createContext(nullptr),
      deleteContext(nullptr),
      makeCurrent(nullptr),
      createContextAttribsARB(nullptr),
      getPixelFormatAttribivARB(nullptr),
      swapIntervalEXT(nullptr)
{
}

void FunctionsWGL::intialize(HMODULE glModule, HDC context)
{
    // First grab the wglGetProcAddress function from the gl module
    PFNWGLGETPROCADDRESSPROC getProcAddressWGL = nullptr;
    GetWGLProcAddress(glModule, nullptr, "wglGetProcAddress", &getProcAddressWGL);

    // Load the core wgl functions
    GetWGLProcAddress(glModule, getProcAddressWGL, "wglCreateContext", &createContext);
    GetWGLProcAddress(glModule, getProcAddressWGL, "wglDeleteContext", &deleteContext);
    GetWGLProcAddress(glModule, getProcAddressWGL, "wglMakeCurrent", &makeCurrent);

    // Load extension string getter functions
    PFNWGLGETEXTENSIONSSTRINGEXTPROC getExtensionStringEXT = nullptr;
    GetWGLProcAddress(glModule, getProcAddressWGL, "wglGetExtensionsStringEXT", &getExtensionStringEXT);

    PFNWGLGETEXTENSIONSSTRINGARBPROC getExtensionStringARB = nullptr;
    GetWGLProcAddress(glModule, getProcAddressWGL, "wglGetExtensionsStringARB", &getExtensionStringARB);

    std::string extensions = "";
    if (getExtensionStringEXT)
    {
        extensions = getExtensionStringEXT();
    }
    else if (getExtensionStringARB && context)
    {
        extensions = getExtensionStringARB(context);
    }

    // Load the wgl extension functions by checking if the context supports the extension first
    GetWGLExtensionProcAddress(glModule, getProcAddressWGL, extensions, "WGL_ARB_create_context", "wglCreateContextAttribsARB", &createContextAttribsARB);
    GetWGLExtensionProcAddress(glModule, getProcAddressWGL, extensions, "WGL_ARB_pixel_format", "wglGetPixelFormatAttribivARB", &getPixelFormatAttribivARB);
    GetWGLExtensionProcAddress(glModule, getProcAddressWGL, extensions, "WGL_EXT_swap_control", "wglSwapIntervalEXT", &swapIntervalEXT);
}

}
