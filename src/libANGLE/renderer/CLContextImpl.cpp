//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContextImpl.cpp: Implements the class methods for CLContextImpl.

#include "libANGLE/renderer/CLContextImpl.h"

#include "libANGLE/renderer/CLPlatformImpl.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLContextImpl::CLContextImpl(CLPlatformImpl &platform, CLDeviceImpl::List &&devices)
    : mPlatform(platform), mDevices(std::move(devices))
{}

CLContextImpl::~CLContextImpl()
{
    auto it = std::find(mPlatform.mContexts.cbegin(), mPlatform.mContexts.cend(), this);
    if (it != mPlatform.mContexts.cend())
    {
        mPlatform.mContexts.erase(it);
    }
    else
    {
        ERR() << "Context not in platform's list";
    }
}

}  // namespace rx
