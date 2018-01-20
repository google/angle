//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// WindowSurfaceVkAndroid.cpp:
//    Implements the class methods for WindowSurfaceVkAndroid.
//

#include "libANGLE/renderer/vulkan/android/WindowSurfaceVkAndroid.h"

#include <android/native_window.h>

#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{

WindowSurfaceVkAndroid::WindowSurfaceVkAndroid(const egl::SurfaceState &surfaceState,
                                               EGLNativeWindowType window,
                                               EGLint width,
                                               EGLint height)
    : WindowSurfaceVk(surfaceState, window, width, height)
{
}

vk::ErrorOrResult<gl::Extents> WindowSurfaceVkAndroid::createSurfaceVk(RendererVk *renderer)
{
    VkAndroidSurfaceCreateInfoKHR createInfo;

    createInfo.sType  = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext  = nullptr;
    createInfo.flags  = 0;
    createInfo.window = mNativeWindowType;
    ANGLE_VK_TRY(
        vkCreateAndroidSurfaceKHR(renderer->getInstance(), &createInfo, nullptr, &mSurface));

    int32_t width  = ANativeWindow_getWidth(mNativeWindowType);
    int32_t height = ANativeWindow_getHeight(mNativeWindowType);
    ANGLE_VK_CHECK(width > 0 && height > 0, VK_ERROR_INITIALIZATION_FAILED);

    return gl::Extents(width, height, 0);
}

}  // namespace rx
