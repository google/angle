//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDevice.h: Defines the cl::Device class, which provides information about OpenCL device
// configurations.

#ifndef LIBANGLE_CLDEVICE_H_
#define LIBANGLE_CLDEVICE_H_

#include "libANGLE/CLObject.h"
#include "libANGLE/renderer/CLDeviceImpl.h"

namespace cl
{

class Device final : public _cl_device_id, public Object
{
  public:
    using Ptr     = std::unique_ptr<Device>;
    using PtrList = std::list<Ptr>;

    ~Device();

    Platform &getPlatform() const;
    bool isRoot() const;
    bool hasSubDevice(const Device *device) const;

    void retain();
    bool release();

    cl_int getInfoULong(DeviceInfo name, cl_ulong *value) const;
    cl_int getInfo(DeviceInfo name, size_t valueSize, void *value, size_t *valueSizeRet);

    cl_int createSubDevices(const cl_device_partition_property *properties,
                            cl_uint numDevices,
                            Device **devices,
                            cl_uint *numDevicesRet);

    static PtrList CreateDevices(Platform &platform, rx::CLDeviceImpl::InitList &&initList);

    static bool IsValid(const Device *device);
    static bool IsValidType(cl_device_type type);

  private:
    Device(Platform &platform, Device *parent, rx::CLDeviceImpl::InitData &initData);

    void destroySubDevice(Device *device);

    Platform &mPlatform;
    Device *const mParent;
    const rx::CLDeviceImpl::Ptr mImpl;
    const rx::CLDeviceImpl::Info mInfo;

    PtrList mSubDevices;
};

inline Platform &Device::getPlatform() const
{
    return mPlatform;
}

inline bool Device::isRoot() const
{
    return mParent == nullptr;
}

inline bool Device::hasSubDevice(const Device *device) const
{
    return std::find_if(mSubDevices.cbegin(), mSubDevices.cend(), [=](const Device::Ptr &ptr) {
               return ptr.get() == device || ptr->hasSubDevice(device);
           }) != mSubDevices.cend();
}

inline void Device::retain()
{
    if (!isRoot())
    {
        addRef();
    }
}

inline cl_int Device::getInfoULong(DeviceInfo name, cl_ulong *value) const
{
    return mImpl->getInfoULong(name, value);
}

inline bool Device::IsValidType(cl_device_type type)
{
    return type <= CL_DEVICE_TYPE_CUSTOM || type == CL_DEVICE_TYPE_ALL;
}

}  // namespace cl

#endif  // LIBANGLE_CLDEVICE_H_
