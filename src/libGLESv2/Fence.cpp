//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence.cpp: Implements the gl::Fence class, which supports the GL_NV_fence extension.

// Important note on accurate timers in Windows:
//
// QueryPerformanceCounter has a few major issues, including being 10x as expensive to call
// as timeGetTime on laptops and "jumping" during certain hardware events.
//
// See the comments at the top of the Chromium source file "chromium/src/base/time/time_win.cc"
//   https://code.google.com/p/chromium/codesearch#chromium/src/base/time/time_win.cc
//
// We still opt to use QPC. In the present and moving forward, most newer systems will not suffer
// from buggy implementations.

#include "libGLESv2/Fence.h"
#include "libGLESv2/renderer/FenceImpl.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/main.h"

#include "angle_gl.h"

namespace gl
{

FenceNV::FenceNV(rx::Renderer *renderer)
    : mFence(renderer->createFence()),
      mIsSet(false),
      mStatus(GL_FALSE),
      mCondition(GL_NONE)
{
}

FenceNV::~FenceNV()
{
    SafeDelete(mFence);
}

GLboolean FenceNV::isFence() const
{
    // GL_NV_fence spec:
    // A name returned by GenFencesNV, but not yet set via SetFenceNV, is not the name of an existing fence.
    return (mIsSet ? GL_TRUE : GL_FALSE);
}

Error FenceNV::setFence(GLenum condition)
{
    Error error = mFence->set();
    if (error.isError())
    {
        return error;
    }

    mCondition = condition;
    mStatus = GL_FALSE;
    mIsSet = true;

    return Error(GL_NO_ERROR);
}

Error FenceNV::testFence(GLboolean *outResult)
{
    // Flush the command buffer by default
    Error error = mFence->test(true, &mStatus);
    if (error.isError())
    {
        return error;
    }

    *outResult = mStatus;
    return Error(GL_NO_ERROR);
}

Error FenceNV::finishFence()
{
    ASSERT(mIsSet);

    while (mStatus != GL_TRUE)
    {
        Error error = mFence->test(true, &mStatus);
        if (error.isError())
        {
            return error;
        }

        Sleep(0);
    }

    return Error(GL_NO_ERROR);
}

Error FenceNV::getFencei(GLenum pname, GLint *params)
{
    ASSERT(mIsSet);

    switch (pname)
    {
      case GL_FENCE_STATUS_NV:
        // GL_NV_fence spec:
        // Once the status of a fence has been finished (via FinishFenceNV) or tested and the returned status is TRUE (via either TestFenceNV
        // or GetFenceivNV querying the FENCE_STATUS_NV), the status remains TRUE until the next SetFenceNV of the fence.
        if (mStatus != GL_TRUE)
        {
            Error error = mFence->test(false, &mStatus);
            if (error.isError())
            {
                return error;
            }
        }
        *params = mStatus;
        break;

      case GL_FENCE_CONDITION_NV:
        *params = mCondition;
        break;

      default:
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION);
    }

    return Error(GL_NO_ERROR);
}

FenceSync::FenceSync(rx::Renderer *renderer, GLuint id)
    : RefCountObject(id),
      mFence(renderer->createFence()),
      mCounterFrequency(0),
      mCondition(GL_NONE)
{
    LARGE_INTEGER counterFreqency = { 0 };
    BOOL success = QueryPerformanceFrequency(&counterFreqency);
    UNUSED_ASSERTION_VARIABLE(success);
    ASSERT(success);

    mCounterFrequency = counterFreqency.QuadPart;
}

FenceSync::~FenceSync()
{
    SafeDelete(mFence);
}

Error FenceSync::set(GLenum condition)
{
    Error error = mFence->set();
    if (error.isError())
    {
        return error;
    }

    mCondition = condition;
    return Error(GL_NO_ERROR);
}

Error FenceSync::clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult)
{
    ASSERT(mCondition != GL_NONE);

    bool flushCommandBuffer = ((flags & GL_SYNC_FLUSH_COMMANDS_BIT) != 0);

    GLboolean result = GL_FALSE;
    Error error = mFence->test(flushCommandBuffer, &result);
    if (error.isError())
    {
        *outResult = GL_WAIT_FAILED;
        return error;
    }

    if (result == GL_TRUE)
    {
        *outResult = GL_ALREADY_SIGNALED;
        return Error(GL_NO_ERROR);
    }

    if (timeout == 0)
    {
        *outResult = GL_TIMEOUT_EXPIRED;
        return Error(GL_NO_ERROR);
    }

    LARGE_INTEGER currentCounter = { 0 };
    BOOL success = QueryPerformanceCounter(&currentCounter);
    UNUSED_ASSERTION_VARIABLE(success);
    ASSERT(success);

    LONGLONG timeoutInSeconds = static_cast<LONGLONG>(timeout) * static_cast<LONGLONG>(1000000ll);
    LONGLONG endCounter = currentCounter.QuadPart + mCounterFrequency * timeoutInSeconds;

    while (currentCounter.QuadPart < endCounter && !result)
    {
        Sleep(0);
        BOOL success = QueryPerformanceCounter(&currentCounter);
        UNUSED_ASSERTION_VARIABLE(success);
        ASSERT(success);

        error = mFence->test(flushCommandBuffer, &result);
        if (error.isError())
        {
            *outResult = GL_WAIT_FAILED;
            return error;
        }
    }

    if (currentCounter.QuadPart >= endCounter)
    {
        *outResult = GL_TIMEOUT_EXPIRED;
    }
    else
    {
        *outResult = GL_CONDITION_SATISFIED;
    }

    return Error(GL_NO_ERROR);
}

Error FenceSync::serverWait()
{
    // Because our API is currently designed to be called from a single thread, we don't need to do
    // extra work for a server-side fence. GPU commands issued after the fence is created will always
    // be processed after the fence is signaled.
    return Error(GL_NO_ERROR);
}

Error FenceSync::getStatus(GLint *outResult) const
{
    GLboolean result = GL_FALSE;
    Error error = mFence->test(false, &result);
    if (error.isError())
    {
        // The spec does not specify any way to report errors during the status test (e.g. device lost)
        // so we report the fence is unblocked in case of error or signaled.
        *outResult = GL_SIGNALED;

        return error;
    }

    *outResult = (result ? GL_SIGNALED : GL_UNSIGNALED);
    return Error(GL_NO_ERROR);
}

}
