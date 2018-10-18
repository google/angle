//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// FenceNVGL.cpp: Implements the class methods for FenceNVGL.

#include "libANGLE/renderer/gl/FenceNVGL.h"

#include "common/debug.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"


namespace rx
{

FenceNVGL::FenceNVGL(const FunctionsGL *functions)
    : FenceNVImpl(),
      mFunctions(functions)
{
    mFunctions->genFencesNV(1, &mFence);
}

FenceNVGL::~FenceNVGL()
{
    mFunctions->deleteFencesNV(1, &mFence);
    mFence = 0;
}

angle::Result FenceNVGL::set(const gl::Context *context, GLenum condition)
{
    ASSERT(condition == GL_ALL_COMPLETED_NV);
    mFunctions->setFenceNV(mFence, condition);
    return angle::Result::Continue();
}

angle::Result FenceNVGL::test(const gl::Context *context, GLboolean *outFinished)
{
    ASSERT(outFinished);
    *outFinished = mFunctions->testFenceNV(mFence);
    return angle::Result::Continue();
}

angle::Result FenceNVGL::finish(const gl::Context *context)
{
    mFunctions->finishFenceNV(mFence);
    return angle::Result::Continue();
}

}
