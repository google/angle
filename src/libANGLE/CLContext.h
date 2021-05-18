//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContext.h: Defines the cl::Context class, which manages OpenCL objects such as command-queues,
// memory, program and kernel objects and for executing kernels on one or more devices.

#ifndef LIBANGLE_CLCONTEXT_H_
#define LIBANGLE_CLCONTEXT_H_

#include "libANGLE/CLCommandQueue.h"
#include "libANGLE/CLDevice.h"
#include "libANGLE/renderer/CLContextImpl.h"

namespace cl
{

class Context final : public _cl_context, public Object
{
  public:
    using PtrList   = std::list<ContextPtr>;
    using PropArray = std::vector<cl_context_properties>;

    ~Context();

    const Platform &getPlatform() const noexcept;
    bool hasDevice(const _cl_device_id *device) const;
    bool hasCommandQueue(const _cl_command_queue *commandQueue) const;

    void retain() noexcept;
    bool release();

    cl_int getInfo(ContextInfo name, size_t valueSize, void *value, size_t *valueSizeRet);

    cl_command_queue createCommandQueue(cl_device_id device,
                                        cl_command_queue_properties properties,
                                        cl_int *errcodeRet);

    cl_command_queue createCommandQueueWithProperties(cl_device_id device,
                                                      const cl_queue_properties *properties,
                                                      cl_int *errcodeRet);

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

    void destroyCommandQueue(CommandQueue *commandQueue);

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

    CommandQueue::PtrList mCommandQueues;

    friend class CommandQueue;
    friend class Platform;
};

inline const Platform &Context::getPlatform() const noexcept
{
    return mPlatform;
}

inline bool Context::hasDevice(const _cl_device_id *device) const
{
    return std::find_if(mDevices.cbegin(), mDevices.cend(), [=](const DeviceRefPtr &ptr) {
               return ptr.get() == device;
           }) != mDevices.cend();
}

inline bool Context::hasCommandQueue(const _cl_command_queue *commandQueue) const
{
    return std::find_if(mCommandQueues.cbegin(), mCommandQueues.cend(),
                        [=](const CommandQueuePtr &ptr) { return ptr.get() == commandQueue; }) !=
           mCommandQueues.cend();
}

inline void Context::retain() noexcept
{
    addRef();
}

}  // namespace cl

#endif  // LIBANGLE_CLCONTEXT_H_
