//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FunctionsGLX.h: Defines the FunctionsGLX class to load functions and data from GLX

#ifndef LIBANGLE_RENDERER_GL_GLX_FUNCTIONSGLX_H_
#define LIBANGLE_RENDERER_GL_GLX_FUNCTIONSGLX_H_

#include <string>
#include <vector>

#include "common/angleutils.h"
#include "libANGLE/Error.h"
#include "libANGLE/renderer/gl/glx/functionsglx_typedefs.h"

namespace rx
{

class FunctionsGLX : angle::NonCopyable
{
  public:
    FunctionsGLX();
    ~FunctionsGLX();

    // Load data from GLX, can be called multiple times
    egl::Error initialize(Display *xDisplay, int screen);
    void terminate();

    bool hasExtension(const char *extension) const;
    int majorVersion;
    int minorVersion;

    Display *getDisplay() const;
    int getScreen() const;

    PFNGLXGETPROCADDRESSPROC getProc;

    // GLX 1.0
    void destroyContext(GLXContext context) const;
    Bool makeCurrent(GLXDrawable drawable, GLXContext context) const;
    void swapBuffers(GLXDrawable drawable) const;
    Bool queryExtension(int *errorBase, int *event) const;
    Bool queryVersion(int *major, int *minor) const;
    void waitX() const;
    void waitGL() const;

    // GLX 1.1
    const char *queryExtensionsString() const;

    // GLX 1.3
    GLXFBConfig *getFBConfigs(int *nElements) const;
    GLXFBConfig *chooseFBConfig(const int *attribList, int *nElements) const;
    int getFBConfigAttrib(GLXFBConfig config, int attribute, int *value) const;
    XVisualInfo *getVisualFromFBConfig(GLXFBConfig config) const;
    GLXWindow createWindow(GLXFBConfig config, Window window, const int *attribList) const;
    void destroyWindow(GLXWindow window) const;
    GLXPbuffer createPbuffer(GLXFBConfig config, const int *attribList) const;
    void destroyPbuffer(GLXPbuffer pbuffer) const;
    void queryDrawable(GLXDrawable drawable, int attribute, unsigned int *value) const;

    // GLX_ARB_create_context
    GLXContext createContextAttribsARB(GLXFBConfig config, GLXContext shareContext, Bool direct, const int *attribList) const;

  private:
    void *mLibHandle;
    Display *mXDisplay;
    int mXScreen;

    std::vector<std::string> mExtensions;

    // GLX 1.0
    PFNGLXDESTROYCONTEXTPROC mDestroyContextPtr;
    PFNGLXMAKECURRENTPROC mMakeCurrentPtr;
    PFNGLXSWAPBUFFERSPROC mSwapBuffersPtr;
    PFNGLXQUERYEXTENSIONPROC mQueryExtensionPtr;
    PFNGLXQUERYVERSIONPROC mQueryVersionPtr;
    PFNGLXWAITXPROC mWaitXPtr;
    PFNGLXWAITGLPROC mWaitGLPtr;

    // GLX 1.1
    PFNGLXQUERYEXTENSIONSSTRINGPROC mQueryExtensionsStringPtr;

    //GLX 1.3
    PFNGLXGETFBCONFIGSPROC mGetFBConfigsPtr;
    PFNGLXCHOOSEFBCONFIGPROC mChooseFBConfigPtr;
    PFNGLXGETFBCONFIGATTRIBPROC mGetFBConfigAttribPtr;
    PFNGLXGETVISUALFROMFBCONFIGPROC mGetVisualFromFBConfigPtr;
    PFNGLXCREATEWINDOWPROC mCreateWindowPtr;
    PFNGLXDESTROYWINDOWPROC mDestroyWindowPtr;
    PFNGLXCREATEPBUFFERPROC mCreatePbufferPtr;
    PFNGLXDESTROYPBUFFERPROC mDestroyPbufferPtr;
    PFNGLXQUERYDRAWABLEPROC mQueryDrawablePtr;

    // GLX_ARB_create_context
    PFNGLXCREATECONTEXTATTRIBSARBPROC mCreateContextAttribsARBPtr;
};

}

#endif // LIBANGLE_RENDERER_GL_GLX_FUNCTIONSGLX_H_
