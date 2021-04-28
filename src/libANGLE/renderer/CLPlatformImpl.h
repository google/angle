//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CLPlatformImpl.h: Defines the abstract rx::CLPlatformImpl class.

#ifndef LIBANGLE_RENDERER_CLPLATFORMIMPL_H_
#define LIBANGLE_RENDERER_CLPLATFORMIMPL_H_

#include "libANGLE/renderer/CLtypes.h"

#include <list>
#include <vector>

namespace rx
{

class CLPlatformImpl : angle::NonCopyable
{
  public:
    using Ptr           = std::unique_ptr<CLPlatformImpl>;
    using ImplList      = std::list<Ptr>;
    using ExtensionList = std::vector<cl_name_version>;

    struct Info
    {
        Info();
        ~Info();

        Info(const Info &) = delete;
        Info &operator=(const Info &) = delete;

        Info(Info &&);
        Info &operator=(Info &&);

        Info(std::string &&profile,
             std::string &&versionStr,
             cl_version version,
             std::string &&name,
             std::string &&extensions,
             rx::CLPlatformImpl::ExtensionList &&extensionList,
             cl_ulong hostTimerRes);

        std::string mProfile;
        std::string mVersionStr;
        cl_version mVersion;
        std::string mName;
        std::string mExtensions;
        rx::CLPlatformImpl::ExtensionList mExtensionList;
        cl_ulong mHostTimerRes;
    };

    explicit CLPlatformImpl(Info &&info);
    virtual ~CLPlatformImpl();

    const Info &getInfo();

  protected:
    const Info mInfo;
};

inline const CLPlatformImpl::Info &CLPlatformImpl::getInfo()
{
    return mInfo;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLPLATFORMIMPL_H_
