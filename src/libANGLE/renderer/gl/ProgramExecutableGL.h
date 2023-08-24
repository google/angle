//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramExecutableGL.h: Implementation of ProgramExecutableImpl.

#ifndef LIBANGLE_RENDERER_GL_PROGRAMEXECUTABLEGL_H_
#define LIBANGLE_RENDERER_GL_PROGRAMEXECUTABLEGL_H_

#include "libANGLE/ProgramExecutable.h"
#include "libANGLE/renderer/ProgramExecutableImpl.h"

namespace rx
{
class ProgramExecutableGL : public ProgramExecutableImpl
{
  public:
    ProgramExecutableGL(const gl::ProgramExecutable *executable);
    ~ProgramExecutableGL() override;

    void destroy(const gl::Context *context) override;

    // TODO: move relevant state from ProgramGL here.  http://anglebug.com/8297
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_GL_PROGRAMEXECUTABLEGL_H_
