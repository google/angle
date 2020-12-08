//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// GPUTestConfig_iOS.mm:
//   Helper functions for GPUTestConfig that have to be compiled in ObjectiveC++

#include "GPUTestConfig_ios.h"

#include "common/apple_platform_utils.h"

namespace angle
{

void GetOperatingSystemVersionNumbers(int32_t *majorVersion, int32_t *minorVersion)
{
    // TODO(jdarpinian): Implement this. http://anglebug.com/5485
    *majorVersion = 0;
    *minorVersion = 0;
}

}  // namespace angle
