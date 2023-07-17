//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayImpl.h: Implementation methods of egl::Display

#ifndef LIBANGLE_RENDERER_SHAREGROUPIMPL_H_
#define LIBANGLE_RENDERER_SHAREGROUPIMPL_H_

#include "common/angleutils.h"

namespace egl
{
class Display;
}  // namespace egl

namespace gl
{
class Context;
}  // namespace gl

namespace rx
{
class ShareGroupImpl : angle::NonCopyable
{
  public:
    ShareGroupImpl() : mAnyContextWithRobustness(false) {}
    virtual ~ShareGroupImpl() {}
    virtual void onDestroy(const egl::Display *display) {}

    void onRobustContextAdd() { mAnyContextWithRobustness = true; }
    bool hasAnyContextWithRobustness() const { return mAnyContextWithRobustness; }

  private:
    // Whether any context in the share group has robustness enabled.  If any context in the share
    // group is robust, any program created in any context of the share group must have robustness
    // enabled.  This is because programs are shared between the share group contexts.
    bool mAnyContextWithRobustness;
};
}  // namespace rx

#endif  // LIBANGLE_RENDERER_SHAREGROUPIMPL_H_
