//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FunctionsGLX.cpp: Implements the FunctionsGLX class.

#define ANGLE_SKIP_GLX_DEFINES 1
#include "libANGLE/renderer/gl/glx/FunctionsGLX.h"
#undef ANGLE_SKIP_GLX_DEFINES

// HACK(cwallez) this is a horrible hack to prevent glx from including GL/glext.h
// as it causes a bunch of conflicts (macro redefinition, etc) with GLES2/gl2ext.h
#define __glext_h_ 1
#include <GL/glx.h>
#undef __glext_h_

#include <dlfcn.h>
#include <algorithm>

#include "libANGLE/renderer/gl/renderergl_utils.h"
#include "libANGLE/renderer/gl/glx/functionsglx_typedefs.h"

namespace rx
{

template<typename T>
static bool GetProc(PFNGETPROCPROC getProc, T *member, const char *name)
{
    *member = reinterpret_cast<T>(getProc(name));
    return *member != nullptr;
}

struct FunctionsGLX::GLXFunctionTable
{
    GLXFunctionTable()
      : destroyContextPtr(nullptr),
        makeCurrentPtr(nullptr),
        swapBuffersPtr(nullptr),
        queryExtensionPtr(nullptr),
        queryVersionPtr(nullptr),
        waitXPtr(nullptr),
        waitGLPtr(nullptr),
        queryExtensionsStringPtr(nullptr),
        getFBConfigsPtr(nullptr),
        chooseFBConfigPtr(nullptr),
        getFBConfigAttribPtr(nullptr),
        getVisualFromFBConfigPtr(nullptr),
        createWindowPtr(nullptr),
        destroyWindowPtr(nullptr),
        createPbufferPtr(nullptr),
        destroyPbufferPtr(nullptr),
        queryDrawablePtr(nullptr),
        createContextAttribsARBPtr(nullptr)
    {
    }

    // GLX 1.0
    PFNGLXDESTROYCONTEXTPROC destroyContextPtr;
    PFNGLXMAKECURRENTPROC makeCurrentPtr;
    PFNGLXSWAPBUFFERSPROC swapBuffersPtr;
    PFNGLXQUERYEXTENSIONPROC queryExtensionPtr;
    PFNGLXQUERYVERSIONPROC queryVersionPtr;
    PFNGLXWAITXPROC waitXPtr;
    PFNGLXWAITGLPROC waitGLPtr;

    // GLX 1.1
    PFNGLXQUERYEXTENSIONSSTRINGPROC queryExtensionsStringPtr;

    //GLX 1.3
    PFNGLXGETFBCONFIGSPROC getFBConfigsPtr;
    PFNGLXCHOOSEFBCONFIGPROC chooseFBConfigPtr;
    PFNGLXGETFBCONFIGATTRIBPROC getFBConfigAttribPtr;
    PFNGLXGETVISUALFROMFBCONFIGPROC getVisualFromFBConfigPtr;
    PFNGLXCREATEWINDOWPROC createWindowPtr;
    PFNGLXDESTROYWINDOWPROC destroyWindowPtr;
    PFNGLXCREATEPBUFFERPROC createPbufferPtr;
    PFNGLXDESTROYPBUFFERPROC destroyPbufferPtr;
    PFNGLXQUERYDRAWABLEPROC queryDrawablePtr;

