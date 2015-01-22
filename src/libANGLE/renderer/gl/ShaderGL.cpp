//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderGL.cpp: Implements the class methods for ShaderGL.

#include "libANGLE/renderer/gl/ShaderGL.h"

#include "common/debug.h"

namespace rx
{

ShaderGL::ShaderGL()
    : ShaderImpl()
{}

ShaderGL::~ShaderGL()
{}

bool ShaderGL::compile(gl::Compiler *compiler, const std::string &source)
{
    UNIMPLEMENTED();
    return bool();
}

std::string ShaderGL::getDebugInfo() const
{
    UNIMPLEMENTED();
    return std::string();
}

}
