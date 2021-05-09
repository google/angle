//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.cpp: Implements the cl::Platform class.

#include "libANGLE/CLPlatform.h"

#include <cstdint>
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

Platform::~Platform()
{
    removeRef();
}

Device::RefList Platform::mapDevices(const rx::CLDeviceImpl::List &deviceImplList) const
{
    Device::RefList devices;
    for (rx::CLDeviceImpl *impl : deviceImplList)
    {
        auto it = mDevices.cbegin();
        while (it != mDevices.cend() && (*it)->mImpl.get() != impl)
        {
            ++it;
        }
        if (it != mDevices.cend())
        {
            devices.emplace_back(it->get());
        }
        else
        {
            ERR() << "Device not found in platform list";
        }
    }
    if (devices.size() != deviceImplList.size())
    {
        devices.clear();
    }
    return devices;
}

cl_int Platform::getInfo(PlatformInfo name, size_t valueSize, void *value, size_t *valueSizeRet)
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
            if (mInfo.mVersion < CL_MAKE_VERSION(3, 0, 0))
            {
                return CL_INVALID_VALUE;
            }
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
            if (mInfo.mVersion < CL_MAKE_VERSION(3, 0, 0))
            {
                return CL_INVALID_VALUE;
            }
            copyValue = mInfo.mExtensionsWithVersion.data();
            copySize  = mInfo.mExtensionsWithVersion.size() *
                       sizeof(decltype(mInfo.mExtensionsWithVersion)::value_type);
            break;
        case PlatformInfo::HostTimerResolution:
            if (mInfo.mVersion < CL_MAKE_VERSION(2, 1, 0))
            {
                return CL_INVALID_VALUE;
            }
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
    if (valueSizeRet != nullptr)
    {
        *valueSizeRet = copySize;
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

Context *Platform::createContext(Context::PropArray &&properties,
                                 cl_uint numDevices,
                                 Device *const *devices,
                                 ContextErrorCB notify,
                                 void *userData,
                                 bool userSync,
                                 cl_int *errcodeRet)
{
    Device::RefList refDevices;
    while (numDevices-- != 0u)
    {
        refDevices.emplace_back(*devices++);
    }
    mContexts.emplace_back(new Context(*this, std::move(properties), std::move(refDevices), notify,
                                       userData, userSync, errcodeRet));
    if (!mContexts.back()->mImpl)
    {
        mContexts.back()->release();
        return nullptr;
    }
    return mContexts.back().get();
}

Context *Platform::createContextFromType(Context::PropArray &&properties,
                                         cl_device_type deviceType,
                                         ContextErrorCB notify,
                                         void *userData,
                                         bool userSync,
                                         cl_int *errcodeRet)
{
    mContexts.emplace_back(new Context(*this, std::move(properties), deviceType, notify, userData,
                                       userSync, errcodeRet));
    if (!mContexts.back()->mImpl || mContexts.back()->mDevices.empty())
    {
        mContexts.back()->release();
        return nullptr;
    }
    return mContexts.back().get();
}

void Platform::CreatePlatform(const cl_icd_dispatch &dispatch,
                              rx::CLPlatformImpl::InitData &initData)
{
    Ptr platform(new Platform(dispatch, initData));
    if (!platform->mDevices.empty())
    {
        GetList().emplace_back(std::move(platform));
    }
}

Platform::Platform(const cl_icd_dispatch &dispatch, rx::CLPlatformImpl::InitData &initData)
    : _cl_platform_id(dispatch),
      mImpl(std::move(std::get<0>(initData))),
      mInfo(std::move(std::get<1>(initData))),
      mDevices(Device::CreateDevices(*this, std::move(std::get<2>(initData))))
{
    ASSERT(isCompatible(this));
}

rx::CLContextImpl::Ptr Platform::createContext(const Device::RefList &devices,
                                               ContextErrorCB notify,
                                               void *userData,
                                               bool userSync,
                                               cl_int *errcodeRet)
{
    rx::CLDeviceImpl::List deviceImplList;
    for (const Device::RefPtr &device : devices)
    {
        deviceImplList.emplace_back(device->mImpl.get());
    }
    return mImpl->createContext(std::move(deviceImplList), notify, userData, userSync, errcodeRet);
}

void Platform::destroyContext(Context *context)
{
    auto contextIt = mContexts.cbegin();
    while (contextIt != mContexts.cend() && contextIt->get() != context)
    {
        ++contextIt;
    }
    if (contextIt != mContexts.cend())
    {
        mContexts.erase(contextIt);
    }
    else
    {
        ERR() << "Context not found";
    }
}

constexpr char Platform::kVendor[];
constexpr char Platform::kIcdSuffix[];

}  // namespace cl
