
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RendererD3D.h: Defines a back-end specific class for the DirectX renderer.

#ifndef LIBGLESV2_RENDERER_RENDERERD3D_H_
#define LIBGLESV2_RENDERER_RENDERERD3D_H_

#include "libGLESv2/renderer/Renderer.h"

namespace rx
{

class RendererD3D : public Renderer
{
  public:
    explicit RendererD3D(egl::Display *display);
    virtual ~RendererD3D();

  private:
    DISALLOW_COPY_AND_ASSIGN(RendererD3D);
};

}

#endif // LIBGLESV2_RENDERER_RENDERERD3D_H_
