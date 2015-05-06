//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayGLX.h: GLX implementation of egl::Display

#include "libANGLE/renderer/gl/glx/DisplayGLX.h"

#include "common/debug.h"
#include "libANGLE/Config.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"

namespace rx
{

class FunctionsGLX : public FunctionsGL
{
  public:
    FunctionsGLX()
    {
    }

    virtual ~FunctionsGLX()
    {
    }

  private:
    void *loadProcAddress(const std::string &function) override
    {
        return nullptr;
    }
};

DisplayGLX::DisplayGLX()
    : DisplayGL(),
      mFunctionsGL(nullptr)
{
}

DisplayGLX::~DisplayGLX()
{
}

egl::Error DisplayGLX::initialize(egl::Display *display)
{
    mFunctionsGL = new FunctionsGLX;
    mFunctionsGL->initialize();

    return egl::Error(EGL_SUCCESS);
}

void DisplayGLX::terminate()
{
    DisplayGL::terminate();

    SafeDelete(mFunctionsGL);
}

SurfaceImpl *DisplayGLX::createWindowSurface(const egl::Config *configuration,
                                             EGLNativeWindowType window,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl *DisplayGLX::createPbufferSurface(const egl::Config *configuration,
                                              const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl* DisplayGLX::createPbufferFromClientBuffer(const egl::Config *configuration,
                                                       EGLClientBuffer shareHandle,
                                                       const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl *DisplayGLX::createPixmapSurface(const egl::Config *configuration,
                                             NativePixmapType nativePixmap,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

egl::Error DisplayGLX::getDevice(DeviceImpl **device)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_BAD_DISPLAY);
}

egl::ConfigSet DisplayGLX::generateConfigs() const
{
    UNIMPLEMENTED();
    egl::ConfigSet configs;
    return configs;
}

bool DisplayGLX::isDeviceLost() const
{
    UNIMPLEMENTED();
    return false;
}

bool DisplayGLX::testDeviceLost()
{
    UNIMPLEMENTED();
    return false;
}

egl::Error DisplayGLX::restoreLostDevice()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_BAD_DISPLAY);
}

bool DisplayGLX::isValidNativeWindow(EGLNativeWindowType window) const
{
    UNIMPLEMENTED();
    return true;
}

std::string DisplayGLX::getVendorString() const
{
    UNIMPLEMENTED();
    return "";
}

const FunctionsGL *DisplayGLX::getFunctionsGL() const
{
    UNIMPLEMENTED();
    return mFunctionsGL;
}

void DisplayGLX::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    UNIMPLEMENTED();
}

void DisplayGLX::generateCaps(egl::Caps *outCaps) const
{
    UNIMPLEMENTED();
}

}
