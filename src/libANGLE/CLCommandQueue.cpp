//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueue.cpp: Implements the cl::CommandQueue class.

#include "libANGLE/CLCommandQueue.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLDevice.h"

#include <cstring>

namespace cl
{

CommandQueue::~CommandQueue()
{
    if (mDevice->mDefaultCommandQueue == this)
    {
        mDevice->mDefaultCommandQueue = nullptr;
    }
}

cl_int CommandQueue::getInfo(CommandQueueInfo name,
                             size_t valueSize,
                             void *value,
                             size_t *valueSizeRet) const
{
    cl_uint valUInt       = 0u;
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case CommandQueueInfo::Context:
            valPointer = mContext->getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case CommandQueueInfo::Device:
            valPointer = mDevice->getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case CommandQueueInfo::ReferenceCount:
            valUInt   = getRefCount();
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case CommandQueueInfo::Properties:
            copyValue = &mProperties;
            copySize  = sizeof(mProperties);
            break;
        case CommandQueueInfo::PropertiesArray:
            copyValue = mPropArray.data();
            copySize  = mPropArray.size() * sizeof(decltype(mPropArray)::value_type);
            break;
        case CommandQueueInfo::Size:
            copyValue = &mSize;
            copySize  = sizeof(mSize);
            break;
        case CommandQueueInfo::DeviceDefault:
            valPointer = CommandQueue::CastNative(mDevice->mDefaultCommandQueue);
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return type
        // as specified in the Command Queue Parameter table, and param_value is not a NULL value.
        if (valueSize < copySize)
        {
            return CL_INVALID_VALUE;
        }
        if (copyValue != nullptr)
        {
            std::memcpy(value, copyValue, copySize);
        }
    }
    if (valueSizeRet != nullptr)
    {
        *valueSizeRet = copySize;
    }
    return CL_SUCCESS;
}

cl_int CommandQueue::setProperty(CommandQueueProperties properties,
                                 cl_bool enable,
                                 cl_command_queue_properties *oldProperties)
{
    if (oldProperties != nullptr)
    {
        *oldProperties = mProperties.get();
    }
    const cl_int result = mImpl->setProperty(properties, enable);
    if (result == CL_SUCCESS)
    {
        if (enable == CL_FALSE)
        {
            mProperties.clear(properties);
        }
        else
        {
            mProperties.set(properties);
        }
    }
    return result;
}

CommandQueue::CommandQueue(Context &context,
                           Device &device,
                           CommandQueueProperties properties,
                           cl_int &errorCode)
    : mContext(&context),
      mDevice(&device),
      mProperties(properties),
      mImpl(context.getImpl().createCommandQueue(*this, errorCode))
{}

CommandQueue::CommandQueue(Context &context,
                           Device &device,
                           PropArray &&propArray,
                           CommandQueueProperties properties,
                           cl_uint size,
                           cl_int &errorCode)
    : mContext(&context),
      mDevice(&device),
      mPropArray(std::move(propArray)),
      mProperties(properties),
      mSize(size),
      mImpl(context.getImpl().createCommandQueue(*this, errorCode))
{
    if (mProperties.isSet(CL_QUEUE_ON_DEVICE_DEFAULT))
    {
        mDevice->mDefaultCommandQueue = this;
    }
}

}  // namespace cl
