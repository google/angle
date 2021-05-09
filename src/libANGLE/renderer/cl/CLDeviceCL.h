//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceCL.h: Defines the class interface for CLDeviceCL, implementing CLDeviceImpl.

#ifndef LIBANGLE_RENDERER_CL_CLDEVICECL_H_
#define LIBANGLE_RENDERER_CL_CLDEVICECL_H_

#include "libANGLE/renderer/cl/cl_types.h"

#include "libANGLE/renderer/CLDeviceImpl.h"

namespace rx
{

class CLDeviceCL : public CLDeviceImpl
{
  public:
    ~CLDeviceCL() override;

    cl_device_id getNative();

    Info createInfo() const override;

    cl_int getInfoUInt(cl::DeviceInfo name, cl_uint *value) const override;
    cl_int getInfoULong(cl::DeviceInfo name, cl_ulong *value) const override;
    cl_int getInfoSizeT(cl::DeviceInfo name, size_t *value) const override;
    cl_int getInfoStringLength(cl::DeviceInfo name, size_t *value) const override;
    cl_int getInfoString(cl::DeviceInfo name, size_t size, char *value) const override;

    cl_int createSubDevices(const cl_device_partition_property *properties,
                            cl_uint numDevices,
                            PtrList &implList,
                            cl_uint *numDevicesRet) override;

    static CLDeviceCL *Create(CLPlatformCL &platform, CLDeviceCL *parent, cl_device_id device);

  private:
    CLDeviceCL(CLPlatformCL &platform, CLDeviceCL *parent, cl_device_id device, cl_version version);

    const cl_device_id mDevice;
    const cl_version mVersion;
};

inline cl_device_id CLDeviceCL::getNative()
{
    return mDevice;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLDEVICECL_H_
