//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TransformFeedbackD3D.h: Implements the abstract rx::TransformFeedbackImpl class.

#ifndef LIBANGLE_RENDERER_D3D_TRANSFORMFEEDBACKD3D_H_
#define LIBANGLE_RENDERER_D3D_TRANSFORMFEEDBACKD3D_H_

#include "libANGLE/renderer/TransformFeedbackImpl.h"
#include "libANGLE/angletypes.h"

namespace rx
{

class TransformFeedbackD3D : public TransformFeedbackImpl
{
  public:
    TransformFeedbackD3D();
    virtual ~TransformFeedbackD3D();

    virtual void begin(GLenum primitiveMode);
    virtual void end();
    virtual void pause();
    virtual void resume();
};

}

#endif // LIBANGLE_RENDERER_D3D_TRANSFORMFEEDBACKD3D_H_
