//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence.h: Defines the gl::Fence class, which supports the GL_NV_fence extension.

#ifndef LIBGLESV2_FENCE_H_
#define LIBGLESV2_FENCE_H_

#include "libGLESv2/Error.h"

#include "common/angleutils.h"
#include "common/RefCountObject.h"

namespace rx
{
class Renderer;
class FenceImpl;
}

namespace gl
{

class FenceNV
{
  public:
    explicit FenceNV(rx::Renderer *renderer);
    virtual ~FenceNV();

    GLboolean isFence() const;
    Error setFence(GLenum condition);
    Error testFence(GLboolean *outResult);
    Error finishFence();
    Error getFencei(GLenum pname, GLint *params);

    GLboolean getStatus() const { return mStatus; }
    GLuint getCondition() const { return mCondition; }

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceNV);

    rx::FenceImpl *mFence;

    bool mIsSet;

    GLboolean mStatus;
    GLenum mCondition;
};

class FenceSync : public RefCountObject
{
  public:
    explicit FenceSync(rx::Renderer *renderer, GLuint id);
    virtual ~FenceSync();

    Error set(GLenum condition);
    Error clientWait(GLbitfield flags, GLuint64 timeout, GLenum *outResult);
    Error serverWait();
    Error getStatus(GLint *outResult) const;

    GLuint getCondition() const { return mCondition; }

  private:
    DISALLOW_COPY_AND_ASSIGN(FenceSync);

    rx::FenceImpl *mFence;
    LONGLONG mCounterFrequency;

    GLenum mCondition;
};

}

#endif   // LIBGLESV2_FENCE_H_
