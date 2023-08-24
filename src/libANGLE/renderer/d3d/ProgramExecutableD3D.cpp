//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramExecutableD3D.cpp: Implementation of ProgramExecutableD3D.

#include "libANGLE/renderer/d3d/ProgramExecutableD3D.h"

namespace rx
{
ProgramExecutableD3D::ProgramExecutableD3D(const gl::ProgramExecutable *executable)
    : ProgramExecutableImpl(executable)
{}

ProgramExecutableD3D::~ProgramExecutableD3D() {}

void ProgramExecutableD3D::destroy(const gl::Context *context) {}
}  // namespace rx
