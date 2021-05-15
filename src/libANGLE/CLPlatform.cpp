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

Context::PropArray ParseContextProperties(const cl_context_properties *properties,
                                          Platform *&platform,
                                          bool &userSync)
{
    Context::PropArray propArray;
    if (properties != nullptr)
    {
        // Count the trailing zero
        size_t propSize                     = 1u;
        const cl_context_properties *propIt = properties;
        while (*propIt != 0)
        {
            ++propSize;
            switch (*propIt++)
            {
                case CL_CONTEXT_PLATFORM:
                    platform = reinterpret_cast<Platform *>(*propIt++);
                    ++propSize;
                    break;
                case CL_CONTEXT_INTEROP_USER_SYNC:
                    userSync = *propIt++ != CL_FALSE;
                    ++propSize;
                    break;
            }
        }
        propArray.reserve(propSize);
        propArray.insert(propArray.cend(), properties, properties + propSize);
    }
    if (platform == nullptr)
    {
        platform = Platform::GetDefault();
    }
    return propArray;
}

}  // namespace

Platform::~Platform()
{
    removeRef();
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
                              cl_device_id *devices,
                              cl_uint *numDevices) const
{
    cl_uint found = 0u;
    for (const DevicePtr &device : mDevices)
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

void Platform::CreatePlatform(const cl_icd_dispatch &dispatch, const CreateImplFunc &createImplFunc)
{
    PlatformPtr platform(new Platform(dispatch, createImplFunc));
    if (platform->mInfo.isValid() && !platform->mDevices.empty())
    {
        GetList().emplace_back(std::move(platform));
    }
}

cl_int Platform::GetPlatformIDs(cl_uint num_entries,
                                cl_platform_id *platforms,
                                cl_uint *num_platforms)
{
    const PtrList &platformList = GetList();
    if (num_platforms != nullptr)
    {
        *num_platforms = static_cast<cl_uint>(platformList.size());
    }
    if (platforms != nullptr)
    {
        cl_uint entry   = 0u;
        auto platformIt = platformList.cbegin();
        while (entry < num_entries && platformIt != platformList.cend())
        {
            platforms[entry++] = (*platformIt++).get();
        }
    }
    return CL_SUCCESS;
}

cl_context Platform::CreateContext(const cl_context_properties *properties,
                                   cl_uint numDevices,
                                   const cl_device_id *devices,
                                   ContextErrorCB notify,
                                   void *userData,
                                   cl_int *errcodeRet)
{
    Platform *platform           = nullptr;
    bool userSync                = false;
    Context::PropArray propArray = ParseContextProperties(properties, platform, userSync);
    ASSERT(platform != nullptr);
    DeviceRefList refDevices;
    while (numDevices-- != 0u)
    {
        refDevices.emplace_back(static_cast<Device *>(*devices++));
    }

    platform->mContexts.emplace_back(new Context(*platform, std::move(propArray),
                                                 std::move(refDevices), notify, userData, userSync,
                                                 errcodeRet));
    if (!platform->mContexts.back()->mImpl)
    {
        platform->mContexts.back()->release();
        return nullptr;
    }
    return platform->mContexts.back().get();
}

cl_context Platform::CreateContextFromType(const cl_context_properties *properties,
                                           cl_device_type deviceType,
                                           ContextErrorCB notify,
                                           void *userData,
                                           cl_int *errcodeRet)
{
    Platform *platform           = nullptr;
    bool userSync                = false;
    Context::PropArray propArray = ParseContextProperties(properties, platform, userSync);
    ASSERT(platform != nullptr);

    platform->mContexts.emplace_back(new Context(*platform, std::move(propArray), deviceType,
                                                 notify, userData, userSync, errcodeRet));
    if (!platform->mContexts.back()->mImpl || platform->mContexts.back()->mDevices.empty())
    {
        platform->mContexts.back()->release();
        return nullptr;
    }
    return platform->mContexts.back().get();
}

Platform::Platform(const cl_icd_dispatch &dispatch, const CreateImplFunc &createImplFunc)
    : _cl_platform_id(dispatch),
      mImpl(createImplFunc(*this)),
      mInfo(mImpl->createInfo()),
      mDevices(mImpl->createDevices(*this))
{}

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
