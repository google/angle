//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContext.h: Defines the cl::Context class, which manages OpenCL objects such as command-queues,
// memory, program and kernel objects and for executing kernels on one or more devices.

#ifndef LIBANGLE_CLCONTEXT_H_
#define LIBANGLE_CLCONTEXT_H_

#include "libANGLE/CLDevice.h"
#include "libANGLE/CLRefPointer.h"
#include "libANGLE/renderer/CLContextImpl.h"

#include <list>

namespace cl
{

class Context final : public _cl_context, public Object
{
  public:
    using PtrList   = std::list<ContextPtr>;
    using RefPtr    = RefPointer<Context>;
    using PropArray = std::vector<cl_context_properties>;

    ~Context();

    Platform &getPlatform() const noexcept;

    void retain() noexcept;
    bool release();

    cl_int getInfo(ContextInfo name, size_t valueSize, void *value, size_t *valueSizeRet);

    static bool IsValid(const _cl_context *context);

  private:
    Context(Platform &platform,
            PropArray &&properties,
            DeviceRefList &&devices,
            ContextErrorCB notify,
            void *userData,
            bool userSync,
            cl_int *errcodeRet);

    Context(Platform &platform,
            PropArray &&properties,
            cl_device_type deviceType,
            ContextErrorCB notify,
            void *userData,
            bool userSync,
            cl_int *errcodeRet);

    static void CL_CALLBACK ErrorCallback(const char *errinfo,
                                          const void *privateInfo,
                                          size_t cb,
                                          void *userData);

    Platform &mPlatform;
    const rx::CLContextImpl::Ptr mImpl;
    const PropArray mProperties;
    const DeviceRefList mDevices;
    const ContextErrorCB mNotify;
    void *const mUserData;

    friend class Platform;
};

inline Platform &Context::getPlatform() const noexcept
{
    return mPlatform;
}

inline void Context::retain() noexcept
{
    addRef();
}

}  // namespace cl

#endif  // LIBANGLE_CLCONTEXT_H_
