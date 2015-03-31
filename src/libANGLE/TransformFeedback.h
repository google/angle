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

    void start(GLenum primitiveMode);
    void stop();
    GLboolean isStarted() const;

    GLenum getDrawMode() const;

    void pause();
    void resume();
    GLboolean isPaused() const;

  private:
    rx::TransformFeedbackImpl* mTransformFeedback;

    GLboolean mStarted;
    GLenum mPrimitiveMode;
    GLboolean mPaused;
};

}

#endif // LIBANGLE_TRANSFORM_FEEDBACK_H_
