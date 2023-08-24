//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramExecutableD3D.h: Implementation of ProgramExecutableImpl.

#ifndef LIBANGLE_RENDERER_D3D_PROGRAMEXECUTABLED3D_H_
#define LIBANGLE_RENDERER_D3D_PROGRAMEXECUTABLED3D_H_

#include "libANGLE/ProgramExecutable.h"
#include "libANGLE/renderer/ProgramExecutableImpl.h"

namespace rx
{
class ProgramExecutableD3D : public ProgramExecutableImpl
{
  public:
    ProgramExecutableD3D(const gl::ProgramExecutable *executable);
    ~ProgramExecutableD3D() override;

    void destroy(const gl::Context *context) override;

    // TODO: move all state from ProgramD3D here.  http://anglebug.com/8297
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_D3D_PROGRAMEXECUTABLED3D_H_