    // GLX_ARB_create_context
    PFNGLXCREATECONTEXTATTRIBSARBPROC createContextAttribsARBPtr;
};

FunctionsGLX::FunctionsGLX()
  : majorVersion(0),
    minorVersion(0),
    mLibHandle(nullptr),
    mXDisplay(nullptr),
    mXScreen(-1),
    mFnPtrs(new GLXFunctionTable())
{
}

FunctionsGLX::~FunctionsGLX()
{
    SafeDelete(mFnPtrs);
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

    getProc = reinterpret_cast<PFNGETPROCPROC>(dlsym(mLibHandle, "glXGetProcAddress"));
    if (!getProc)
    {
        getProc = reinterpret_cast<PFNGETPROCPROC>(dlsym(mLibHandle, "glXGetProcAddressARB"));
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
    GET_PROC_OR_ERROR(&mFnPtrs->destroyContextPtr, "glXDestroyContext");
    GET_PROC_OR_ERROR(&mFnPtrs->makeCurrentPtr, "glXMakeCurrent");
    GET_PROC_OR_ERROR(&mFnPtrs->swapBuffersPtr, "glXSwapBuffers");
    GET_PROC_OR_ERROR(&mFnPtrs->queryExtensionPtr, "glXQueryExtension");
    GET_PROC_OR_ERROR(&mFnPtrs->queryVersionPtr, "glXQueryVersion");
    GET_PROC_OR_ERROR(&mFnPtrs->waitXPtr, "glXWaitX");
    GET_PROC_OR_ERROR(&mFnPtrs->waitGLPtr, "glXWaitGL");

    // GLX 1.1
    GET_PROC_OR_ERROR(&mFnPtrs->queryExtensionsStringPtr, "glXQueryExtensionsString");

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
    GET_PROC_OR_ERROR(&mFnPtrs->getFBConfigsPtr, "glXGetFBConfigs");
    GET_PROC_OR_ERROR(&mFnPtrs->chooseFBConfigPtr, "glXChooseFBConfig");
    GET_PROC_OR_ERROR(&mFnPtrs->getFBConfigAttribPtr, "glXGetFBConfigAttrib");
    GET_PROC_OR_ERROR(&mFnPtrs->getVisualFromFBConfigPtr, "glXGetVisualFromFBConfig");
    GET_PROC_OR_ERROR(&mFnPtrs->createWindowPtr, "glXCreateWindow");
    GET_PROC_OR_ERROR(&mFnPtrs->destroyWindowPtr, "glXDestroyWindow");
    GET_PROC_OR_ERROR(&mFnPtrs->createPbufferPtr, "glXCreatePbuffer");
    GET_PROC_OR_ERROR(&mFnPtrs->destroyPbufferPtr, "glXDestroyPbuffer");
    GET_PROC_OR_ERROR(&mFnPtrs->queryDrawablePtr, "glXQueryDrawable");

    // Extensions
    if (hasExtension("GLX_ARB_create_context"))
    {
        GET_PROC_OR_ERROR(&mFnPtrs->createContextAttribsARBPtr, "glXCreateContextAttribsARB");
    }
    else
    {
        mFnPtrs->createContextAttribsARBPtr = nullptr;
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
void FunctionsGLX::destroyContext(glx::Context context) const
{
    GLXContext ctx = reinterpret_cast<GLXContext>(context);
    mFnPtrs->destroyContextPtr(mXDisplay, ctx);
}
Bool FunctionsGLX::makeCurrent(glx::Drawable drawable, glx::Context context) const
{
    GLXContext ctx = reinterpret_cast<GLXContext>(context);
    return mFnPtrs->makeCurrentPtr(mXDisplay, drawable, ctx);
}
void FunctionsGLX::swapBuffers(glx::Drawable drawable) const
{
    mFnPtrs->swapBuffersPtr(mXDisplay, drawable);
}
Bool FunctionsGLX::queryExtension(int *errorBase, int *event) const
{
    return mFnPtrs->queryExtensionPtr(mXDisplay, errorBase, event);
}
Bool FunctionsGLX::queryVersion(int *major, int *minor) const
{
    return mFnPtrs->queryVersionPtr(mXDisplay, major, minor);
}
void FunctionsGLX::waitX() const
{
    mFnPtrs->waitXPtr();
}
void FunctionsGLX::waitGL() const
{
    mFnPtrs->waitGLPtr();
}

// GLX 1.1
const char *FunctionsGLX::queryExtensionsString() const
{
    return mFnPtrs->queryExtensionsStringPtr(mXDisplay, mXScreen);
}

// GLX 1.4
glx::FBConfig *FunctionsGLX::getFBConfigs(int *nElements) const
{
    GLXFBConfig *configs = mFnPtrs->getFBConfigsPtr(mXDisplay, mXScreen, nElements);
    return reinterpret_cast<glx::FBConfig*>(configs);
}
glx::FBConfig *FunctionsGLX::chooseFBConfig(const int *attribList, int *nElements) const
{
    GLXFBConfig *configs = mFnPtrs->chooseFBConfigPtr(mXDisplay, mXScreen, attribList, nElements);
    return reinterpret_cast<glx::FBConfig*>(configs);
}
int FunctionsGLX::getFBConfigAttrib(glx::FBConfig config, int attribute, int *value) const
{
    GLXFBConfig cfg = reinterpret_cast<GLXFBConfig>(config);
    return mFnPtrs->getFBConfigAttribPtr(mXDisplay, cfg, attribute, value);
}
XVisualInfo *FunctionsGLX::getVisualFromFBConfig(glx::FBConfig config) const
{
    GLXFBConfig cfg = reinterpret_cast<GLXFBConfig>(config);
    return mFnPtrs->getVisualFromFBConfigPtr(mXDisplay, cfg);
}
GLXWindow FunctionsGLX::createWindow(glx::FBConfig config, Window window, const int *attribList) const
{
    GLXFBConfig cfg = reinterpret_cast<GLXFBConfig>(config);
    return mFnPtrs->createWindowPtr(mXDisplay, cfg, window, attribList);
}
void FunctionsGLX::destroyWindow(glx::Window window) const
{
    mFnPtrs->destroyWindowPtr(mXDisplay, window);
}
glx::Pbuffer FunctionsGLX::createPbuffer(glx::FBConfig config, const int *attribList) const
{
    GLXFBConfig cfg = reinterpret_cast<GLXFBConfig>(config);
    return mFnPtrs->createPbufferPtr(mXDisplay, cfg, attribList);
}
void FunctionsGLX::destroyPbuffer(glx::Pbuffer pbuffer) const
{
    mFnPtrs->destroyPbufferPtr(mXDisplay, pbuffer);
}
void FunctionsGLX::queryDrawable(glx::Drawable drawable, int attribute, unsigned int *value) const
{
    mFnPtrs->queryDrawablePtr(mXDisplay, drawable, attribute, value);
}

// GLX_ARB_create_context
glx::Context FunctionsGLX::createContextAttribsARB(glx::FBConfig config, glx::Context shareContext, Bool direct, const int *attribList) const
{
    GLXContext shareCtx = reinterpret_cast<GLXContext>(shareContext);
    GLXFBConfig cfg = reinterpret_cast<GLXFBConfig>(config);
    GLXContext ctx = mFnPtrs->createContextAttribsARBPtr(mXDisplay, cfg, shareCtx, direct, attribList);
    return reinterpret_cast<glx::Context>(ctx);
}

}
