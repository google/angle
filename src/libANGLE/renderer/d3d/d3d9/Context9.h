//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Context9:
//   D3D9-specific functionality associated with a GL Context.
//

#ifndef LIBANGLE_RENDERER_D3D_D3D9_CONTEXT9_H_
#define LIBANGLE_RENDERER_D3D_D3D9_CONTEXT9_H_

#include "libANGLE/renderer/ContextImpl.h"

namespace rx
{

class Context9 : public ContextImpl
{
  public:
    Context9() {}
    ~Context9() override {}

    gl::Error initialize(Renderer *renderer) override { return gl::NoError(); }
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_D3D_D3D9_CONTEXT9_H_
