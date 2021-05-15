//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformVk.h: Defines the class interface for CLPlatformVk, implementing CLPlatformImpl.

#ifndef LIBANGLE_RENDERER_VULKAN_CLPLATFORMVK_H_
#define LIBANGLE_RENDERER_VULKAN_CLPLATFORMVK_H_

#include "libANGLE/renderer/CLPlatformImpl.h"

namespace rx
{

class CLPlatformVk : public CLPlatformImpl
{
  public:
    ~CLPlatformVk() override;

    Info createInfo() const override;
    cl::DevicePtrList createDevices(cl::Platform &platform) const override;

    CLContextImpl::Ptr createContext(const cl::Context &context,
                                     const cl::DeviceRefList &devices,
                                     cl::ContextErrorCB notify,
                                     void *userData,
                                     bool userSync,
                                     cl_int *errcodeRet) override;

    CLContextImpl::Ptr createContextFromType(const cl::Context &context,
                                             cl_device_type deviceType,
                                             cl::ContextErrorCB notify,
                                             void *userData,
                                             bool userSync,
                                             cl_int *errcodeRet) override;

    static void Initialize(const cl_icd_dispatch &dispatch);

    static constexpr cl_version GetVersion();
    static const std::string &GetVersionString();

  private:
    explicit CLPlatformVk(const cl::Platform &platform);
};

constexpr cl_version CLPlatformVk::GetVersion()
{
    return CL_MAKE_VERSION(1, 2, 0);
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_CLPLATFORMVK_H_
