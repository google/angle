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
    if (!mFence->isSet())
    {
        return gl::error(GL_INVALID_OPERATION);
    }

    while (!mFence->test(true))
    {
        Sleep(0);
    }
}

void Fence::getFenceiv(GLenum pname, GLint *params)
{
    if (!mFence->isSet())
    {
        return error(GL_INVALID_OPERATION);
    }

    switch (pname)
    {
      case GL_FENCE_STATUS_NV:
        {
            // GL_NV_fence spec:
            // Once the status of a fence has been finished (via FinishFenceNV) or tested and the returned status is TRUE (via either TestFenceNV
            // or GetFenceivNV querying the FENCE_STATUS_NV), the status remains TRUE until the next SetFenceNV of the fence.
            if (mStatus == GL_TRUE)
            {
                params[0] = GL_TRUE;
                return;
            }

            mStatus = (mFence->test(false) ? GL_TRUE : GL_FALSE);
            params[0] = mStatus;
            break;
        }

      case GL_FENCE_CONDITION_NV:
        params[0] = mCondition;
        break;

      default:
        return error(GL_INVALID_ENUM);
    }
}

}
