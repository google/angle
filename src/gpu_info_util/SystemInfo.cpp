//
// Copyright (c) 2013-2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SystemInfo.cpp: implementation of the system-agnostic parts of SystemInfo.h

#include "gpu_info_util/SystemInfo.h"

#include <cstring>
#include <sstream>

namespace angle
{

bool IsAMD(VendorID vendorId)
{
    return vendorId == kVendorID_AMD;
}

bool IsIntel(VendorID vendorId)
{
    return vendorId == kVendorID_Intel;
}

bool IsNvidia(VendorID vendorId)
{
    return vendorId == kVendorID_Nvidia;
}

bool IsQualcomm(VendorID vendorId)
{
    return vendorId == kVendorID_Qualcomm;
}

bool ParseAMDBrahmaDriverVersion(const std::string &content, std::string *version)
{
    const size_t begin = content.find_first_of("0123456789");
    if (begin == std::string::npos)
    {
        return false;
    }

    const size_t end = content.find_first_not_of("0123456789.", begin);
    if (end == std::string::npos)
    {
        *version = content.substr(begin);
    }
    else
    {
        *version = content.substr(begin, end - begin);
    }
    return true;
}

bool ParseAMDCatalystDriverVersion(const std::string &content, std::string *version)
{
    std::istringstream stream(content);

    std::string line;
    while (std::getline(stream, line))
    {
        static const char kReleaseVersion[] = "ReleaseVersion=";
        if (line.compare(0, std::strlen(kReleaseVersion), kReleaseVersion) != 0)
        {
            continue;
        }

        if (ParseAMDBrahmaDriverVersion(line, version))
        {
            return true;
        }
    }
    return false;
}

}  // namespace angle
