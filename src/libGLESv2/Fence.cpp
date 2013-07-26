#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence.cpp: Implements the gl::Fence class, which supports the GL_NV_fence extension.

#include "libGLESv2/Fence.h"
#include "libGLESv2/renderer/FenceImpl.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/main.h"

namespace gl
{

Fence::Fence(rx::Renderer *renderer)
{
    mFence = renderer->createFence();
}

Fence::~Fence()
{
    delete mFence;
}

GLboolean Fence::isFence() const
{
    // GL_NV_fence spec:
    // A name returned by GenFencesNV, but not yet set via SetFenceNV, is not the name of an existing fence.
    return (mFence->isSet() ? GL_TRUE : GL_FALSE);
}

void Fence::setFence(GLenum condition)
{
    mFence->set();

    mCondition = condition;
    mStatus = GL_FALSE;
}

GLboolean Fence::testFence()
{
    // Flush the command buffer by default
    bool result = mFence->test(true);

    mStatus = (result ? GL_TRUE : GL_FALSE);
    return mStatus;
}

void Fence::finishFence()
{
    ASSERT(mFence->isSet());

    while (!mFence->test(true))
    {
        Sleep(0);
    }
}

GLint Fence::getFencei(GLenum pname)
{
    ASSERT(mFence->isSet());

    switch (pname)
    {
      case GL_FENCE_STATUS_NV:
        {
            // GL_NV_fence spec:
            // Once the status of a fence has been finished (via FinishFenceNV) or tested and the returned status is TRUE (via either TestFenceNV
            // or GetFenceivNV querying the FENCE_STATUS_NV), the status remains TRUE until the next SetFenceNV of the fence.
            if (mStatus == GL_TRUE)
            {
                return GL_TRUE;
            }

            mStatus = (mFence->test(false) ? GL_TRUE : GL_FALSE);
            return mStatus;
        }

      case GL_FENCE_CONDITION_NV:
        return mCondition;

      default: UNREACHABLE(); return 0;
    }
}

}
