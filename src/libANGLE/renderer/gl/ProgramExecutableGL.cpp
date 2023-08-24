//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramExecutableGL.cpp: Implementation of ProgramExecutableGL.

#include "libANGLE/renderer/gl/ProgramExecutableGL.h"

namespace rx
{
ProgramExecutableGL::ProgramExecutableGL(const gl::ProgramExecutable *executable)
    : ProgramExecutableImpl(executable)
{}

ProgramExecutableGL::~ProgramExecutableGL() {}

void ProgramExecutableGL::destroy(const gl::Context *context) {}
}  // namespace rx
