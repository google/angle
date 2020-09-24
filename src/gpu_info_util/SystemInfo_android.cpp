//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SystemInfo_android.cpp: implementation of the Android-specific parts of SystemInfo.h

#include "gpu_info_util/SystemInfo_internal.h"

#include <sys/system_properties.h>
#include <cstring>
#include <fstream>

#include "common/angleutils.h"
#include "common/debug.h"

#if defined(ANGLE_ENABLE_VULKAN)
#    include "gpu_info_util/SystemInfo_vulkan.h"
#endif  // defined(ANGLE_ENABLE_VULKAN)

namespace angle
{
namespace
{
bool GetAndroidSystemProperty(const std::string &propertyName, std::string *value)
{
    // PROP_VALUE_MAX from <sys/system_properties.h>
    std::vector<char> propertyBuf(PROP_VALUE_MAX);
    int len = __system_property_get(propertyName.c_str(), propertyBuf.data());
    if (len <= 0)
    {
        return false;
    }
    *value = std::string(propertyBuf.data());
    return true;
}
}  // namespace

bool GetSystemInfo(SystemInfo *info)
{
    bool isFullyPopulated = true;

    isFullyPopulated =
        GetAndroidSystemProperty("ro.product.manufacturer", &info->machineManufacturer) &&
        isFullyPopulated;
    isFullyPopulated =
        GetAndroidSystemProperty("ro.product.model", &info->machineModelName) && isFullyPopulated;

#if defined(ANGLE_ENABLE_VULKAN)
    isFullyPopulated = GetSystemInfoVulkan(info) && isFullyPopulated;
#else
    isFullyPopulated = false;
#endif  // defined(ANGLE_ENABLE_VULKAN)

    return isFullyPopulated;
}

}  // namespace angle
