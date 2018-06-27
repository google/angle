//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DisplayVkAndroid.h:
//    Defines the class interface for DisplayVkAndroid, implementing DisplayVk for Android.
//

#ifndef LIBANGLE_RENDERER_VULKAN_ANDROID_DISPLAYVKANDROID_H_
#define LIBANGLE_RENDERER_VULKAN_ANDROID_DISPLAYVKANDROID_H_

#include "libANGLE/renderer/vulkan/DisplayVk.h"

namespace rx
{
class DisplayVkAndroid : public DisplayVk
{
  public:
    DisplayVkAndroid(const egl::DisplayState &state);

    bool isValidNativeWindow(EGLNativeWindowType window) const override;

    SurfaceImpl *createWindowSurfaceVk(const egl::SurfaceState &state,
                                       EGLNativeWindowType window,
                                       EGLint width,
                                       EGLint height) override;

    egl::ConfigSet generateConfigs() override;
    bool checkConfigSupport(egl::Config *config) override;

    const char *getWSIName() const override;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_ANDROID_DISPLAYVKANDROID_H_
