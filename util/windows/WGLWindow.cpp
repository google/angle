//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// WGLWindow:
//   Implements initializing a WGL rendering context.
//

#include "util/windows/WGLWindow.h"

#include "common/debug.h"
#include "common/string_utils.h"
#include "util/OSWindow.h"
#include "util/system_utils.h"
#include "util/windows/win32/Win32Window.h"

#include <iostream>

namespace
{
PIXELFORMATDESCRIPTOR GetDefaultPixelFormatDescriptor()
{
    PIXELFORMATDESCRIPTOR pixelFormatDescriptor = {0};
    pixelFormatDescriptor.nSize                 = sizeof(pixelFormatDescriptor);
    pixelFormatDescriptor.nVersion              = 1;
    pixelFormatDescriptor.dwFlags =
        PFD_DRAW_TO_WINDOW | PFD_GENERIC_ACCELERATED | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pixelFormatDescriptor.iPixelType   = PFD_TYPE_RGBA;
    pixelFormatDescriptor.cColorBits   = 24;
    pixelFormatDescriptor.cAlphaBits   = 8;
    pixelFormatDescriptor.cDepthBits   = 24;
    pixelFormatDescriptor.cStencilBits = 8;
    pixelFormatDescriptor.iLayerType   = PFD_MAIN_PLANE;

    return pixelFormatDescriptor;
}

PFNWGLGETPROCADDRESSPROC gCurrentWGLGetProcAddress = nullptr;
HMODULE gCurrentModule                             = nullptr;

angle::GenericProc WINAPI GetProcAddressWithFallback(const char *name)
{
    ASSERT(GetLastError() == ERROR_SUCCESS);
    angle::GenericProc proc = reinterpret_cast<angle::GenericProc>(gCurrentWGLGetProcAddress(name));
    // ERROR_INVALID_HANDLE and ERROR_PROC_NOT_FOUND are expected from wglGetProcAddress,
    // reset last error if they happen.
    if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_INVALID_HANDLE &&
        GetLastError() != ERROR_PROC_NOT_FOUND)
    {
        std::cerr << "Unexpected error calling wglGetProcAddress: 0x" << std::hex << GetLastError()
                  << std::endl;
    }
    else
    {
        SetLastError(ERROR_SUCCESS);
    }

    if (proc)
    {
        return proc;
    }

    proc = reinterpret_cast<angle::GenericProc>(GetProcAddress(gCurrentModule, name));
    // ERROR_PROC_NOT_FOUND is expected from GetProcAddress, reset last error if it happens.
    if (GetLastError() != ERROR_SUCCESS && GetLastError() != ERROR_PROC_NOT_FOUND)
    {
        std::cerr << "Unexpected error calling GetProcAddress: 0x" << std::hex << GetLastError()
                  << std::endl;
    }
    else
    {
        SetLastError(ERROR_SUCCESS);
    }
    return proc;
}

bool HasExtension(const std::vector<std::string> &extensions, const char *ext)
{
    return std::find(extensions.begin(), extensions.end(), ext) != extensions.end();
}
}  // namespace

WGLWindow::WGLWindow(int glesMajorVersion, int glesMinorVersion)
    : GLWindowBase(glesMajorVersion, glesMinorVersion),
      mDeviceContext(nullptr),
      mWGLContext(nullptr),
      mWindow(nullptr)
{}

WGLWindow::~WGLWindow() {}

