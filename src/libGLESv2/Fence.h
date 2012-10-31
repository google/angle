//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence.h: Defines the gl::Fence class, which supports the GL_NV_fence extension.

#ifndef LIBGLESV2_FENCE_H_
#define LIBGLESV2_FENCE_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/angleutils.h"
#include "libGLESv2/renderer/Renderer9.h"

namespace gl
{

class Fence
{
  public:
    explicit Fence(rx::Renderer9 *renderer);
    virtual ~Fence();

    GLboolean isFence();
    void setFence(GLenum condition);
    GLboolean testFence();
    void finishFence();
    void getFenceiv(GLenum pname, GLint *params);

  private:
    DISALLOW_COPY_AND_ASSIGN(Fence);

    rx::Renderer9 *mRenderer;  // D3D9_REPLACE
    IDirect3DQuery9* mQuery;  // D3D9_REPLACE
    GLenum mCondition;
    GLboolean mStatus;
};

}

#endif   // LIBGLESV2_FENCE_H_
