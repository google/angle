//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/TransformFeedback.h"
#include "libANGLE/renderer/TransformFeedbackImpl.h"

namespace gl
{

TransformFeedback::TransformFeedback(rx::TransformFeedbackImpl* impl, GLuint id)
    : RefCountObject(id),
      mImplementation(impl),
      mActive(false),
      mPrimitiveMode(GL_NONE),
      mPaused(false)
{
    ASSERT(impl != NULL);
}

TransformFeedback::~TransformFeedback()
{
    SafeDelete(mImplementation);
}

void TransformFeedback::begin(GLenum primitiveMode)
{
    mActive = true;
    mPrimitiveMode = primitiveMode;
    mPaused = false;
    mImplementation->begin(primitiveMode);
}

void TransformFeedback::end()
{
    mActive = false;
    mPrimitiveMode = GL_NONE;
    mPaused = false;
    mImplementation->end();
}

void TransformFeedback::pause()
{
    mPaused = true;
    mImplementation->pause();
}

void TransformFeedback::resume()
{
    mPaused = false;
    mImplementation->resume();
}

bool TransformFeedback::isActive() const
{
    return mActive;
}

bool TransformFeedback::isPaused() const
{
    return mPaused;
}

GLenum TransformFeedback::getPrimitiveMode() const
{
    return mPrimitiveMode;
}

rx::TransformFeedbackImpl *TransformFeedback::getImplementation()
{
    return mImplementation;
}

const rx::TransformFeedbackImpl *TransformFeedback::getImplementation() const
{
    return mImplementation;
}

}