// Internally initializes GL resources.
bool WGLWindow::initializeGL(OSWindow *osWindow, angle::Library *glWindowingLibrary)
{
    glWindowingLibrary->getAs("wglGetProcAddress", &gCurrentWGLGetProcAddress);

    if (!gCurrentWGLGetProcAddress)
    {
        std::cerr << "Error loading wglGetProcAddress." << std::endl;
        return false;
    }

    gCurrentModule = reinterpret_cast<HMODULE>(glWindowingLibrary->getNative());
    angle::LoadWGL(GetProcAddressWithFallback);

    Win32Window *win32Window = static_cast<Win32Window *>(osWindow);
    if (!win32Window->setPixelFormat(GetDefaultPixelFormatDescriptor()))
    {
        std::cerr << "Failed to set default pixel format." << std::endl;
        return false;
    }

    mWindow        = osWindow->getNativeWindow();
    mDeviceContext = GetDC(mWindow);
    mWGLContext = _wglCreateContext(mDeviceContext);
    if (!mWGLContext)
    {
        std::cerr << "Failed to create a WGL context." << std::endl;
        return false;
    }

    makeCurrent();

    // Reload entry points to capture extensions.
    angle::LoadWGL(GetProcAddressWithFallback);

    if (!_wglGetExtensionsStringARB)
    {
        std::cerr << "Driver does not expose wglGetExtensionsStringARB." << std::endl;
        return false;
    }

    const char *extensionsString = _wglGetExtensionsStringARB(mDeviceContext);

    std::vector<std::string> extensions;
    angle::SplitStringAlongWhitespace(extensionsString, &extensions);

    if (!HasExtension(extensions, "WGL_EXT_create_context_es2_profile"))
    {
        std::cerr << "Driver does not expose WGL_EXT_create_context_es2_profile." << std::endl;
        return false;
    }

    if (mWebGLCompatibility.valid() || mRobustResourceInit.valid())
    {
        std::cerr << "WGLWindow does not support the requested feature set." << std::endl;
        return false;
    }

    // Tear down the context and create another with ES2 compatibility.
    _wglDeleteContext(mWGLContext);

    // This could be extended to cover ES1 compatiblity.
    int kCreateAttribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                            mClientMajorVersion,
                            WGL_CONTEXT_MINOR_VERSION_ARB,
                            mClientMinorVersion,
                            WGL_CONTEXT_PROFILE_MASK_ARB,
                            WGL_CONTEXT_ES2_PROFILE_BIT_EXT,
                            0,
                            0};

    mWGLContext = _wglCreateContextAttribsARB(mDeviceContext, nullptr, kCreateAttribs);
    if (!mWGLContext)
    {
        std::cerr << "Failed to create an ES2 compatible WGL context." << std::endl;
        return false;
    }

    makeCurrent();

    if (mSwapInterval != -1)
    {
        if (_wglSwapIntervalEXT)
        {
            if (_wglSwapIntervalEXT(mSwapInterval) == FALSE)
            {
                std::cerr << "Error setting swap interval." << std::endl;
            }
        }
        else
        {
            std::cerr << "Error setting swap interval." << std::endl;
        }
    }

    angle::LoadGLES(GetProcAddressWithFallback);
    return true;
}

void WGLWindow::destroyGL()
{
    if (mWGLContext)
    {
        _wglDeleteContext(mWGLContext);
        mWGLContext = nullptr;
    }

    if (mDeviceContext)
    {
        ReleaseDC(mWindow, mDeviceContext);
        mDeviceContext = nullptr;
    }
}

bool WGLWindow::isGLInitialized() const
{
    return mWGLContext != nullptr;
}

void WGLWindow::makeCurrent()
{
    if (_wglMakeCurrent(mDeviceContext, mWGLContext) == FALSE)
    {
        std::cerr << "Error during wglMakeCurrent." << std::endl;
    }
}

void WGLWindow::swap()
{
    if (SwapBuffers(mDeviceContext) == FALSE)
    {
        std::cerr << "Error during SwapBuffers." << std::endl;
    }
}

bool WGLWindow::hasError() const
{
    return GetLastError() != S_OK;
}

// static
WGLWindow *WGLWindow::New(int glesMajorVersion, int glesMinorVersion)
{
    return new WGLWindow(glesMajorVersion, glesMinorVersion);
}

// static
void WGLWindow::Delete(WGLWindow **window)
{
    delete *window;
    *window = nullptr;
}
