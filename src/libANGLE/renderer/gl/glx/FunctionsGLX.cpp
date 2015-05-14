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
    mLibHandle(nullptr),
    mXDisplay(nullptr),
    mXScreen(-1),
    mDestroyContextPtr(nullptr),
    mMakeCurrentPtr(nullptr),
    mSwapBuffersPtr(nullptr),
    mQueryExtensionPtr(nullptr),
    mQueryVersionPtr(nullptr),
    mWaitXPtr(nullptr),
    mWaitGLPtr(nullptr),
    mQueryExtensionsStringPtr(nullptr),
    mGetFBConfigsPtr(nullptr),
    mChooseFBConfigPtr(nullptr),
    mGetFBConfigAttribPtr(nullptr),
    mGetVisualFromFBConfigPtr(nullptr),
    mCreateWindowPtr(nullptr),
    mDestroyWindowPtr(nullptr),
    mCreatePbufferPtr(nullptr),
    mDestroyPbufferPtr(nullptr),
    mQueryDrawablePtr(nullptr),
    mCreateContextAttribsARBPtr(nullptr)
{
}

FunctionsGLX::~FunctionsGLX()
{
    terminate();
}

egl::Error FunctionsGLX::initialize(Display *xDisplay, int screen)
{
    terminate();
    mXDisplay = xDisplay;
    mXScreen = screen;

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
    GET_PROC_OR_ERROR(&mDestroyContextPtr, "glXDestroyContext");
    GET_PROC_OR_ERROR(&mMakeCurrentPtr, "glXMakeCurrent");
    GET_PROC_OR_ERROR(&mSwapBuffersPtr, "glXSwapBuffers");
    GET_PROC_OR_ERROR(&mQueryExtensionPtr, "glXQueryExtension");
    GET_PROC_OR_ERROR(&mQueryVersionPtr, "glXQueryVersion");
    GET_PROC_OR_ERROR(&mWaitXPtr, "glXWaitX");
    GET_PROC_OR_ERROR(&mWaitGLPtr, "glXWaitGL");

    // GLX 1.1
    GET_PROC_OR_ERROR(&mQueryExtensionsStringPtr, "glXQueryExtensionsString");

    // Check we have a working GLX
    {
        int errorBase;
        int eventBase;
        if (!queryExtension(&errorBase, &eventBase))
        {
            return egl::Error(EGL_NOT_INITIALIZED, "GLX is not present.");
        }
    }

    // Check we have a supported version of GLX
    if (!queryVersion(&majorVersion, &minorVersion))
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Could not query the GLX version.");
    }
    if (majorVersion != 1 || minorVersion < 3)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Unsupported GLX version (requires at least 1.3).");
    }

    const char *extensions = queryExtensionsString();
    if (!extensions)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "glXQueryExtensionsString returned NULL");
    }
    mExtensions = TokenizeExtensionsString(extensions);

    // GLX 1.3
    GET_PROC_OR_ERROR(&mGetFBConfigsPtr, "glXGetFBConfigs");
    GET_PROC_OR_ERROR(&mChooseFBConfigPtr, "glXChooseFBConfig");
    GET_PROC_OR_ERROR(&mGetFBConfigAttribPtr, "glXGetFBConfigAttrib");
    GET_PROC_OR_ERROR(&mGetVisualFromFBConfigPtr, "glXGetVisualFromFBConfig");
    GET_PROC_OR_ERROR(&mCreateWindowPtr, "glXCreateWindow");
    GET_PROC_OR_ERROR(&mDestroyWindowPtr, "glXDestroyWindow");
    GET_PROC_OR_ERROR(&mCreatePbufferPtr, "glXCreatePbuffer");
    GET_PROC_OR_ERROR(&mDestroyPbufferPtr, "glXDestroyPbuffer");
    GET_PROC_OR_ERROR(&mQueryDrawablePtr, "glXQueryDrawable");

    // Extensions
    if (hasExtension("GLX_ARB_create_context"))
    {
        GET_PROC_OR_ERROR(&mCreateContextAttribsARBPtr, "glXCreateContextAttribsARB");
    }
    else
    {
        mCreateContextAttribsARBPtr = nullptr;
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

Display *FunctionsGLX::getDisplay() const
{
    return mXDisplay;
}

int FunctionsGLX::getScreen() const
{
    return mXScreen;
}

// GLX functions

// GLX 1.0
void FunctionsGLX::destroyContext(GLXContext context) const
{
    mDestroyContextPtr(mXDisplay, context);
}
Bool FunctionsGLX::makeCurrent(GLXDrawable drawable, GLXContext context) const
{
    return mMakeCurrentPtr(mXDisplay, drawable, context);
}
void FunctionsGLX::swapBuffers(GLXDrawable drawable) const
{
    mSwapBuffersPtr(mXDisplay, drawable);
}
Bool FunctionsGLX::queryExtension(int *errorBase, int *event) const
{
    return mQueryExtensionPtr(mXDisplay, errorBase, event);
}
Bool FunctionsGLX::queryVersion(int *major, int *minor) const
{
    return mQueryVersionPtr(mXDisplay, major, minor);
}
void FunctionsGLX::waitX() const
{
    mWaitXPtr();
}
void FunctionsGLX::waitGL() const
{
    mWaitGLPtr();
}

// GLX 1.1
const char *FunctionsGLX::queryExtensionsString() const
{
    return mQueryExtensionsStringPtr(mXDisplay, mXScreen);
}

// GLX 1.4
GLXFBConfig *FunctionsGLX::getFBConfigs(int *nElements) const
{
    return mGetFBConfigsPtr(mXDisplay, mXScreen, nElements);
}
GLXFBConfig *FunctionsGLX::chooseFBConfig(const int *attribList, int *nElements) const
{
    return mChooseFBConfigPtr(mXDisplay, mXScreen, attribList, nElements);
}
int FunctionsGLX::getFBConfigAttrib(GLXFBConfig config, int attribute, int *value) const
{
    return mGetFBConfigAttribPtr(mXDisplay, config, attribute, value);
}
XVisualInfo *FunctionsGLX::getVisualFromFBConfig(GLXFBConfig config) const
{
    return mGetVisualFromFBConfigPtr(mXDisplay, config);
}
GLXWindow FunctionsGLX::createWindow(GLXFBConfig config, Window window, const int *attribList) const
{
    return mCreateWindowPtr(mXDisplay, config, window, attribList);
}
void FunctionsGLX::destroyWindow(GLXWindow window) const
{
    mDestroyWindowPtr(mXDisplay, window);
}
GLXPbuffer FunctionsGLX::createPbuffer(GLXFBConfig config, const int *attribList) const
{
    return mCreatePbufferPtr(mXDisplay, config, attribList);
}
void FunctionsGLX::destroyPbuffer(GLXPbuffer pbuffer) const
{
    mDestroyPbufferPtr(mXDisplay, pbuffer);
}
void FunctionsGLX::queryDrawable(GLXDrawable drawable, int attribute, unsigned int *value) const
{
    mQueryDrawablePtr(mXDisplay, drawable, attribute, value);
}

// GLX_ARB_create_context
GLXContext FunctionsGLX::createContextAttribsARB(GLXFBConfig config, GLXContext shareContext, Bool direct, const int *attribList) const
{
    return mCreateContextAttribsARBPtr(mXDisplay, config, shareContext, direct, attribList);
}

}
