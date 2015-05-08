//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FunctionsGLX.cpp: Implements the FunctionsGLX class.

#include "libANGLE/renderer/gl/glx/FunctionsGLX.h"

#include <dlfcn.h>
#include <algorithm>

#include "libANGLE/Error.h"
#include "libANGLE/renderer/gl/renderergl_utils.h"

namespace rx
{

template<typename T>
static bool GetProc(PFNGLXGETPROCADDRESSPROC getProc, T *member, const char *name)
{
    *member = reinterpret_cast<T>(getProc(reinterpret_cast<const GLubyte*>(name)));
    return *member != nullptr;
}

FunctionsGLX::FunctionsGLX()
  : majorVersion(0),
    minorVersion(0),
    getProc(nullptr),
    destroyContext(nullptr),
    makeCurrent(nullptr),
    swapBuffers(nullptr),
    queryExtension(nullptr),
    queryVersion(nullptr),
    queryExtensionsString(nullptr),
    getFBConfigs(nullptr),
    chooseFBConfig(nullptr),
    getFBConfigAttrib(nullptr),
    getVisualFromFBConfig(nullptr),
    createWindow(nullptr),
    destroyWindow(nullptr),
    createPbuffer(nullptr),
    destroyPbuffer(nullptr),
    createContextAttribsARB(nullptr),
    mLibHandle(nullptr)
{
}

FunctionsGLX::~FunctionsGLX()
{
    terminate();
}

egl::Error FunctionsGLX::initialize(Display *xDisplay)
{
    terminate();

    mLibHandle = dlopen("libGL.so.1", RTLD_NOW);
    if (!mLibHandle)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Could not dlopen libGL.so.1: %s", dlerror());
    }

    getProc = reinterpret_cast<PFNGLXGETPROCADDRESSPROC>(dlsym(mLibHandle, "glXGetProcAddress"));
    if (!getProc)
    {
        getProc = reinterpret_cast<PFNGLXGETPROCADDRESSPROC>(dlsym(mLibHandle, "glXGetProcAddressARB"));
    }
    if (!getProc)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Could not retrieve glXGetProcAddress");
    }

#define GET_PROC_OR_ERROR(MEMBER, NAME) \
    if (!GetProc(getProc, MEMBER, NAME)) \
    { \
        return egl::Error(EGL_NOT_INITIALIZED, "Could not load GLX entry point " NAME); \
    }

    // GLX 1.0
    GET_PROC_OR_ERROR(&destroyContext, "glXDestroyContext");
    GET_PROC_OR_ERROR(&makeCurrent, "glXMakeCurrent");
    GET_PROC_OR_ERROR(&swapBuffers, "glXSwapBuffers");
    GET_PROC_OR_ERROR(&queryExtension, "glXQueryExtension");
    GET_PROC_OR_ERROR(&queryVersion, "glXQueryVersion");

    // GLX 1.1
    GET_PROC_OR_ERROR(&queryExtensionsString, "glXQueryExtensionsString");

    // Check we have a working GLX
    {
        int errorBase;
        int eventBase;
        if (!queryExtension(xDisplay, &errorBase, &eventBase))
        {
            return egl::Error(EGL_NOT_INITIALIZED, "GLX is not present.");
        }
    }

    // Check we have a supported version of GLX
    if (!queryVersion(xDisplay, &majorVersion, &minorVersion))
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Could not query the GLX version.");
    }
    if (majorVersion != 1 || minorVersion < 3)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Unsupported GLX version (requires at least 1.3).");
    }

    const char *extensions = queryExtensionsString(xDisplay, DefaultScreen(xDisplay));
    if (!extensions)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "glXQueryExtensionsString returned NULL");
    }
    mExtensions = TokenizeExtensionsString(extensions);

    // GLX 1.3
    GET_PROC_OR_ERROR(&getFBConfigs, "glXGetFBConfigs");
    GET_PROC_OR_ERROR(&chooseFBConfig, "glXChooseFBConfig");
    GET_PROC_OR_ERROR(&getFBConfigAttrib, "glXGetFBConfigAttrib");
    GET_PROC_OR_ERROR(&getVisualFromFBConfig, "glXGetVisualFromFBConfig");
    GET_PROC_OR_ERROR(&createWindow, "glXCreateWindow");
    GET_PROC_OR_ERROR(&destroyWindow, "glXDestroyWindow");
    GET_PROC_OR_ERROR(&createPbuffer, "glXCreatePbuffer");
    GET_PROC_OR_ERROR(&destroyPbuffer, "glXDestroyPbuffer");

    // Extensions
    if (hasExtension("GLX_ARB_create_context"))
    {
        GET_PROC_OR_ERROR(&createContextAttribsARB, "glXCreateContextAttribsARB");
    }
    else
    {
        createContextAttribsARB = nullptr;
    }

#undef GET_PROC_OR_ERROR

    return egl::Error(EGL_SUCCESS);
}

void FunctionsGLX::terminate()
{
    if (mLibHandle)
    {
        dlclose(mLibHandle);
        mLibHandle = nullptr;
    }
}

bool FunctionsGLX::hasExtension(const char *extension) const
{
    return std::find(mExtensions.begin(), mExtensions.end(), extension) != mExtensions.end();
}

}
