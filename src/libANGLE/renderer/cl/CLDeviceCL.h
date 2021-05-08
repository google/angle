//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceCL.h: Defines the class interface for CLDeviceCL, implementing CLDeviceImpl.

#ifndef LIBANGLE_RENDERER_CL_CLDEVICECL_H_
#define LIBANGLE_RENDERER_CL_CLDEVICECL_H_

#include "libANGLE/renderer/CLDeviceImpl.h"

namespace rx
{

class CLDeviceCL : public CLDeviceImpl
{
  public:
    explicit CLDeviceCL(cl_device_id device);
    ~CLDeviceCL() override;

    cl_device_id getNative();

    cl_int getInfoUInt(cl::DeviceInfo name, cl_uint *value) const override;
    cl_int getInfoULong(cl::DeviceInfo name, cl_ulong *value) const override;
    cl_int getInfoSizeT(cl::DeviceInfo name, size_t *value) const override;
    cl_int getInfoStringLength(cl::DeviceInfo name, size_t *value) const override;
    cl_int getInfoString(cl::DeviceInfo name, size_t size, char *value) const override;

    static Info GetInfo(cl_device_id device);

  private:
    const cl_device_id mDevice;
};

inline cl_device_id CLDeviceCL::getNative()
{
    return mDevice;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLDEVICECL_H_
