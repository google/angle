//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CompilerGL.cpp: Implements the class methods for CompilerGL.

#include "libANGLE/renderer/gl/CompilerGL.h"

#include "common/debug.h"

namespace rx
{

CompilerGL::CompilerGL()
    : CompilerImpl()
{}

CompilerGL::~CompilerGL()
{}

gl::Error CompilerGL::release()
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

}
