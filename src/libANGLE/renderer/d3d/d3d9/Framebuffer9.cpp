//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer9.cpp: Implements the Framebuffer9 class.

#include "libANGLE/renderer/d3d/d3d9/Framebuffer9.h"
#include "libANGLE/renderer/d3d/d3d9/Renderer9.h"

namespace rx
{

Framebuffer9::Framebuffer9(Renderer9 *renderer)
    : FramebufferD3D(renderer),
      mRenderer(renderer)
{
    ASSERT(mRenderer != nullptr);
}

Framebuffer9::~Framebuffer9()
{
}

}
