//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RendererMtl.h:
//    Defines class interface for RendererMtl.

#ifndef LIBANGLE_RENDERER_METAL_RENDERERMTL_H_
#define LIBANGLE_RENDERER_METAL_RENDERERMTL_H_

#import <Metal/Metal.h>

#include "common/PackedEnums.h"
#include "libANGLE/Caps.h"
#include "libANGLE/angletypes.h"

namespace egl
{
class Display;
}

namespace rx
{

class ContextMtl;

class RendererMtl final : angle::NonCopyable
{
  public:
    RendererMtl();
    ~RendererMtl();

    angle::Result initialize(egl::Display *display);
    void onDestroy();

    std::string getVendorString() const;
    std::string getRendererDescription() const;
    const gl::Limitations &getNativeLimitations() const;

    id<MTLDevice> getMetalDevice() const { return nil; }

  private:
    gl::Limitations mNativeLimitations;
};
}  // namespace rx

#endif /* LIBANGLE_RENDERER_METAL_RENDERERMTL_H_ */
