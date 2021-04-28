//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformVk.cpp:
//    Implements the class methods for CLPlatformVk.
//

#include "libANGLE/renderer/vulkan/CLPlatformVk.h"

#include "anglebase/no_destructor.h"
#include "common/angle_version.h"

#include <algorithm>

namespace rx
{

namespace
{
std::string CreateExtensionString(const CLPlatformImpl::ExtensionList &extList)
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
}  // anonymous namespace

CLPlatformVk::CLPlatformVk(Info &&info) : CLPlatformImpl(std::move(info)) {}

CLPlatformVk::~CLPlatformVk() = default;

CLPlatformVk::ImplList CLPlatformVk::GetPlatforms()
{
    ExtensionList extList = {
        cl_name_version{CL_MAKE_VERSION(1, 0, 0), "cl_khr_icd"},
        cl_name_version{CL_MAKE_VERSION(1, 0, 0), "cl_khr_extended_versioning"}};
    std::string extensions = CreateExtensionString(extList);

    Info info("FULL_PROFILE", std::string(GetVersionString()), GetVersion(), "ANGLE Vulkan",
              std::move(extensions), std::move(extList), 0u);

    ImplList implList;
    implList.emplace_back(new CLPlatformVk(std::move(info)));
    return implList;
}

const std::string &CLPlatformVk::GetVersionString()
{
    static const angle::base::NoDestructor<const std::string> sVersion(
        "OpenCL " + std::to_string(CL_VERSION_MAJOR(GetVersion())) + "." +
        std::to_string(CL_VERSION_MINOR(GetVersion())) + " ANGLE " ANGLE_VERSION_STRING);
    return *sVersion;
}

}  // namespace rx
