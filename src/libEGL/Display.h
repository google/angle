//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Display.h: Defines the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#ifndef LIBEGL_DISPLAY_H_
#define LIBEGL_DISPLAY_H_

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d9.h>

#include <set>
#include <vector>

#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/Renderer9.h"

#include "libEGL/Config.h"
#include "libEGL/Surface.h"

namespace egl
{
class Display
{
  public:
    ~Display();

    bool initialize();
    void terminate();

    static egl::Display *getDisplay(EGLNativeDisplayType displayId);

    bool getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig);
    bool getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value);

    EGLSurface createWindowSurface(HWND window, EGLConfig config, const EGLint *attribList);
    EGLSurface createOffscreenSurface(EGLConfig config, HANDLE shareHandle, const EGLint *attribList);
    EGLContext createContext(EGLConfig configHandle, const gl::Context *shareContext, bool notifyResets, bool robustAccess);

    void destroySurface(egl::Surface *surface);
    void destroyContext(gl::Context *context);

    bool isInitialized() const;
    bool isValidConfig(EGLConfig config);
    bool isValidContext(gl::Context *context);
    bool isValidSurface(egl::Surface *surface);
    bool hasExistingWindowSurface(HWND window);

    renderer::Renderer9 *getRenderer() { return mRenderer; };

    virtual void notifyDeviceLost();

    const char *getExtensionString() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Display);

    Display(EGLNativeDisplayType displayId, HDC deviceContext, bool software);

    bool restoreLostDevice();

    EGLNativeDisplayType mDisplayId;
    const HDC mDc;

    bool mSoftwareDevice;
    
    typedef std::set<Surface*> SurfaceSet;
    SurfaceSet mSurfaceSet;

    ConfigSet mConfigSet;

    typedef std::set<gl::Context*> ContextSet;
    ContextSet mContextSet;

    renderer::Renderer9 *mRenderer;   // D3D9_REPLACE

    void initExtensionString();
    std::string mExtensionString;
};
}

#endif   // LIBEGL_DISPLAY_H_
