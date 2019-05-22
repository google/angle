//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SemaphoreImpl.h: Implements the rx::SemaphoreImpl class [EXT_external_objects]

#ifndef LIBANGLE_RENDERER_SEMAPHOREIMPL_H_
#define LIBANGLE_RENDERER_SEMAPHOREIMPL_H_

#include "angle_gl.h"
#include "common/PackedEnums.h"
#include "common/angleutils.h"
#include "libANGLE/Error.h"

namespace gl
{
class Context;
class Semaphore;
}  // namespace gl

namespace rx
{

class SemaphoreImpl : angle::NonCopyable
{
  public:
    virtual ~SemaphoreImpl() {}

    virtual void onDestroy(const gl::Context *context) = 0;

    virtual angle::Result importFd(gl::Context *context, gl::HandleType handleType, GLint fd) = 0;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_SEMAPHOREIMPL_H_
