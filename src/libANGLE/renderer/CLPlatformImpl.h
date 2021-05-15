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
    using Ptr = std::unique_ptr<CLPlatformImpl>;

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

    explicit CLPlatformImpl(const cl::Platform &platform);
    virtual ~CLPlatformImpl();

    // For initialization only
    virtual Info createInfo() const                                       = 0;
    virtual cl::DevicePtrList createDevices(cl::Platform &platform) const = 0;

    virtual CLContextImpl::Ptr createContext(const cl::Context &context,
                                             const cl::DeviceRefList &devices,
                                             cl::ContextErrorCB notify,
                                             void *userData,
                                             bool userSync,
                                             cl_int *errcodeRet) = 0;

    virtual CLContextImpl::Ptr createContextFromType(const cl::Context &context,
                                                     cl_device_type deviceType,
                                                     cl::ContextErrorCB notify,
                                                     void *userData,
                                                     bool userSync,
                                                     cl_int *errcodeRet) = 0;

  protected:
    const cl::Platform &mPlatform;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLPLATFORMIMPL_H_
