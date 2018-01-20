//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DisplayVkWin32.cpp:
//    Implements the class methods for DisplayVkWin32.
//

#include "libANGLE/renderer/vulkan/win32/DisplayVkWin32.h"

#include <vulkan/vulkan.h>

#include "libANGLE/renderer/vulkan/win32/WindowSurfaceVkWin32.h"

namespace rx
{

DisplayVkWin32::DisplayVkWin32(const egl::DisplayState &state) : DisplayVk(state)
{
}

bool DisplayVkWin32::isValidNativeWindow(EGLNativeWindowType window) const
{
    return (IsWindow(window) == TRUE);
}

SurfaceImpl *DisplayVkWin32::createWindowSurfaceVk(const egl::SurfaceState &state,
                                                   EGLNativeWindowType window,
                                                   EGLint width,
                                                   EGLint height)
{
    return new WindowSurfaceVkWin32(state, window, width, height);
}

egl::ConfigSet DisplayVkWin32::generateConfigs()
{
    // TODO(jmadill): Multiple configs, pbuffers, and proper checking of config attribs.
    egl::Config singleton;
    singleton.renderTargetFormat    = GL_BGRA8_EXT;
    singleton.depthStencilFormat    = GL_NONE;
    singleton.bufferSize            = 32;
    singleton.redSize               = 8;
    singleton.greenSize             = 8;
    singleton.blueSize              = 8;
    singleton.alphaSize             = 8;
    singleton.alphaMaskSize         = 0;
    singleton.bindToTextureRGB      = EGL_FALSE;
    singleton.bindToTextureRGBA     = EGL_FALSE;
    singleton.colorBufferType       = EGL_RGB_BUFFER;
    singleton.configCaveat          = EGL_NONE;
    singleton.conformant            = 0;
    singleton.depthSize             = 0;
    singleton.stencilSize           = 0;
    singleton.level                 = 0;
    singleton.matchNativePixmap     = EGL_NONE;
    singleton.maxPBufferWidth       = 0;
    singleton.maxPBufferHeight      = 0;
    singleton.maxPBufferPixels      = 0;
    singleton.maxSwapInterval       = 1;
    singleton.minSwapInterval       = 1;
    singleton.nativeRenderable      = EGL_TRUE;
    singleton.nativeVisualID        = 0;
    singleton.nativeVisualType      = EGL_NONE;
    singleton.renderableType        = EGL_OPENGL_ES2_BIT;
    singleton.sampleBuffers         = 0;
    singleton.samples               = 0;
    singleton.surfaceType           = EGL_WINDOW_BIT;
    singleton.optimalOrientation    = 0;
    singleton.transparentType       = EGL_NONE;
    singleton.transparentRedValue   = 0;
    singleton.transparentGreenValue = 0;
    singleton.transparentBlueValue  = 0;
    singleton.colorComponentType    = EGL_COLOR_COMPONENT_TYPE_FIXED_EXT;

    egl::ConfigSet configSet;
    configSet.add(singleton);
    return configSet;
}

const char *DisplayVkWin32::getWSIName() const
{
    return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
}

}  // namespace rx
