//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayCGL.mm: CGL implementation of egl::Display

#include "libANGLE/renderer/gl/CGL/DisplayCGL.h"

#include "common/debug.h"
#include "libANGLE/renderer/gl/CGL/WindowSurfaceCGL.h"

namespace rx
{

DisplayCGL::DisplayCGL()
    : DisplayGL(),
      mEGLDisplay(nullptr)
{
}

DisplayCGL::~DisplayCGL()
{
}

egl::Error DisplayCGL::initialize(egl::Display *display)
{
    UNIMPLEMENTED();
    mEGLDisplay = display;
    return DisplayGL::initialize(display);
}

void DisplayCGL::terminate()
{
    UNIMPLEMENTED();
    DisplayGL::terminate();
}

SurfaceImpl *DisplayCGL::createWindowSurface(const egl::Config *configuration,
                                             EGLNativeWindowType window,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return new WindowSurfaceCGL(this->getRenderer());
}

SurfaceImpl *DisplayCGL::createPbufferSurface(const egl::Config *configuration,
                                              const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl* DisplayCGL::createPbufferFromClientBuffer(const egl::Config *configuration,
                                                       EGLClientBuffer shareHandle,
                                                       const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl *DisplayCGL::createPixmapSurface(const egl::Config *configuration,
                                             NativePixmapType nativePixmap,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

egl::Error DisplayCGL::getDevice(DeviceImpl **device)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_BAD_DISPLAY);
}

egl::ConfigSet DisplayCGL::generateConfigs() const
{
    UNIMPLEMENTED();
    egl::ConfigSet configs;
    return configs;
}

bool DisplayCGL::isDeviceLost() const
{
    UNIMPLEMENTED();
    return false;
}

bool DisplayCGL::testDeviceLost()
{
    UNIMPLEMENTED();
    return false;
}

egl::Error DisplayCGL::restoreLostDevice()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_BAD_DISPLAY);
}

bool DisplayCGL::isValidNativeWindow(EGLNativeWindowType window) const
{
    UNIMPLEMENTED();
    return true;
}

std::string DisplayCGL::getVendorString() const
{
    UNIMPLEMENTED();
    return "";
}

const FunctionsGL *DisplayCGL::getFunctionsGL() const
{
    UNIMPLEMENTED();
    return nullptr;
}

void DisplayCGL::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    UNIMPLEMENTED();
}

void DisplayCGL::generateCaps(egl::Caps *outCaps) const
{
    UNIMPLEMENTED();
}
}
