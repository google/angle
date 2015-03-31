//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferImpl.h: Defines the abstract class gl::RenderbufferImpl

#ifndef LIBANGLE_RENDERER_RENDERBUFFERIMPL_H_
#define LIBANGLE_RENDERER_RENDERBUFFERIMPL_H_

#include "angle_gl.h"

#include "libANGLE/Error.h"

#include "common/angleutils.h"

namespace rx
{

class RenderbufferImpl : angle::NonCopyable
{
  public:
    RenderbufferImpl();
    virtual ~RenderbufferImpl() = 0;

    virtual gl::Error setStorage(GLenum internalformat, size_t width, size_t height) = 0;
    virtual gl::Error setStorageMultisample(size_t samples, GLenum internalformat, size_t width, size_t height) = 0;
};

}

#endif   // LIBANGLE_RENDERER_RENDERBUFFERIMPL_H_
