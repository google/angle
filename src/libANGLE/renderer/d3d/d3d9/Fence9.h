//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence9.h: Defines the rx::FenceNV9 class which implements rx::FenceNVImpl.

#ifndef LIBANGLE_RENDERER_D3D_D3D9_FENCE9_H_
#define LIBANGLE_RENDERER_D3D_D3D9_FENCE9_H_

#include "libANGLE/renderer/FenceNVImpl.h"
#include "libANGLE/renderer/FenceSyncImpl.h"

namespace rx
{
class Renderer9;

class FenceNV9 : public FenceNVImpl
{
  public:
    explicit FenceNV9(Renderer9 *renderer);
    virtual ~FenceNV9();

    gl::Error set();
    gl::Error test(bool flushCommandBuffer, GLboolean *outFinished);
    gl::Error finishFence(GLboolean *outFinished);

  private:
    Renderer9 *mRenderer;
    IDirect3DQuery9 *mQuery;
};

}

#endif // LIBANGLE_RENDERER_D3D_D3D9_FENCE9_H_
