//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformImpl.cpp: Implements the class methods for CLPlatformImpl.

#include "libANGLE/renderer/CLPlatformImpl.h"

namespace rx
{

CLPlatformImpl::Info::Info() = default;

CLPlatformImpl::Info::~Info() = default;

CLPlatformImpl::Info::Info(Info &&) = default;

CLPlatformImpl::Info &CLPlatformImpl::Info::operator=(Info &&) = default;

CLPlatformImpl::Info::Info(std::string &&profile,
                           std::string &&versionStr,
                           cl_version version,
                           std::string &&name,
                           std::string &&extensions,
                           rx::CLPlatformImpl::ExtensionList &&extensionList,
                           cl_ulong hostTimerRes)
    : mProfile(std::move(profile)),
      mVersionStr(std::move(versionStr)),
      mVersion(version),
      mName(std::move(name)),
      mExtensions(std::move(extensions)),
      mExtensionList(std::move(extensionList)),
      mHostTimerRes(hostTimerRes)
{}

CLPlatformImpl::CLPlatformImpl(Info &&info) : mInfo(std::move(info)) {}

CLPlatformImpl::~CLPlatformImpl() = default;

}  // namespace rx
