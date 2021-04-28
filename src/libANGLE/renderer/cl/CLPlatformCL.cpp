//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformCL.cpp:
//    Implements the class methods for CLPlatformCL.
//

#include "libANGLE/renderer/cl/CLPlatformCL.h"

#include "libANGLE/CLPlatform.h"
#include "libANGLE/Debug.h"

#include "anglebase/no_destructor.h"
#include "common/angle_version.h"

extern "C" {
#include "icd.h"
}  // extern "C"

#include <cstdlib>
#include <unordered_set>
#include <vector>

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

CLPlatformCL::ImplList CLPlatformCL::GetPlatforms(bool isIcd)
{
    // Using khrIcdInitialize() of the third party Khronos OpenCL ICD Loader to enumerate the
    // available OpenCL implementations on the system. They will be stored in the singly linked
    // list khrIcdVendors of the C struct KHRicdVendor.
    if (khrIcdVendors == nullptr)
    {
        // Our OpenCL entry points are not reentrant, so we have to prevent khrIcdInitialize()
        // from querying ANGLE's OpenCL ICD library. We store a dummy entry with the library in
        // the khrIcdVendors list, because the ICD Loader skips the libries which are already in
        // the list as it assumes they were already enumerated.
        static angle::base::NoDestructor<KHRicdVendor> sVendorAngle({});
        sVendorAngle->library = khrIcdOsLibraryLoad(ANGLE_OPENCL_ICD_LIB_NAME);
        khrIcdVendors         = sVendorAngle.get();
        if (khrIcdVendors->library != nullptr)
        {
            khrIcdInitialize();
            // After the enumeration we don't need ANGLE's OpenCL ICD library any more,
            // but we keep the dummy entry int the list to prevent another enumeration.
            khrIcdOsLibraryUnload(khrIcdVendors->library);
            khrIcdVendors->library = nullptr;
        }
    }

    // Iterating through the singly linked list khrIcdVendors to create an ANGLE CL pass-through
    // platform for each found ICD platform. Skipping our dummy entry that has an invalid platform.
    ImplList implList;
    for (KHRicdVendor *vendorIt = khrIcdVendors; vendorIt != nullptr; vendorIt = vendorIt->next)
    {
        if (vendorIt->platform != nullptr)
        {
            rx::CLPlatformImpl::Ptr impl = Create(vendorIt->platform);
            if (impl)
            {
                implList.emplace_back(std::move(impl));
            }
        }
    }
    return implList;
}

CLPlatformCL::CLPlatformCL(cl_platform_id platform, Info &&info)
    : CLPlatformImpl(std::move(info)), mPlatform(platform)
{}

#define ANGLE_GET_INFO(info, size, param, size_ret) \
    result = platform->getDispatch().clGetPlatformInfo(platform, info, size, param, size_ret)

#define ANGLE_TRY_GET_INFO(info, size, param, size_ret)  \
    do                                                   \
    {                                                    \
        ANGLE_GET_INFO(info, size, param, size_ret);     \
        if (result != CL_SUCCESS)                        \
        {                                                \
            ERR() << "Failed to query CL platform info"; \
            return std::unique_ptr<CLPlatformCL>();      \
        }                                                \
    } while (0)

