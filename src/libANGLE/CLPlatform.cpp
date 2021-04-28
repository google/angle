//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatform.cpp: Implements the cl::Platform class.

#include "libANGLE/CLPlatform.h"

namespace cl
{

Platform::~Platform() = default;

void Platform::CreatePlatform(const cl_icd_dispatch &dispatch, rx::CLPlatformImpl::Ptr &&impl)
{
    GetList().emplace_back(new Platform(dispatch, std::move(impl)));
}

Platform::Platform(const cl_icd_dispatch &dispatch, rx::CLPlatformImpl::Ptr &&impl)
    : _cl_platform_id(dispatch), mImpl(std::move(impl))
{}

constexpr char Platform::kVendor[];
constexpr char Platform::kIcdSuffix[];

}  // namespace cl
