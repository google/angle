//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformVk.cpp: Implements the class methods for CLPlatformVk.

#include "libANGLE/renderer/vulkan/CLPlatformVk.h"

#include "libANGLE/renderer/vulkan/CLDeviceVk.h"

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

CLDeviceImpl::List CreateDevices(CLPlatformVk &platform, CLDeviceImpl::PtrList &implList)
{
    implList.emplace_back(new CLDeviceVk(platform, nullptr));
    return CLDeviceImpl::List(1u, implList.back().get());
}

}  // namespace

CLPlatformVk::~CLPlatformVk() = default;

CLContextImpl::Ptr CLPlatformVk::createContext(CLDeviceImpl::List &&deviceImplList,
                                               cl::ContextErrorCB notify,
                                               void *userData,
                                               bool userSync,
                                               cl_int *errcodeRet)
{
    CLContextImpl::Ptr contextImpl;
    return contextImpl;
}

CLContextImpl::Ptr CLPlatformVk::createContextFromType(cl_device_type deviceType,
                                                       cl::ContextErrorCB notify,
                                                       void *userData,
                                                       bool userSync,
                                                       cl_int *errcodeRet)
{
    CLContextImpl::Ptr contextImpl;
    return contextImpl;
}

CLPlatformVk::InitList CLPlatformVk::GetPlatforms()
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

    InitList initList;
    if (info.isValid())
    {
        CLDeviceImpl::PtrList devices;
        Ptr platform(new CLPlatformVk(devices));
        if (!devices.empty())
        {
            initList.emplace_back(std::move(platform), std::move(info), std::move(devices));
        }
    }
    return initList;
}

const std::string &CLPlatformVk::GetVersionString()
{
    static const angle::base::NoDestructor<const std::string> sVersion(
        "OpenCL " + std::to_string(CL_VERSION_MAJOR(GetVersion())) + "." +
        std::to_string(CL_VERSION_MINOR(GetVersion())) + " ANGLE " ANGLE_VERSION_STRING);
    return *sVersion;
}

CLPlatformVk::CLPlatformVk(CLDeviceImpl::PtrList &devices)
    : CLPlatformImpl(CreateDevices(*this, devices))
{}

}  // namespace rx
