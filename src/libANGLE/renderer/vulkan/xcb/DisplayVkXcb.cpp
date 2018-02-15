//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DisplayVkXcb.cpp:
//    Implements the class methods for DisplayVkXcb.
//

#include "libANGLE/renderer/vulkan/xcb/DisplayVkXcb.h"

#include <xcb/xcb.h>

#include "libANGLE/renderer/vulkan/xcb/WindowSurfaceVkXcb.h"

namespace rx
{

DisplayVkXcb::DisplayVkXcb(const egl::DisplayState &state)
    : DisplayVk(state), mXcbConnection(nullptr)
{
}

egl::Error DisplayVkXcb::initialize(egl::Display *display)
{
    mXcbConnection = xcb_connect(nullptr, nullptr);
    if (mXcbConnection == nullptr)
    {
        return egl::EglNotInitialized();
    }
    return DisplayVk::initialize(display);
}

void DisplayVkXcb::terminate()
{
    ASSERT(mXcbConnection != nullptr);
    xcb_disconnect(mXcbConnection);
    mXcbConnection = nullptr;
    DisplayVk::terminate();
}

bool DisplayVkXcb::isValidNativeWindow(EGLNativeWindowType window) const
{
    // There doesn't appear to be an xcb function explicitly for checking the validity of a
    // window ID, but xcb_query_tree_reply will return nullptr if the window doesn't exist.
    xcb_query_tree_cookie_t cookie = xcb_query_tree(mXcbConnection, window);
    xcb_query_tree_reply_t *reply  = xcb_query_tree_reply(mXcbConnection, cookie, nullptr);
    if (reply)
    {
        free(reply);
        return true;
    }
    return false;
}

SurfaceImpl *DisplayVkXcb::createWindowSurfaceVk(const egl::SurfaceState &state,
                                                 EGLNativeWindowType window,
                                                 EGLint width,
                                                 EGLint height)
{
    return new WindowSurfaceVkXcb(state, window, width, height, mXcbConnection);
}

egl::ConfigSet DisplayVkXcb::generateConfigs()
{
    // TODO(jmadill): Multiple configs, pbuffers, and proper checking of config attribs.
    egl::Config bgra;
    bgra.renderTargetFormat    = GL_BGRA8_EXT;
    bgra.depthStencilFormat    = GL_NONE;
    bgra.bufferSize            = 32;
    bgra.redSize               = 8;
    bgra.greenSize             = 8;
    bgra.blueSize              = 8;
    bgra.alphaSize             = 8;
    bgra.alphaMaskSize         = 0;
    bgra.bindToTextureRGB      = EGL_FALSE;
    bgra.bindToTextureRGBA     = EGL_FALSE;
    bgra.colorBufferType       = EGL_RGB_BUFFER;
    bgra.configCaveat          = EGL_NONE;
    bgra.conformant            = 0;
    bgra.depthSize             = 0;
    bgra.stencilSize           = 0;
    bgra.level                 = 0;
    bgra.matchNativePixmap     = EGL_NONE;
    bgra.maxPBufferWidth       = 0;
    bgra.maxPBufferHeight      = 0;
    bgra.maxPBufferPixels      = 0;
    bgra.maxSwapInterval       = 1;
    bgra.minSwapInterval       = 1;
    bgra.nativeRenderable      = EGL_TRUE;
    bgra.nativeVisualID        = 0;
    bgra.nativeVisualType      = EGL_NONE;
    bgra.renderableType        = EGL_OPENGL_ES2_BIT;
    bgra.sampleBuffers         = 0;
    bgra.samples               = 0;
    bgra.surfaceType           = EGL_WINDOW_BIT;
    bgra.optimalOrientation    = 0;
    bgra.transparentType       = EGL_NONE;
    bgra.transparentRedValue   = 0;
    bgra.transparentGreenValue = 0;
    bgra.transparentBlueValue  = 0;
    bgra.colorComponentType    = EGL_COLOR_COMPONENT_TYPE_FIXED_EXT;

    egl::Config bgraD24S8;
    bgraD24S8                    = bgra;
    bgraD24S8.depthStencilFormat = GL_DEPTH24_STENCIL8;
    bgraD24S8.depthSize          = 24;
    bgraD24S8.stencilSize        = 8;

    egl::ConfigSet configSet;
    configSet.add(bgra);
    configSet.add(bgraD24S8);
    return configSet;
}

const char *DisplayVkXcb::getWSIName() const
{
    return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
}

}  // namespace rx
