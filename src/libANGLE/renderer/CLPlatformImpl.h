//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLPlatformImpl.h: Defines the abstract rx::CLPlatformImpl class.

#ifndef LIBANGLE_RENDERER_CLPLATFORMIMPL_H_
#define LIBANGLE_RENDERER_CLPLATFORMIMPL_H_

#include "libANGLE/renderer/CLContextImpl.h"
#include "libANGLE/renderer/CLDeviceImpl.h"

#include <tuple>

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

        bool isValid() const { return mVersion != 0u; }

        std::string mProfile;
        std::string mVersionStr;
        cl_version mVersion = 0u;
        std::string mName;
        std::string mExtensions;
        NameVersionVector mExtensionsWithVersion;
        cl_ulong mHostTimerRes = 0u;
    };

    using Ptr      = std::unique_ptr<CLPlatformImpl>;
    using InitData = std::tuple<Ptr, Info, CLDeviceImpl::PtrList>;
    using InitList = std::list<InitData>;

    explicit CLPlatformImpl(CLDeviceImpl::List &&devices);
    virtual ~CLPlatformImpl();

    const CLDeviceImpl::List &getDevices() const;

    virtual CLContextImpl::Ptr createContext(CLDeviceImpl::List &&devices,
                                             cl::ContextErrorCB notify,
                                             void *userData,
                                             bool userSync,
                                             cl_int *errcodeRet) = 0;

    virtual CLContextImpl::Ptr createContextFromType(cl_device_type deviceType,
                                                     cl::ContextErrorCB notify,
                                                     void *userData,
                                                     bool userSync,
                                                     cl_int *errcodeRet) = 0;

  protected:
    const CLDeviceImpl::List mDevices;

    CLContextImpl::List mContexts;

    friend class CLContextImpl;
};

inline const CLDeviceImpl::List &CLPlatformImpl::getDevices() const
{
    return mDevices;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLPLATFORMIMPL_H_
