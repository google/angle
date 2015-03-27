//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBANGLE_TRANSFORM_FEEDBACK_H_
#define LIBANGLE_TRANSFORM_FEEDBACK_H_

#include "libANGLE/RefCountObject.h"

#include "common/angleutils.h"

#include "angle_gl.h"

namespace rx
{
class TransformFeedbackImpl;
}

namespace gl
{

class TransformFeedback : public RefCountObject
{
  public:
    TransformFeedback(rx::TransformFeedbackImpl* impl, GLuint id);
    virtual ~TransformFeedback();

    void begin(GLenum primitiveMode);
    void end();
    void pause();
    void resume();

    bool isActive() const;
    bool isPaused() const;
    GLenum getPrimitiveMode() const;

    rx::TransformFeedbackImpl *getImplementation();
    const rx::TransformFeedbackImpl *getImplementation() const;

  private:
    rx::TransformFeedbackImpl* mImplementation;

    bool mActive;
    GLenum mPrimitiveMode;
    bool mPaused;
};

}

#endif // LIBANGLE_TRANSFORM_FEEDBACK_H_
