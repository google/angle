//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceImpl.cpp: Implements the class methods for CLDeviceImpl.

#include "libANGLE/renderer/CLDeviceImpl.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLDeviceImpl::Info::Info() = default;

CLDeviceImpl::Info::~Info() = default;

CLDeviceImpl::Info::Info(Info &&) = default;

CLDeviceImpl::Info &CLDeviceImpl::Info::operator=(Info &&) = default;

CLDeviceImpl::CLDeviceImpl(CLPlatformImpl &platform, CLDeviceImpl *parent)
    : mPlatform(platform), mParent(parent)
{}

CLDeviceImpl::~CLDeviceImpl()
{
    if (mParent != nullptr)
    {
        auto it = std::find(mParent->mSubDevices.cbegin(), mParent->mSubDevices.cend(), this);
        if (it != mParent->mSubDevices.cend())
        {
            mParent->mSubDevices.erase(it);
        }
        else
        {
            ERR() << "Sub-device not in parent's list";
        }
    }
}

}  // namespace rx
