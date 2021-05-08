//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceImpl.cpp: Implements the class methods for CLDeviceImpl.

#include "libANGLE/renderer/CLDeviceImpl.h"

namespace rx
{

CLDeviceImpl::Info::Info() = default;

CLDeviceImpl::Info::~Info() = default;

CLDeviceImpl::Info::Info(Info &&) = default;

CLDeviceImpl::Info &CLDeviceImpl::Info::operator=(Info &&) = default;

bool CLDeviceImpl::Info::isValid() const
{
    // From the OpenCL specification for info name CL_DEVICE_MAX_WORK_ITEM_SIZES:
    // "The minimum value is (1, 1, 1) for devices that are not of type CL_DEVICE_TYPE_CUSTOM."
    // https://www.khronos.org/registry/OpenCL/specs/3.0-unified/html/OpenCL_API.html#clGetDeviceInfo
    // Custom devices are currently not supported by ANGLE back ends.
    return mMaxWorkItemSizes.size() >= 3u && mMaxWorkItemSizes[0] >= 1u &&
           mMaxWorkItemSizes[1] >= 1u && mMaxWorkItemSizes[2] >= 1u;
}

}  // namespace rx
