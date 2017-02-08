//
// Copyright (c) 2013-2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SystemInfo.h: gathers information available without starting a GPU driver.

#ifndef GPU_INFO_UTIL_SYSTEM_INFO_H_
#define GPU_INFO_UTIL_SYSTEM_INFO_H_

#include <cstdint>
#include <string>
#include <vector>

namespace angle
{

struct GPUDeviceInfo
{
    uint32_t vendorId;
    uint32_t deviceId;

    std::string driverVendor;
    std::string driverVersion;
    std::string driverDate;
};

struct SystemInfo
{
    std::vector<GPUDeviceInfo> gpus;
    int primaryGPUIndex;

    bool isOptimus;
    bool isAMDSwitchable;

    std::string machineModelName;
};

bool GetSystemInfo(SystemInfo *info);

enum VendorID : uint32_t
{
    VENDOR_ID_UNKNOWN = 0x0,
    VENDOR_ID_AMD     = 0x1002,
    VENDOR_ID_INTEL   = 0x8086,
    VENDOR_ID_NVIDIA  = 0x10DE,
    // This is Qualcomm PCI Vendor ID.
    // Android doesn't have a PCI bus, but all we need is a unique id.
    VENDOR_ID_QUALCOMM = 0x5143,
};

bool IsAMD(uint32_t vendorId);
bool IsIntel(uint32_t vendorId);
bool IsNvidia(uint32_t vendorId);
bool IsQualcomm(uint32_t vendorId);

}  // namespace angle

#endif  // GPU_INFO_UTIL_SYSTEM_INFO_H_
