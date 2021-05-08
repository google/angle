//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformCL.cpp: Implements the class methods for CLPlatformCL.

#include "libANGLE/renderer/cl/CLPlatformCL.h"

#include "libANGLE/renderer/cl/CLDeviceCL.h"

#include "libANGLE/CLPlatform.h"
#include "libANGLE/Debug.h"

#include "anglebase/no_destructor.h"
#include "common/angle_version.h"

extern "C" {
#include "icd.h"
}  // extern "C"

#include <cstdlib>
#include <unordered_set>

namespace rx
{

namespace
{
using ExtensionSet = std::unordered_set<std::string>;

const ExtensionSet &GetSupportedExtensions()
{
    static angle::base::NoDestructor<ExtensionSet> sExtensions(
        {"cl_khr_extended_versioning", "cl_khr_icd"});
    return *sExtensions;
}
}  // namespace

CLPlatformCL::~CLPlatformCL() = default;

CLDeviceImpl::InitList CLPlatformCL::getDevices()
{
    CLDeviceImpl::InitList initList;

    // Fetch all regular devices. This does not include CL_DEVICE_TYPE_CUSTOM, which are not
    // supported by the CL pass-through back end because they have no standard feature set.
    // This makes them unreliable for the purpose of this back end.
    cl_uint numDevices = 0u;
    if (mPlatform->getDispatch().clGetDeviceIDs(mPlatform, CL_DEVICE_TYPE_ALL, 0u, nullptr,
                                                &numDevices) == CL_SUCCESS)
    {
        std::vector<cl_device_id> devices(numDevices, nullptr);
        if (mPlatform->getDispatch().clGetDeviceIDs(mPlatform, CL_DEVICE_TYPE_ALL, numDevices,
                                                    devices.data(), nullptr) == CL_SUCCESS)
        {
            for (cl_device_id device : devices)
            {
                CLDeviceImpl::Info info = CLDeviceCL::GetInfo(device);
                if (info.isValid())
                {
                    initList.emplace_back(new CLDeviceCL(device), std::move(info));
                }
            }
        }
        else
        {
            ERR() << "Failed to query CL devices";
        }
    }
    else
    {
        ERR() << "Failed to query CL devices";
    }

    return initList;
}

CLPlatformCL::InitList CLPlatformCL::GetPlatforms(bool isIcd)
{
    // Using khrIcdInitialize() of the third party Khronos OpenCL ICD Loader to enumerate the
    // available OpenCL implementations on the system. They will be stored in the singly linked
    // list khrIcdVendors of the C struct KHRicdVendor.
    if (khrIcdVendors == nullptr)
    {
        // Our OpenCL entry points are not reentrant, so we have to prevent khrIcdInitialize()
        // from querying ANGLE's OpenCL library. We store a dummy entry with the library in the
        // khrIcdVendors list, because the ICD Loader skips the libraries which are already in
        // the list as it assumes they were already enumerated.
        static angle::base::NoDestructor<KHRicdVendor> sVendorAngle({});
        sVendorAngle->library = khrIcdOsLibraryLoad(ANGLE_OPENCL_LIB_NAME);
        khrIcdVendors         = sVendorAngle.get();
        if (khrIcdVendors->library != nullptr)
        {
            khrIcdInitialize();
            // After the enumeration we don't need ANGLE's OpenCL library any more,
            // but we keep the dummy entry int the list to prevent another enumeration.
            khrIcdOsLibraryUnload(khrIcdVendors->library);
            khrIcdVendors->library = nullptr;
        }
    }

    // Iterating through the singly linked list khrIcdVendors to create an ANGLE CL pass-through
    // platform for each found ICD platform. Skipping our dummy entry that has an invalid platform.
    InitList initList;
    for (KHRicdVendor *vendorIt = khrIcdVendors; vendorIt != nullptr; vendorIt = vendorIt->next)
    {
        if (vendorIt->platform != nullptr)
        {
            Info info = GetInfo(vendorIt->platform);
            if (info.isValid())
            {
                initList.emplace_back(new CLPlatformCL(vendorIt->platform), std::move(info));
            }
        }
    }
    return initList;
}

CLPlatformCL::CLPlatformCL(cl_platform_id platform) : mPlatform(platform) {}

#define ANGLE_GET_INFO_SIZE(name, size_ret) \
    platform->getDispatch().clGetPlatformInfo(platform, name, 0u, nullptr, size_ret)

#define ANGLE_GET_INFO_SIZE_RET(name, size_ret)                       \
    do                                                                \
    {                                                                 \
        if (ANGLE_GET_INFO_SIZE(name, size_ret) != CL_SUCCESS)        \
        {                                                             \
            ERR() << "Failed to query CL platform info for " << name; \
            return info;                                              \
        }                                                             \
    } while (0)

#define ANGLE_GET_INFO(name, size, param) \
    platform->getDispatch().clGetPlatformInfo(platform, name, size, param, nullptr)

#define ANGLE_GET_INFO_RET(name, size, param)                         \
    do                                                                \
    {                                                                 \
        if (ANGLE_GET_INFO(name, size, param) != CL_SUCCESS)          \
        {                                                             \
            ERR() << "Failed to query CL platform info for " << name; \
            return info;                                              \
        }                                                             \
    } while (0)

CLPlatformImpl::Info CLPlatformCL::GetInfo(cl_platform_id platform)
{
    Info info;
    size_t valueSize = 0u;
    std::vector<char> valString;

    // Verify that the platform is valid
    ASSERT(platform != nullptr);
    ASSERT(platform->getDispatch().clGetPlatformInfo != nullptr);
    ASSERT(platform->getDispatch().clGetDeviceIDs != nullptr);
    ASSERT(platform->getDispatch().clGetDeviceInfo != nullptr);

    // Skip ANGLE CL implementation to prevent passthrough loop
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_VENDOR, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_VENDOR, valueSize, valString.data());
    if (std::string(valString.data()).compare(cl::Platform::GetVendor()) == 0)
    {
        ERR() << "Tried to create CL pass-through back end for ANGLE library";
        return info;
    }

