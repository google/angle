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
    egl::Error initialize(Display *xDisplay);
    void terminate();

    bool hasExtension(const char *extension) const;

    int majorVersion;
    int minorVersion;

    PFNGLXGETPROCADDRESSPROC getProc;

    // GLX 1.0
    PFNGLXDESTROYCONTEXTPROC destroyContext;
    PFNGLXMAKECURRENTPROC makeCurrent;
    PFNGLXSWAPBUFFERSPROC swapBuffers;
    PFNGLXQUERYEXTENSIONPROC queryExtension;
    PFNGLXQUERYVERSIONPROC queryVersion;

    // GLX 1.1
    PFNGLXQUERYEXTENSIONSSTRINGPROC queryExtensionsString;

    //GLX 1.3
    PFNGLXGETFBCONFIGSPROC getFBConfigs;
    PFNGLXCHOOSEFBCONFIGPROC chooseFBConfig;
    PFNGLXGETFBCONFIGATTRIBPROC getFBConfigAttrib;
    PFNGLXGETVISUALFROMFBCONFIGPROC getVisualFromFBConfig;
    PFNGLXCREATEWINDOWPROC createWindow;
    PFNGLXDESTROYWINDOWPROC destroyWindow;
    PFNGLXCREATEPBUFFERPROC createPbuffer;
    PFNGLXDESTROYPBUFFERPROC destroyPbuffer;

    // GLX_ARB_create_context
    PFNGLXCREATECONTEXTATTRIBSARBPROC createContextAttribsARB;

  private:
    void *mLibHandle;
    std::vector<std::string> mExtensions;
};

}

#endif // LIBANGLE_RENDERER_GL_GLX_FUNCTIONSGLX_H_
