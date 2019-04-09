// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MemoryObjectVk.h: Defines the class interface for MemoryObjectVk,
// implementing MemoryObjectImpl.

#ifndef LIBANGLE_RENDERER_VULKAN_MEMORYOBJECTVK_H_
#define LIBANGLE_RENDERER_VULKAN_MEMORYOBJECTVK_H_

#include "libANGLE/renderer/MemoryObjectImpl.h"

namespace rx
{

class MemoryObjectVk : public MemoryObjectImpl
{
  public:
    MemoryObjectVk();
    ~MemoryObjectVk() override;

    void onDestroy(const gl::Context *context) override;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_MEMORYOBJECTVK_H_
