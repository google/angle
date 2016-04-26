//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextImpl:
//   Implementation-specific functionality associated with a GL Context.
//

#ifndef LIBANGLE_RENDERER_CONTEXTIMPL_H_
#define LIBANGLE_RENDERER_CONTEXTIMPL_H_

#include "common/angleutils.h"
#include "libANGLE/ContextState.h"

namespace rx
{
class Renderer;

class ContextImpl : angle::NonCopyable
{
  public:
    ContextImpl(const gl::ContextState &state) : mState(state) {}
    virtual ~ContextImpl() {}

    virtual gl::Error initialize(Renderer *renderer) = 0;

    int getClientVersion() const { return mState.clientVersion; }
    const gl::State &getState() const { return *mState.state; }
    const gl::Caps &getCaps() const { return *mState.caps; }
    const gl::TextureCapsMap &getTextureCaps() const { return *mState.textureCaps; }
    const gl::Extensions &getExtensions() const { return *mState.extensions; }
    const gl::Limitations &getLimitations() const { return *mState.limitations; }

  private:
    const gl::ContextState &mState;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CONTEXTIMPL_H_
