//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManagerGL.h: Defines a class for caching applied OpenGL state

#include "libANGLE/renderer/gl/StateManagerGL.h"

#include "libANGLE/renderer/gl/FunctionsGL.h"

namespace rx
{

StateManagerGL::StateManagerGL(const FunctionsGL *functions)
    : mFunctions(functions),
      mProgram(0)
{
    ASSERT(mFunctions);
}

void StateManagerGL::setProgram(GLuint program)
{
    if (mProgram != program)
    {
        mProgram = program;
        mFunctions->useProgram(mProgram);
    }
}

}
