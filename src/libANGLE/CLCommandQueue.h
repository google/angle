//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueue.h: Defines the cl::CommandQueue class, which can be used to queue a set of OpenCL
// operations.

#ifndef LIBANGLE_CLCOMMANDQUEUE_H_
#define LIBANGLE_CLCOMMANDQUEUE_H_

#include "libANGLE/CLObject.h"
#include "libANGLE/renderer/CLCommandQueueImpl.h"

#include <limits>

namespace cl
{

class CommandQueue final : public _cl_command_queue, public Object
{
  public:
    using PtrList   = std::list<CommandQueuePtr>;
    using PropArray = std::vector<cl_queue_properties>;

    static constexpr cl_uint kNoSize = std::numeric_limits<cl_uint>::max();

    ~CommandQueue() override;

    const Context &getContext() const;
    const Device &getDevice() const;

    cl_command_queue_properties getProperties() const;
    bool hasSize() const;
    cl_uint getSize() const;

    void retain() noexcept;
    bool release();

    cl_int getInfo(CommandQueueInfo name,
                   size_t valueSize,
                   void *value,
                   size_t *valueSizeRet) const;

    cl_int setProperty(cl_command_queue_properties properties,
                       cl_bool enable,
                       cl_command_queue_properties *oldProperties);

    static bool IsValid(const _cl_command_queue *commandQueue);

  private:
    CommandQueue(Context &context,
                 Device &device,
                 cl_command_queue_properties properties,
                 cl_int *errcodeRet);

    CommandQueue(Context &context,
                 Device &device,
                 PropArray &&propArray,
                 cl_command_queue_properties properties,
                 cl_uint size,
                 cl_int *errcodeRet);

    const ContextRefPtr mContext;
    const DeviceRefPtr mDevice;
    const PropArray mPropArray;
    cl_command_queue_properties mProperties;
    const cl_uint mSize = kNoSize;
    const rx::CLCommandQueueImpl::Ptr mImpl;

    friend class Context;
};

inline const Context &CommandQueue::getContext() const
{
    return *mContext;
}

inline const Device &CommandQueue::getDevice() const
{
    return *mDevice;
}

inline cl_command_queue_properties CommandQueue::getProperties() const
{
    return mProperties;
}

inline bool CommandQueue::hasSize() const
{
    return mSize != kNoSize;
}

inline cl_uint CommandQueue::getSize() const
{
    return mSize;
}

inline void CommandQueue::retain() noexcept
{
    addRef();
}

}  // namespace cl

#endif  // LIBANGLE_CLCOMMANDQUEUE_H_
