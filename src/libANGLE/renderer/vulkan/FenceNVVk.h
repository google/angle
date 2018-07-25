//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FenceNVVk.h:
//    Defines the class interface for FenceNVVk, implementing FenceNVImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_FENCENVVK_H_
#define LIBANGLE_RENDERER_VULKAN_FENCENVVK_H_

#include "libANGLE/renderer/FenceNVImpl.h"

namespace rx
{
class FenceNVVk : public FenceNVImpl
{
  public:
    FenceNVVk();
    ~FenceNVVk() override;

    gl::Error set(const gl::Context *context, GLenum condition) override;
    gl::Error test(const gl::Context *context, GLboolean *outFinished) override;
    gl::Error finish(const gl::Context *context) override;
};
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_FENCENVVK_H_
