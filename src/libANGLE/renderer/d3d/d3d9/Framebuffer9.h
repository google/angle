//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer9.h: Defines the Framebuffer9 class.

#ifndef LIBANGLE_RENDERER_D3D_D3D9_FRAMBUFFER9_H_
#define LIBANGLE_RENDERER_D3D_D3D9_FRAMBUFFER9_H_

#include "libANGLE/renderer/d3d/FramebufferD3D.h"

namespace rx
{
class Renderer9;

class Framebuffer9 : public FramebufferD3D
{
  public:
    Framebuffer9(Renderer9 *renderer);
    virtual ~Framebuffer9();

  private:
    gl::Error clear(const gl::State &state, const gl::ClearParameters &clearParams) override;

    Renderer9 *const mRenderer;
};

}

#endif // LIBANGLE_RENDERER_D3D_D3D9_FRAMBUFFER9_H_
