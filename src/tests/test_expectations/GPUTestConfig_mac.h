//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// GPUTestConfig_mac.h:
//   Helper functions for GPUTestConfig that have to be compiled in ObjectiveC++
//

#ifndef TEST_EXPECTATIONS_GPU_TEST_CONFIG_MAC_H_
#define TEST_EXPECTATIONS_GPU_TEST_CONFIG_MAC_H_

#include "GPUInfo.h"

namespace angle
{

void GetOperatingSystemVersionNumbers(int32_t *major_version,
                                      int32_t *minor_version,
                                      int32_t *bugfix_version);

}  // namespace angle

#endif  // TEST_EXPECTATIONS_GPU_TEST_CONFIG_MAC_H_
