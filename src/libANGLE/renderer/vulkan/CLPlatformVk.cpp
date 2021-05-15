//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformVk.cpp: Implements the class methods for CLPlatformVk.

#include "libANGLE/renderer/vulkan/CLPlatformVk.h"

#include "libANGLE/renderer/vulkan/CLDeviceVk.h"

#include "libANGLE/CLPlatform.h"

#include "anglebase/no_destructor.h"
#include "common/angle_version.h"

namespace rx
{

namespace
{

std::string CreateExtensionString(const NameVersionVector &extList)
{
    std::string extensions;
    for (const cl_name_version &ext : extList)
    {
        extensions += ext.name;
        extensions += ' ';
    }
    if (!extensions.empty())
    {
        extensions.pop_back();
    }
    return extensions;
}

}  // namespace

CLPlatformVk::~CLPlatformVk() = default;

CLPlatformImpl::Info CLPlatformVk::createInfo() const
{
    NameVersionVector extList = {
        cl_name_version{CL_MAKE_VERSION(1, 0, 0), "cl_khr_icd"},
        cl_name_version{CL_MAKE_VERSION(1, 0, 0), "cl_khr_extended_versioning"}};

    Info info;
    info.mProfile.assign("FULL_PROFILE");
    info.mVersionStr.assign(GetVersionString());
    info.mVersion = GetVersion();
    info.mName.assign("ANGLE Vulkan");
    info.mExtensions.assign(CreateExtensionString(extList));
    info.mExtensionsWithVersion = std::move(extList);
    info.mHostTimerRes          = 0u;
    return info;
}

cl::DevicePtrList CLPlatformVk::createDevices(cl::Platform &platform) const
{
    cl::DevicePtrList devices;
    const cl::Device::CreateImplFunc createImplFunc = [](const cl::Device &device) {
        return CLDeviceVk::Ptr(new CLDeviceVk(device));
    };
    devices.emplace_back(cl::Device::CreateDevice(platform, nullptr, createImplFunc));
    if (!devices.back())
    {
        devices.clear();
    }
    return devices;
}

CLContextImpl::Ptr CLPlatformVk::createContext(const cl::Context &context,
                                               const cl::DeviceRefList &devices,
                                               cl::ContextErrorCB notify,
                                               void *userData,
                                               bool userSync,
                                               cl_int *errcodeRet)
{
    CLContextImpl::Ptr contextImpl;
    return contextImpl;
}

CLContextImpl::Ptr CLPlatformVk::createContextFromType(const cl::Context &context,
                                                       cl_device_type deviceType,
                                                       cl::ContextErrorCB notify,
                                                       void *userData,
                                                       bool userSync,
                                                       cl_int *errcodeRet)
{
    CLContextImpl::Ptr contextImpl;
    return contextImpl;
}

void CLPlatformVk::Initialize(const cl_icd_dispatch &dispatch)
{
    const cl::Platform::CreateImplFunc createImplFunc = [](const cl::Platform &platform) {
        return Ptr(new CLPlatformVk(platform));
    };
    cl::Platform::CreatePlatform(dispatch, createImplFunc);
}

const std::string &CLPlatformVk::GetVersionString()
{
    static const angle::base::NoDestructor<const std::string> sVersion(
        "OpenCL " + std::to_string(CL_VERSION_MAJOR(GetVersion())) + "." +
        std::to_string(CL_VERSION_MINOR(GetVersion())) + " ANGLE " ANGLE_VERSION_STRING);
    return *sVersion;
}

CLPlatformVk::CLPlatformVk(const cl::Platform &platform) : CLPlatformImpl(platform) {}

}  // namespace rx
