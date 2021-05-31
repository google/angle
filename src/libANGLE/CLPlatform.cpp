//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.cpp: Implements the cl::Platform class.

#include "libANGLE/CLPlatform.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLDevice.h"

#include <cstring>

namespace cl
{

namespace
{

bool IsDeviceTypeMatch(DeviceType select, DeviceType type)
{
    // The type 'DeviceType' is a bitfield, so it matches if any selected bit is set.
    // A custom device is an exception, which only matches if it was explicitely selected, see:
    // https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceIDs
    return type == CL_DEVICE_TYPE_CUSTOM ? select == CL_DEVICE_TYPE_CUSTOM : type.isSet(select);
}

Context::PropArray ParseContextProperties(const cl_context_properties *properties,
                                          Platform *&platform,
                                          bool &userSync)
{
    Context::PropArray propArray;
    if (properties != nullptr)
    {
        const cl_context_properties *propIt = properties;
        while (*propIt != 0)
        {
            switch (*propIt++)
            {
                case CL_CONTEXT_PLATFORM:
                    platform = &reinterpret_cast<cl_platform_id>(*propIt++)->cast<Platform>();
                    break;
                case CL_CONTEXT_INTEROP_USER_SYNC:
                    userSync = *propIt++ != CL_FALSE;
                    break;
            }
        }
        // Include the trailing zero
        ++propIt;
        propArray.reserve(propIt - properties);
        propArray.insert(propArray.cend(), properties, propIt);
    }
    if (platform == nullptr)
    {
        platform = Platform::GetDefault();
    }
    return propArray;
}

}  // namespace

void Platform::Initialize(const cl_icd_dispatch &dispatch,
                          rx::CLPlatformImpl::CreateFuncs &&createFuncs)
{
    PlatformPtrs &platforms = GetPointers();
    ASSERT(_cl_platform_id::sDispatch == nullptr && platforms.empty());
    if (_cl_platform_id::sDispatch != nullptr || !platforms.empty())
    {
        ERR() << "Already initialized";
        return;
    }

    _cl_platform_id::sDispatch   = &dispatch;
    _cl_device_id::sDispatch     = &dispatch;
    _cl_context::sDispatch       = &dispatch;
    _cl_command_queue::sDispatch = &dispatch;
    _cl_mem::sDispatch           = &dispatch;
    _cl_program::sDispatch       = &dispatch;
    _cl_kernel::sDispatch        = &dispatch;
    _cl_event::sDispatch         = &dispatch;
    _cl_sampler::sDispatch       = &dispatch;

    platforms.reserve(createFuncs.size());
    while (!createFuncs.empty())
    {
        platforms.emplace_back(new Platform(createFuncs.front()));
        if (!platforms.back()->mInfo.isValid() || platforms.back()->mDevices.empty())
        {
            platforms.pop_back();
        }
        createFuncs.pop_front();
    }
}

cl_int Platform::GetPlatformIDs(cl_uint numEntries,
                                cl_platform_id *platforms,
                                cl_uint *numPlatforms)
{
    const PlatformPtrs &availPlatforms = GetPlatforms();
    if (numPlatforms != nullptr)
    {
        *numPlatforms = static_cast<cl_uint>(availPlatforms.size());
    }
    if (platforms != nullptr)
    {
        cl_uint entry   = 0u;
        auto platformIt = availPlatforms.cbegin();
        while (entry < numEntries && platformIt != availPlatforms.cend())
        {
            platforms[entry++] = (*platformIt++).get();
        }
    }
    return CL_SUCCESS;
}

cl_int Platform::getInfo(PlatformInfo name,
                         size_t valueSize,
                         void *value,
                         size_t *valueSizeRet) const
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
            ASSERT(false);
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return type
        // as specified in the OpenCL Platform Queries table, and param_value is not a NULL value.
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

cl_int Platform::getDeviceIDs(DeviceType deviceType,
                              cl_uint numEntries,
                              cl_device_id *devices,
                              cl_uint *numDevices) const
{
    cl_uint found = 0u;
    for (const DevicePtr &device : mDevices)
    {
        if (IsDeviceTypeMatch(deviceType, device->getInfo().mType))
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

    // CL_DEVICE_NOT_FOUND if no OpenCL devices that matched device_type were found.
    if (found == 0u)
    {
        return CL_DEVICE_NOT_FOUND;
    }

    return CL_SUCCESS;
}

cl_context Platform::CreateContext(const cl_context_properties *properties,
                                   cl_uint numDevices,
                                   const cl_device_id *devices,
                                   ContextErrorCB notify,
                                   void *userData,
                                   cl_int &errorCode)
{
    Platform *platform           = nullptr;
    bool userSync                = false;
    Context::PropArray propArray = ParseContextProperties(properties, platform, userSync);
    ASSERT(platform != nullptr);
    DevicePtrs devs;
    devs.reserve(numDevices);
    while (numDevices-- != 0u)
    {
        devs.emplace_back(&(*devices++)->cast<Device>());
    }
    return Object::Create<Context>(errorCode, *platform, std::move(propArray), std::move(devs),
                                   notify, userData, userSync);
}

cl_context Platform::CreateContextFromType(const cl_context_properties *properties,
                                           DeviceType deviceType,
                                           ContextErrorCB notify,
                                           void *userData,
                                           cl_int &errorCode)
{
    Platform *platform           = nullptr;
    bool userSync                = false;
    Context::PropArray propArray = ParseContextProperties(properties, platform, userSync);
    ASSERT(platform != nullptr);
    return Object::Create<Context>(errorCode, *platform, std::move(propArray), deviceType, notify,
                                   userData, userSync);
}

Platform::~Platform() = default;

Platform::Platform(const rx::CLPlatformImpl::CreateFunc &createFunc)
    : mImpl(createFunc(*this)),
      mInfo(mImpl->createInfo()),
      mDevices(createDevices(mImpl->createDevices()))
{}

DevicePtrs Platform::createDevices(rx::CLDeviceImpl::CreateDatas &&createDatas)
{
    DevicePtrs devices;
    devices.reserve(createDatas.size());
    while (!createDatas.empty())
    {
        devices.emplace_back(
            new Device(*this, nullptr, createDatas.front().first, createDatas.front().second));
        if (!devices.back()->mInfo.isValid())
        {
            devices.pop_back();
        }
        createDatas.pop_front();
    }
    return devices;
}

constexpr char Platform::kVendor[];
constexpr char Platform::kIcdSuffix[];

}  // namespace cl
