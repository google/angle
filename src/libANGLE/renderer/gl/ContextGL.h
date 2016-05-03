//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextGL:
//   OpenGL-specific functionality associated with a GL Context.
//

#ifndef LIBANGLE_RENDERER_GL_CONTEXTGL_H_
#define LIBANGLE_RENDERER_GL_CONTEXTGL_H_

#include "libANGLE/renderer/ContextImpl.h"

namespace rx
{

class ContextGL : public ContextImpl
{
  public:
    ContextGL() {}
    ~ContextGL() override {}

    gl::Error initialize(Renderer *renderer) override { return gl::NoError(); }
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_GL_CONTEXTGL_H_
