//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DisplayVkAndroid.cpp:
//    Implements the class methods for DisplayVkAndroid.
//

#include "libANGLE/renderer/vulkan/android/DisplayVkAndroid.h"

#include <android/native_window.h>
#include <vulkan/vulkan.h>

#include "libANGLE/renderer/vulkan/android/WindowSurfaceVkAndroid.h"

namespace rx
{

DisplayVkAndroid::DisplayVkAndroid(const egl::DisplayState &state) : DisplayVk(state)
{
}

bool DisplayVkAndroid::isValidNativeWindow(EGLNativeWindowType window) const
{
    return (ANativeWindow_getFormat(window) >= 0);
}

SurfaceImpl *DisplayVkAndroid::createWindowSurfaceVk(const egl::SurfaceState &state,
                                                     EGLNativeWindowType window,
                                                     EGLint width,
                                                     EGLint height)
{
    return new WindowSurfaceVkAndroid(state, window, width, height);
}

egl::ConfigSet DisplayVkAndroid::generateConfigs()
{
    // TODO(jmadill): Multiple configs, pbuffers, and proper checking of config attribs.
    egl::Config singleton;
    singleton.renderTargetFormat    = GL_RGBA8;
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

const char *DisplayVkAndroid::getWSIName() const
{
    return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

}  // namespace rx
