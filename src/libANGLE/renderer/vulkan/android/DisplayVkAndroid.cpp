//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DisplayVkAndroid.cpp:
//    Implements the class methods for DisplayVkAndroid.
//

#include "libANGLE/renderer/vulkan/android/DisplayVkAndroid.h"

#include <android/log.h>
#include <android/native_window.h>
#include <vulkan/vulkan.h>

#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/android/WindowSurfaceVkAndroid.h"
#include "libANGLE/renderer/vulkan/vk_caps_utils.h"

namespace rx
{

DisplayVkAndroid::DisplayVkAndroid(const egl::DisplayState &state) : DisplayVk(state) {}

egl::Error DisplayVkAndroid::initialize(egl::Display *display)
{
    ANGLE_TRY(DisplayVk::initialize(display));
    std::string rendererDescription = mRenderer->getRendererDescription();
    __android_log_print(ANDROID_LOG_INFO, "ANGLE", "%s", rendererDescription.c_str());
    return egl::NoError();
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
    constexpr GLenum kColorFormats[] = {GL_RGBA8, GL_RGB8, GL_RGB565};
    constexpr EGLint kSampleCounts[] = {0};
    return egl_vk::GenerateConfigs(kColorFormats, egl_vk::kConfigDepthStencilFormats, kSampleCounts,
                                   this);
}

bool DisplayVkAndroid::checkConfigSupport(egl::Config *config)
{
    // TODO(geofflang): Test for native support and modify the config accordingly.
    // anglebug.com/2692
    return true;
}

const char *DisplayVkAndroid::getWSIName() const
{
    return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

}  // namespace rx
