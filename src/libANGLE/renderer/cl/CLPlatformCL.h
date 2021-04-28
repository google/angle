//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformCL.h:
//    Defines the class interface for CLPlatformCL, implementing CLPlatformImpl.
//

#ifndef LIBANGLE_RENDERER_CL_CLPLATFORMCL_H_
#define LIBANGLE_RENDERER_CL_CLPLATFORMCL_H_

#include "libANGLE/renderer/CLPlatformImpl.h"

#include <string>

namespace rx
{

class CLPlatformCL : public CLPlatformImpl
{
  public:
    ~CLPlatformCL() override;

    cl_platform_id getNative();

    static ImplList GetPlatforms(bool isIcd);

  private:
    CLPlatformCL(cl_platform_id platform, Info &&info);

    static std::unique_ptr<CLPlatformCL> Create(cl_platform_id platform);

    const cl_platform_id mPlatform;
};

inline cl_platform_id CLPlatformCL::getNative()
{
    return mPlatform;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLPLATFORMCL_H_
