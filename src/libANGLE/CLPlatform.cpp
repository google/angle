//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.cpp: Implements the cl::Platform class.

#include "libANGLE/CLPlatform.h"

#include <cstring>

namespace cl
{

namespace
{
bool IsDeviceTypeMatch(cl_device_type select, cl_device_type type)
{
    // The type 'cl_device_type' is a bitfield, so masking out the selected bits indicates
    // if a given type is a match. A custom device is an exception, which only matches
    // if it was explicitely selected, as defined here:
    // https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceIDs
    return type == CL_DEVICE_TYPE_CUSTOM ? select == CL_DEVICE_TYPE_CUSTOM : (type & select) != 0u;
}
}  // namespace

Platform::~Platform() = default;

cl_int Platform::getInfo(PlatformInfo name, size_t valueSize, void *value, size_t *sizeRet)
{
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case PlatformInfo::Profile:
            copyValue = mInfo.mProfile.c_str();
            copySize  = mInfo.mProfile.length() + 1u;
            break;
        case PlatformInfo::Version:
            copyValue = mInfo.mVersionStr.c_str();
            copySize  = mInfo.mVersionStr.length() + 1u;
            break;
        case PlatformInfo::NumericVersion:
            copyValue = &mInfo.mVersion;
            copySize  = sizeof(mInfo.mVersion);
            break;
        case PlatformInfo::Name:
            copyValue = mInfo.mName.c_str();
            copySize  = mInfo.mName.length() + 1u;
            break;
        case PlatformInfo::Vendor:
            copyValue = kVendor;
            copySize  = sizeof(kVendor);
            break;
        case PlatformInfo::Extensions:
            copyValue = mInfo.mExtensions.c_str();
            copySize  = mInfo.mExtensions.length() + 1u;
            break;
        case PlatformInfo::ExtensionsWithVersion:
            if (mInfo.mExtensionsWithVersion.empty())
            {
                return CL_INVALID_VALUE;
            }
            copyValue = mInfo.mExtensionsWithVersion.data();
            copySize  = mInfo.mExtensionsWithVersion.size() *
                       sizeof(decltype(mInfo.mExtensionsWithVersion)::value_type);
            break;
        case PlatformInfo::HostTimerResolution:
            copyValue = &mInfo.mHostTimerRes;
            copySize  = sizeof(mInfo.mHostTimerRes);
            break;
        case PlatformInfo::IcdSuffix:
            copyValue = kIcdSuffix;
            copySize  = sizeof(kIcdSuffix);
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        if (valueSize < copySize)
        {
            return CL_INVALID_VALUE;
        }
        if (copyValue != nullptr)
        {
            std::memcpy(value, copyValue, copySize);
        }
    }
    if (sizeRet != nullptr)
    {
        *sizeRet = copySize;
    }
    return CL_SUCCESS;
}

cl_int Platform::getDeviceIDs(cl_device_type deviceType,
                              cl_uint numEntries,
                              Device **devices,
                              cl_uint *numDevices) const
{
    cl_uint found = 0u;
    for (const Device::Ptr &device : mDevices)
    {
        cl_device_type type = 0u;
        if (device->getInfoULong(DeviceInfo::Type, &type) == CL_SUCCESS &&
            IsDeviceTypeMatch(deviceType, type))
        {
            if (devices != nullptr && found < numEntries)
            {
                devices[found] = device.get();
            }
            ++found;
        }
    }
    if (numDevices != nullptr)
    {
        *numDevices = found;
    }
    return found == 0u ? CL_DEVICE_NOT_FOUND : CL_SUCCESS;
}

void Platform::CreatePlatform(const cl_icd_dispatch &dispatch,
                              rx::CLPlatformImpl::InitData &initData)
{
    rx::CLDeviceImpl::InitList deviceInitList = initData.first->getDevices();
    if (!deviceInitList.empty())
    {
        GetList().emplace_back(new Platform(dispatch, initData, std::move(deviceInitList)));
    }
}

Platform::Platform(const cl_icd_dispatch &dispatch,
                   rx::CLPlatformImpl::InitData &initData,
                   rx::CLDeviceImpl::InitList &&deviceInitList)
    : _cl_platform_id(dispatch),
      mImpl(std::move(initData.first)),
      mInfo(std::move(initData.second)),
      mDevices(Device::CreateDevices(*this, std::move(deviceInitList)))
{}

constexpr char Platform::kVendor[];
constexpr char Platform::kIcdSuffix[];

}  // namespace cl
