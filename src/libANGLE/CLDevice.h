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

#include <functional>

namespace cl
{

class Device final : public _cl_device_id, public Object
{
  public:
    using CreateImplFunc = std::function<rx::CLDeviceImpl::Ptr(const cl::Device &)>;

    ~Device();

    Platform &getPlatform() const noexcept;
    bool isRoot() const noexcept;
    rx::CLDeviceImpl &getImpl() const;
    const rx::CLDeviceImpl::Info &getInfo() const;
    bool hasSubDevice(const _cl_device_id *device) const;

    void retain() noexcept;
    bool release();

    cl_int getInfoULong(DeviceInfo name, cl_ulong *value) const;
    cl_int getInfo(DeviceInfo name, size_t valueSize, void *value, size_t *valueSizeRet);

    cl_int createSubDevices(const cl_device_partition_property *properties,
                            cl_uint numDevices,
                            cl_device_id *subDevices,
                            cl_uint *numDevicesRet);

    static DevicePtr CreateDevice(Platform &platform,
                                  DeviceRefPtr &&parent,
                                  const CreateImplFunc &createImplFunc);

    static bool IsValid(const _cl_device_id *device);
    static bool IsValidType(cl_device_type type);

  private:
    Device(Platform &platform, DeviceRefPtr &&parent, const CreateImplFunc &createImplFunc);

    void destroySubDevice(Device *device);

    Platform &mPlatform;
    const DeviceRefPtr mParent;
    const rx::CLDeviceImpl::Ptr mImpl;
    const rx::CLDeviceImpl::Info mInfo;

    DevicePtrList mSubDevices;

    friend class Platform;
};

inline Platform &Device::getPlatform() const noexcept
{
    return mPlatform;
}

inline bool Device::isRoot() const noexcept
{
    return !mParent;
}

inline rx::CLDeviceImpl &Device::getImpl() const
{
    return *mImpl;
}

inline const rx::CLDeviceImpl::Info &Device::getInfo() const
{
    return mInfo;
}

inline bool Device::hasSubDevice(const _cl_device_id *device) const
{
    return std::find_if(mSubDevices.cbegin(), mSubDevices.cend(), [=](const DevicePtr &ptr) {
               return ptr.get() == device || ptr->hasSubDevice(device);
           }) != mSubDevices.cend();
}

inline void Device::retain() noexcept
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
