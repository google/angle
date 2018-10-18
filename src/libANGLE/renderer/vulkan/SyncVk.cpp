//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SyncVk.cpp:
//    Implements the class methods for SyncVk.
//

#include "libANGLE/renderer/vulkan/SyncVk.h"

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace rx
{

SyncVk::SyncVk() : SyncImpl()
{
}

SyncVk::~SyncVk()
{
}

angle::Result SyncVk::set(const gl::Context *context, GLenum condition, GLbitfield flags)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result SyncVk::clientWait(const gl::Context *context,
                                 GLbitfield flags,
                                 GLuint64 timeout,
                                 GLenum *outResult)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result SyncVk::serverWait(const gl::Context *context, GLbitfield flags, GLuint64 timeout)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

angle::Result SyncVk::getStatus(const gl::Context *context, GLint *outResult)
{
    ANGLE_VK_UNREACHABLE(vk::GetImpl(context));
    return angle::Result::Stop();
}

}  // namespace rx
