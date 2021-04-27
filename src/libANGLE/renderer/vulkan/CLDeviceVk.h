//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceVk.h:
//    Defines the class interface for CLDeviceVk, implementing CLDeviceImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_CLDEVICEVK_H_
#define LIBANGLE_RENDERER_VULKAN_CLDEVICEVK_H_

#include "libANGLE/renderer/CLDeviceImpl.h"

namespace rx
{

class CLDeviceVk : public CLDeviceImpl
{
  public:
    CLDeviceVk();
    ~CLDeviceVk() override;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_CLDEVICEVK_H_
