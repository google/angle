//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/renderer/gl/egl/ContextEGL.h"

namespace rx
{
ContextEGL::ContextEGL(const gl::ContextState &state, const std::shared_ptr<RendererEGL> &renderer)
    : ContextGL(state, renderer), mRenderer(renderer)
{}

ContextEGL::~ContextEGL() {}

EGLContext ContextEGL::getContext() const
{
    return mRenderer->getContext();
}
}  // namespace rx
