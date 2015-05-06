//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayGLX.h: GLX implementation of egl::Display

#ifndef LIBANGLE_RENDERER_GL_GLX_DISPLAYGLX_H_
#define LIBANGLE_RENDERER_GL_GLX_DISPLAYGLX_H_

#include <string>
#include <vector>

#include "libANGLE/renderer/gl/DisplayGL.h"
#include "libANGLE/renderer/gl/glx/FunctionsGLX.h"

namespace rx
{

class FunctionsGLX;

class DisplayGLX : public DisplayGL
{
  public:
    DisplayGLX();
    ~DisplayGLX() override;

    egl::Error initialize(egl::Display *display) override;
    void terminate() override;

    SurfaceImpl *createWindowSurface(const egl::Config *configuration,
                                     EGLNativeWindowType window,
                                     const egl::AttributeMap &attribs) override;
    SurfaceImpl *createPbufferSurface(const egl::Config *configuration,
                                      const egl::AttributeMap &attribs) override;
    SurfaceImpl *createPbufferFromClientBuffer(const egl::Config *configuration,
                                               EGLClientBuffer shareHandle,
                                               const egl::AttributeMap &attribs) override;
    SurfaceImpl *createPixmapSurface(const egl::Config *configuration,
                                     NativePixmapType nativePixmap,
                                     const egl::AttributeMap &attribs) override;

    egl::ConfigSet generateConfigs() const override;

    bool isDeviceLost() const override;
    bool testDeviceLost() override;
    egl::Error restoreLostDevice() override;

    bool isValidNativeWindow(EGLNativeWindowType window) const override;

    egl::Error getDevice(DeviceImpl **device) override;

    std::string getVendorString() const override;

  private:
    const FunctionsGL *getFunctionsGL() const override;

    void generateExtensions(egl::DisplayExtensions *outExtensions) const override;
    void generateCaps(egl::Caps *outCaps) const override;

    int getGLXFBConfigAttrib(GLXFBConfig config, int attrib) const;

    FunctionsGL *mFunctionsGL;

    //TODO(cwallez) yuck, change generateConfigs to be non-const or add a userdata member to egl::Config?
    mutable std::map<int, GLXFBConfig> configIdToGLXConfig;

    // The ID of the visual used to create the context
    int mContextVisualId;
    GLXContext mContext;
    // A pbuffer the context is current on during ANGLE initialization
    GLXPbuffer mDummyPbuffer;

    FunctionsGLX mGLX;
    egl::Display *mEGLDisplay;
    Display *mXDisplay;
};

}

#endif // LIBANGLE_RENDERER_GL_GLX_DISPLAYGLX_H_
