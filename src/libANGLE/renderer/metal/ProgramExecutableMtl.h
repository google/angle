//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramExecutableMtl.h: Implementation of ProgramExecutableImpl.

#ifndef LIBANGLE_RENDERER_MTL_PROGRAMEXECUTABLEMTL_H_
#define LIBANGLE_RENDERER_MTL_PROGRAMEXECUTABLEMTL_H_

#include "libANGLE/ProgramExecutable.h"
#include "libANGLE/renderer/ProgramExecutableImpl.h"

namespace rx
{
class ProgramExecutableMtl : public ProgramExecutableImpl
{
  public:
    ProgramExecutableMtl(const gl::ProgramExecutable *executable);
    ~ProgramExecutableMtl() override;

    void destroy(const gl::Context *context) override;

    // TODO: move all state from ProgramMtl here.  http://anglebug.com/8297
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_MTL_PROGRAMEXECUTABLEMTL_H_
