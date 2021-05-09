//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformCL.h: Defines the class interface for CLPlatformCL, implementing CLPlatformImpl.

#ifndef LIBANGLE_RENDERER_CL_CLPLATFORMCL_H_
#define LIBANGLE_RENDERER_CL_CLPLATFORMCL_H_

#include "libANGLE/renderer/CLPlatformImpl.h"

namespace rx
{

class CLPlatformCL : public CLPlatformImpl
{
  public:
    ~CLPlatformCL() override;

    cl_platform_id getNative();

    CLContextImpl::Ptr createContext(CLDeviceImpl::List &&deviceImplList,
                                     cl::ContextErrorCB notify,
                                     void *userData,
                                     bool userSync,
                                     cl_int *errcodeRet) override;

    CLContextImpl::Ptr createContextFromType(cl_device_type deviceType,
                                             cl::ContextErrorCB notify,
                                             void *userData,
                                             bool userSync,
                                             cl_int *errcodeRet) override;

    static InitList GetPlatforms(bool isIcd);

  private:
    CLPlatformCL(cl_platform_id platform, cl_version version, CLDeviceImpl::PtrList &devices);

    static Info GetInfo(cl_platform_id platform);

    const cl_platform_id mPlatform;
    const cl_version mVersion;

    friend class CLContextCL;
};

inline cl_platform_id CLPlatformCL::getNative()
{
    return mPlatform;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLPLATFORMCL_H_
