//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TransformFeedbackGL.cpp: Implements the class methods for TransformFeedbackGL.

#include "libANGLE/renderer/gl/TransformFeedbackGL.h"

#include "common/debug.h"
#include "libANGLE/State.h"
#include "libANGLE/renderer/gl/BufferGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/StateManagerGL.h"

namespace rx
{

TransformFeedbackGL::TransformFeedbackGL(const gl::TransformFeedbackState &state,
                                         const FunctionsGL *functions,
                                         StateManagerGL *stateManager)
    : TransformFeedbackImpl(state),
      mFunctions(functions),
      mStateManager(stateManager),
      mTransformFeedbackID(0),
      mIsActive(false),
      mIsPaused(false)
{
    mFunctions->genTransformFeedbacks(1, &mTransformFeedbackID);
}

TransformFeedbackGL::~TransformFeedbackGL()
{
    mStateManager->deleteTransformFeedback(mTransformFeedbackID);
    mTransformFeedbackID = 0;
}

angle::Result TransformFeedbackGL::begin(const gl::Context *context,
                                         gl::PrimitiveMode primitiveMode)
{
    mStateManager->onTransformFeedbackStateChange();
    return angle::Result::Continue;
}

angle::Result TransformFeedbackGL::end(const gl::Context *context)
{
    mStateManager->onTransformFeedbackStateChange();

    // Immediately end the transform feedback so that the results are visible.
    syncActiveState(false, gl::PrimitiveMode::InvalidEnum);
    return angle::Result::Continue;
}

angle::Result TransformFeedbackGL::pause(const gl::Context *context)
{
    mStateManager->onTransformFeedbackStateChange();

    syncPausedState(true);
    return angle::Result::Continue;
}

angle::Result TransformFeedbackGL::resume(const gl::Context *context)
{
    mStateManager->onTransformFeedbackStateChange();
    return angle::Result::Continue;
}

angle::Result TransformFeedbackGL::bindGenericBuffer(const gl::Context *context,
                                                     const gl::BindingPointer<gl::Buffer> &binding)
{
    return angle::Result::Continue;
}

angle::Result TransformFeedbackGL::bindIndexedBuffer(
    const gl::Context *context,
    size_t index,
    const gl::OffsetBindingPointer<gl::Buffer> &binding)
{
    // Directly bind buffer (not through the StateManager methods) because the buffer bindings are
    // tracked per transform feedback object
    mStateManager->bindTransformFeedback(GL_TRANSFORM_FEEDBACK, mTransformFeedbackID);
    if (binding.get() != nullptr)
    {
        const BufferGL *bufferGL = GetImplAs<BufferGL>(binding.get());
        if (binding.getSize() != 0)
        {
            mFunctions->bindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, static_cast<GLuint>(index),
                                        bufferGL->getBufferID(), binding.getOffset(),
                                        binding.getSize());
        }
        else
        {
            mFunctions->bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, static_cast<GLuint>(index),
                                       bufferGL->getBufferID());
        }
    }
    else
    {
        mFunctions->bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, static_cast<GLuint>(index), 0);
    }
    return angle::Result::Continue;
}

GLuint TransformFeedbackGL::getTransformFeedbackID() const
{
    return mTransformFeedbackID;
}

void TransformFeedbackGL::syncActiveState(bool active, gl::PrimitiveMode primitiveMode) const
{
    if (mIsActive != active)
    {
        mIsActive = active;
        mIsPaused = false;

        mStateManager->bindTransformFeedback(GL_TRANSFORM_FEEDBACK, mTransformFeedbackID);
        if (mIsActive)
        {
            ASSERT(primitiveMode != gl::PrimitiveMode::InvalidEnum);
            mFunctions->beginTransformFeedback(gl::ToGLenum(primitiveMode));
        }
        else
        {
            mFunctions->endTransformFeedback();
        }
    }
}

void TransformFeedbackGL::syncPausedState(bool paused) const
{
    if (mIsActive && mIsPaused != paused)
    {
        mIsPaused = paused;

        mStateManager->bindTransformFeedback(GL_TRANSFORM_FEEDBACK, mTransformFeedbackID);
        if (mIsPaused)
        {
            mFunctions->pauseTransformFeedback();
        }
        else
        {
            mFunctions->resumeTransformFeedback();
        }
    }
}
}  // namespace rx