    // Skip platform if it is not ICD compatible
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_EXTENSIONS, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_EXTENSIONS, valueSize, valString.data());
    info.mExtensions.assign(valString.data());
    if (info.mExtensions.find("cl_khr_icd") == std::string::npos)
    {
        WARN() << "CL platform is not ICD compatible";
        return info;
    }

    // Filter out extensions which are not (yet) supported to be passed through
    if (!info.mExtensions.empty())
    {
        const ExtensionSet &supported   = GetSupportedExtensions();
        std::string::size_type extStart = 0u;
        do
        {
            const std::string::size_type spacePos = info.mExtensions.find(' ', extStart);
            const bool foundSpace                 = spacePos != std::string::npos;
            const std::string::size_type length =
                (foundSpace ? spacePos : info.mExtensions.length()) - extStart;
            if (supported.find(info.mExtensions.substr(extStart, length)) != supported.cend())
            {
                extStart = foundSpace && spacePos + 1u < info.mExtensions.length()
                               ? spacePos + 1u
                               : std::string::npos;
            }
            else
            {
                info.mExtensions.erase(extStart, length + (foundSpace ? 1u : 0u));
                if (extStart >= info.mExtensions.length())
                {
                    extStart = std::string::npos;
                }
            }
        } while (extStart != std::string::npos);
        while (!info.mExtensions.empty() && info.mExtensions.back() == ' ')
        {
            info.mExtensions.pop_back();
        }
    }

    // Fetch common platform info
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_VERSION, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_VERSION, valueSize, valString.data());
    info.mVersionStr.assign(valString.data());
    info.mVersionStr += " (ANGLE " ANGLE_VERSION_STRING ")";

    const std::string::size_type spacePos = info.mVersionStr.find(' ');
    const std::string::size_type dotPos   = info.mVersionStr.find('.');
    if (spacePos == std::string::npos || dotPos == std::string::npos)
    {
        ERR() << "Failed to extract version from OpenCL version string: " << info.mVersionStr;
        return info;
    }
    const cl_uint major =
        static_cast<cl_uint>(std::strtol(&info.mVersionStr[spacePos + 1u], nullptr, 10));
    const cl_uint minor =
        static_cast<cl_uint>(std::strtol(&info.mVersionStr[dotPos + 1u], nullptr, 10));
    if (major == 0)
    {
        ERR() << "Failed to extract version from OpenCL version string: " << info.mVersionStr;
        return info;
    }

    if (ANGLE_GET_INFO(CL_PLATFORM_NUMERIC_VERSION, sizeof(info.mVersion), &info.mVersion) !=
        CL_SUCCESS)
    {
        info.mVersion = CL_MAKE_VERSION(major, minor, 0);
    }
    else if (CL_VERSION_MAJOR(info.mVersion) != major || CL_VERSION_MINOR(info.mVersion) != minor)
    {
        WARN() << "CL_PLATFORM_NUMERIC_VERSION = " << CL_VERSION_MAJOR(info.mVersion) << '.'
               << CL_VERSION_MINOR(info.mVersion)
               << " does not match version string: " << info.mVersionStr;
    }

    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_NAME, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_NAME, valueSize, valString.data());
    info.mName.assign("ANGLE pass-through -> ");
    info.mName += valString.data();

    if (ANGLE_GET_INFO_SIZE(CL_PLATFORM_EXTENSIONS_WITH_VERSION, &valueSize) == CL_SUCCESS &&
        (valueSize % sizeof(decltype(info.mExtensionsWithVersion)::value_type)) == 0u)
    {
        info.mExtensionsWithVersion.resize(
            valueSize / sizeof(decltype(info.mExtensionsWithVersion)::value_type));
        ANGLE_GET_INFO_RET(CL_PLATFORM_EXTENSIONS_WITH_VERSION, valueSize,
                           info.mExtensionsWithVersion.data());

        // Filter out extensions which are not (yet) supported to be passed through
        const ExtensionSet &supported = GetSupportedExtensions();
        auto extIt                    = info.mExtensionsWithVersion.cbegin();
        while (extIt != info.mExtensionsWithVersion.cend())
        {
            if (supported.find(extIt->name) != supported.cend())
            {
                ++extIt;
            }
            else
            {
                extIt = info.mExtensionsWithVersion.erase(extIt);
            }
        }
    }

    ANGLE_GET_INFO(CL_PLATFORM_HOST_TIMER_RESOLUTION, sizeof(info.mHostTimerRes),
                   &info.mHostTimerRes);

    // Get this last, so the info is invalid if anything before fails
    ANGLE_GET_INFO_SIZE_RET(CL_PLATFORM_PROFILE, &valueSize);
    valString.resize(valueSize, '\0');
    ANGLE_GET_INFO_RET(CL_PLATFORM_PROFILE, valueSize, valString.data());
    info.mProfile.assign(valString.data());

    return info;
}

}  // namespace rx
