//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FenceNVGL.h: Defines the class interface for FenceNVGL.

#ifndef LIBANGLE_RENDERER_GL_FENCENVGL_H_
#define LIBANGLE_RENDERER_GL_FENCENVGL_H_

#include "libANGLE/renderer/FenceNVImpl.h"

namespace rx
{

class FenceNVGL : public FenceNVImpl
{
  public:
    FenceNVGL();
    ~FenceNVGL() override;

    gl::Error set() override;
    gl::Error test(bool flushCommandBuffer, GLboolean *outFinished) override;
    gl::Error finishFence(GLboolean *outFinished) override;

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceNVGL);
};

}

#endif // LIBANGLE_RENDERER_GL_FENCENVGL_H_