std::unique_ptr<CLPlatformCL> CLPlatformCL::Create(cl_platform_id platform)
{
    cl_int result    = 0;
    size_t paramSize = 0u;
    std::vector<std::string::value_type> param;
    CLPlatformImpl::Info info;

    // Skip ANGLE CL implementation to prevent passthrough loop
    ANGLE_TRY_GET_INFO(CL_PLATFORM_VENDOR, 0u, nullptr, &paramSize);
    param.resize(paramSize, '\0');
    ANGLE_TRY_GET_INFO(CL_PLATFORM_VENDOR, paramSize, param.data(), nullptr);
    if (std::string(param.data()).compare(cl::Platform::GetVendor()) == 0)
    {
        ERR() << "Tried to create CL pass-through back end for ANGLE library";
        return std::unique_ptr<CLPlatformCL>();
    }

    // Skip platform if it is not ICD compatible
    ANGLE_TRY_GET_INFO(CL_PLATFORM_EXTENSIONS, 0u, nullptr, &paramSize);
    param.resize(paramSize, '\0');
    ANGLE_TRY_GET_INFO(CL_PLATFORM_EXTENSIONS, paramSize, param.data(), nullptr);
    info.mExtensions.assign(param.data());
    if (info.mExtensions.find("cl_khr_icd") == std::string::npos)
    {
        WARN() << "CL platform is not ICD compatible";
        return std::unique_ptr<CLPlatformCL>();
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
    ANGLE_TRY_GET_INFO(CL_PLATFORM_PROFILE, 0u, nullptr, &paramSize);
    param.resize(paramSize, '\0');
    ANGLE_TRY_GET_INFO(CL_PLATFORM_PROFILE, paramSize, param.data(), nullptr);
    info.mProfile.assign(param.data());

    ANGLE_TRY_GET_INFO(CL_PLATFORM_VERSION, 0u, nullptr, &paramSize);
    param.resize(paramSize, '\0');
    ANGLE_TRY_GET_INFO(CL_PLATFORM_VERSION, paramSize, param.data(), nullptr);
    info.mVersionStr.assign(param.data());
    info.mVersionStr += " (ANGLE " ANGLE_VERSION_STRING ")";

    const std::string::size_type spacePos = info.mVersionStr.find(' ');
    const std::string::size_type dotPos   = info.mVersionStr.find('.');
    if (spacePos == std::string::npos || dotPos == std::string::npos)
    {
        ERR() << "Failed to extract version from OpenCL version string: " << info.mVersionStr;
        return std::unique_ptr<CLPlatformCL>();
    }
    const cl_uint major =
        static_cast<cl_uint>(std::strtol(&info.mVersionStr[spacePos + 1u], nullptr, 10));
    const cl_uint minor =
        static_cast<cl_uint>(std::strtol(&info.mVersionStr[dotPos + 1u], nullptr, 10));
    if (major == 0)
    {
        ERR() << "Failed to extract version from OpenCL version string: " << info.mVersionStr;
        return std::unique_ptr<CLPlatformCL>();
    }

    ANGLE_GET_INFO(CL_PLATFORM_NUMERIC_VERSION, sizeof(info.mVersion), &info.mVersion, nullptr);
    if (result != CL_SUCCESS)
    {
        info.mVersion = CL_MAKE_VERSION(major, minor, 0);
    }
    else if (CL_VERSION_MAJOR(info.mVersion) != major || CL_VERSION_MINOR(info.mVersion) != minor)
    {
        WARN() << "CL_PLATFORM_NUMERIC_VERSION = " << CL_VERSION_MAJOR(info.mVersion) << '.'
               << CL_VERSION_MINOR(info.mVersion)
               << " does not match version string: " << info.mVersionStr;
    }

    ANGLE_TRY_GET_INFO(CL_PLATFORM_NAME, 0u, nullptr, &paramSize);
    param.resize(paramSize, '\0');
    ANGLE_TRY_GET_INFO(CL_PLATFORM_NAME, paramSize, param.data(), nullptr);
    info.mName.assign("ANGLE pass-through -> ");
    info.mName += param.data();

    ANGLE_GET_INFO(CL_PLATFORM_EXTENSIONS_WITH_VERSION, 0u, nullptr, &paramSize);
    if (result == CL_SUCCESS)
    {
        info.mExtensionList.resize(paramSize);
        ANGLE_TRY_GET_INFO(CL_PLATFORM_EXTENSIONS_WITH_VERSION, paramSize,
                           info.mExtensionList.data(), nullptr);

        // Filter out extensions which are not (yet) supported to be passed through
        const ExtensionSet &supported       = GetSupportedExtensions();
        ExtensionList::const_iterator extIt = info.mExtensionList.cbegin();
        while (extIt != info.mExtensionList.cend())
        {
            if (supported.find(extIt->name) != supported.cend())
            {
                ++extIt;
            }
            else
            {
                extIt = info.mExtensionList.erase(extIt);
            }
        }
    }

    ANGLE_GET_INFO(CL_PLATFORM_HOST_TIMER_RESOLUTION, sizeof(info.mHostTimerRes),
                   &info.mHostTimerRes, nullptr);

    return std::unique_ptr<CLPlatformCL>(new CLPlatformCL(platform, std::move(info)));
}

}  // namespace rx
