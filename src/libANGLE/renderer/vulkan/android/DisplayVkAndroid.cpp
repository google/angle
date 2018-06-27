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
    egl::Config rgba;
    rgba.renderTargetFormat    = GL_RGBA8;
    rgba.depthStencilFormat    = GL_NONE;
    rgba.bufferSize            = 32;
    rgba.redSize               = 8;
    rgba.greenSize             = 8;
    rgba.blueSize              = 8;
    rgba.alphaSize             = 8;
    rgba.alphaMaskSize         = 0;
    rgba.bindToTextureRGB      = EGL_FALSE;
    rgba.bindToTextureRGBA     = EGL_FALSE;
    rgba.colorBufferType       = EGL_RGB_BUFFER;
    rgba.configCaveat          = EGL_NONE;
    rgba.conformant            = 0;
    rgba.depthSize             = 0;
    rgba.stencilSize           = 0;
    rgba.level                 = 0;
    rgba.matchNativePixmap     = EGL_NONE;
    rgba.maxPBufferWidth       = 0;
    rgba.maxPBufferHeight      = 0;
    rgba.maxPBufferPixels      = 0;
    rgba.maxSwapInterval       = 1;
    rgba.minSwapInterval       = 1;
    rgba.nativeRenderable      = EGL_TRUE;
    rgba.nativeVisualID        = 0;
    rgba.nativeVisualType      = EGL_NONE;
    rgba.renderableType        = EGL_OPENGL_ES2_BIT;
    rgba.sampleBuffers         = 0;
    rgba.samples               = 0;
    rgba.surfaceType           = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;
    rgba.optimalOrientation    = 0;
    rgba.transparentType       = EGL_NONE;
    rgba.transparentRedValue   = 0;
    rgba.transparentGreenValue = 0;
    rgba.transparentBlueValue  = 0;
    rgba.colorComponentType    = EGL_COLOR_COMPONENT_TYPE_FIXED_EXT;

    egl::Config rgbaD24S8;
    rgbaD24S8                    = rgba;
    rgbaD24S8.depthStencilFormat = GL_DEPTH24_STENCIL8;
    rgbaD24S8.depthSize          = 24;
    rgbaD24S8.stencilSize        = 8;

    egl::ConfigSet configSet;
    configSet.add(rgba);
    configSet.add(rgbaD24S8);
    return configSet;
}

const char *DisplayVkAndroid::getWSIName() const
{
    return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

}  // namespace rx
