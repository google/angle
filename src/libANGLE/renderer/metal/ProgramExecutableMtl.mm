//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramExecutableMtl.cpp: Implementation of ProgramExecutableMtl.

#include "libANGLE/renderer/metal/ProgramExecutableMtl.h"

namespace rx
{
ProgramExecutableMtl::ProgramExecutableMtl(const gl::ProgramExecutable *executable)
    : ProgramExecutableImpl(executable)
{}

ProgramExecutableMtl::~ProgramExecutableMtl() {}

void ProgramExecutableMtl::destroy(const gl::Context *context) {}
}  // namespace rx
