//
// Copyright (c) 2013-2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SystemInfo_internal.h: Functions used by the SystemInfo_* files and unittests

#ifndef GPU_INFO_UTIL_SYSTEM_INFO_INTERNAL_H_
#define GPU_INFO_UTIL_SYSTEM_INFO_INTERNAL_H_

#include "gpu_info_util/SystemInfo.h"

namespace angle
{

// Defined in SystemInfo_libpci when GPU_INFO_USE_LIBPCI is defined.
bool GetPCIDevicesWithLibPCI(std::vector<GPUDeviceInfo> *devices);
// Defined in SystemInfo_x11 when GPU_INFO_USE_X11 is defined.
bool GetNvidiaDriverVersionWithXNVCtrl(std::string *version);

// Target specific helper functions that can be compiled on all targets
// Live in SystemInfo.cpp
bool ParseAMDBrahmaDriverVersion(const std::string &content, std::string *version);
bool ParseAMDCatalystDriverVersion(const std::string &content, std::string *version);

}  // namespace angle

#endif  // GPU_INFO_UTIL_SYSTEM_INFO_INTERNAL_H_
