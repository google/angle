//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImageVk.cpp:
//    Implements the class methods for ImageVk.
//

#include "libANGLE/renderer/vulkan/ImageVk.h"

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace rx
{

ImageVk::ImageVk(const egl::ImageState &state) : ImageImpl(state) {}

ImageVk::~ImageVk() {}

egl::Error ImageVk::initialize(const egl::Display *display)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

angle::Result ImageVk::orphan(const gl::Context *context, egl::ImageSibling *sibling)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}
}  // namespace rx
