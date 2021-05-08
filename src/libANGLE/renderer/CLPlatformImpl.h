//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformImpl.h: Defines the abstract rx::CLPlatformImpl class.

#ifndef LIBANGLE_RENDERER_CLPLATFORMIMPL_H_
#define LIBANGLE_RENDERER_CLPLATFORMIMPL_H_

#include "libANGLE/renderer/CLDeviceImpl.h"

namespace rx
{

class CLPlatformImpl : angle::NonCopyable
{
  public:
    struct Info
    {
        Info();
        ~Info();

        Info(const Info &) = delete;
        Info &operator=(const Info &) = delete;

        Info(Info &&);
        Info &operator=(Info &&);

        bool isValid() const;

        std::string mProfile;
        std::string mVersionStr;
        cl_version mVersion;
        std::string mName;
        std::string mExtensions;
        NameVersionVector mExtensionsWithVersion;
        cl_ulong mHostTimerRes;
    };

    using Ptr      = std::unique_ptr<CLPlatformImpl>;
    using InitData = std::pair<Ptr, Info>;
    using InitList = std::list<InitData>;

    CLPlatformImpl()          = default;
    virtual ~CLPlatformImpl() = default;

    virtual CLDeviceImpl::InitList getDevices() = 0;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLPLATFORMIMPL_H_
