//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RendererD3D.cpp: Implementation of the base D3D Renderer.

#include "libGLESv2/renderer/d3d/RendererD3D.h"

namespace rx
{

RendererD3D::RendererD3D(egl::Display *display)
    : mDisplay(display),
      mCurrentClientVersion(2)
{
}

RendererD3D::~RendererD3D()
{
}

// static
RendererD3D *RendererD3D::makeRendererD3D(Renderer *renderer)
{
    ASSERT(HAS_DYNAMIC_TYPE(RendererD3D*, renderer));
    return static_cast<RendererD3D*>(renderer);
}

}
