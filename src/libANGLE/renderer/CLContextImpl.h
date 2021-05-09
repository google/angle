//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContextImpl.h: Defines the abstract rx::CLContextImpl class.

#ifndef LIBANGLE_RENDERER_CLCONTEXTIMPL_H_
#define LIBANGLE_RENDERER_CLCONTEXTIMPL_H_

#include "libANGLE/renderer/CLDeviceImpl.h"

namespace rx
{

class CLContextImpl : angle::NonCopyable
{
  public:
    using Ptr  = std::unique_ptr<CLContextImpl>;
    using List = std::list<CLContextImpl *>;

    CLContextImpl(CLPlatformImpl &platform, CLDeviceImpl::List &&devices);
    virtual ~CLContextImpl();

    template <typename T>
    T &getPlatform() const
    {
        return static_cast<T &>(mPlatform);
    }

    const CLDeviceImpl::List &getDevices() const;

  protected:
    CLPlatformImpl &mPlatform;
    const CLDeviceImpl::List mDevices;
};

inline const CLDeviceImpl::List &CLContextImpl::getDevices() const
{
    return mDevices;
}

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLCONTEXTIMPL_H_
