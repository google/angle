//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformVk.h:
//    Defines the class interface for CLPlatformVk, implementing CLPlatformImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_CLPLATFORMVK_H_
#define LIBANGLE_RENDERER_VULKAN_CLPLATFORMVK_H_

#include "libANGLE/renderer/CLPlatformImpl.h"

#include <string>

namespace rx
{

class CLPlatformVk : public CLPlatformImpl
{
  public:
    ~CLPlatformVk() override;

    static ImplList GetPlatforms();
    static constexpr cl_version GetVersion();
    static const std::string &GetVersionString();

  private:
    explicit CLPlatformVk(Info &&info);
};

constexpr cl_version CLPlatformVk::GetVersion()
{
    return CL_MAKE_VERSION(1, 2, 0);
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_CLPLATFORMVK_H_
