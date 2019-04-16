// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MemoryObjectVk.cpp: Defines the class interface for MemoryObjectVk, implementing
// MemoryObjectImpl.

#include "libANGLE/renderer/vulkan/MemoryObjectVk.h"

#include <vulkan/vulkan.h>

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

#if !defined(ANGLE_PLATFORM_WINDOWS)
#    include <unistd.h>
#else
#    include <io.h>
#endif

namespace rx
{

namespace
{

constexpr int kInvalidFd = -1;

#if defined(ANGLE_PLATFORM_WINDOWS)
int close(int fd)
{
    return _close(fd);
}
#endif

}  // namespace

MemoryObjectVk::MemoryObjectVk() : mSize(0), mFd(kInvalidFd) {}

MemoryObjectVk::~MemoryObjectVk() = default;

void MemoryObjectVk::onDestroy(const gl::Context *context)
{
    if (mFd != kInvalidFd)
    {
        close(mFd);
        mFd = kInvalidFd;
    }
}

angle::Result MemoryObjectVk::importFd(gl::Context *context,
                                       GLuint64 size,
                                       gl::HandleType handleType,
                                       GLint fd)
{
    switch (handleType)
    {
        case gl::HandleType::OpaqueFd:
            return importOpaqueFd(context, size, fd);

        default:
            UNREACHABLE();
            return angle::Result::Stop;
    }
}

angle::Result MemoryObjectVk::importOpaqueFd(gl::Context *context, GLuint64 size, GLint fd)
{
    ASSERT(mFd == kInvalidFd);
    mFd   = fd;
    mSize = size;
    return angle::Result::Continue;
}

}  // namespace rx
