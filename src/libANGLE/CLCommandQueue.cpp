//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueue.cpp: Implements the cl::CommandQueue class.

#include "libANGLE/CLCommandQueue.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLDevice.h"
#include "libANGLE/CLPlatform.h"

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

bool CommandQueue::release()
{
    const bool released = removeRef();
    if (released)
    {
        mContext->destroyCommandQueue(this);
    }
    return released;
}

cl_int CommandQueue::getInfo(CommandQueueInfo name,
                             size_t valueSize,
                             void *value,
                             size_t *valueSizeRet) const
{
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case CommandQueueInfo::Context:
            valPointer = static_cast<cl_context>(mContext.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case CommandQueueInfo::Device:
            valPointer = static_cast<cl_device_id>(mDevice.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case CommandQueueInfo::ReferenceCount:
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
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
            valPointer = static_cast<cl_command_queue>(mDevice->mDefaultCommandQueue);
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
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

cl_int CommandQueue::setProperty(cl_command_queue_properties properties,
                                 cl_bool enable,
                                 cl_command_queue_properties *oldProperties)
{
    if (oldProperties != nullptr)
    {
        *oldProperties = mProperties;
    }
    const cl_int result = mImpl->setProperty(properties, enable);
    if (result == CL_SUCCESS)
    {
        if (enable == CL_FALSE)
        {
            mProperties &= ~properties;
        }
        else
        {
            mProperties |= properties;
        }
    }
    return result;
}

bool CommandQueue::IsValid(const _cl_command_queue *commandQueue)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasCommandQueue(commandQueue);
           }) != platforms.cend();
}

CommandQueue::CommandQueue(Context &context,
                           Device &device,
                           cl_command_queue_properties properties,
                           cl_int *errcodeRet)
    : _cl_command_queue(context.getDispatch()),
      mContext(&context),
      mDevice(&device),
      mProperties(properties),
      mImpl(context.mImpl->createCommandQueue(*this, errcodeRet))
{}

CommandQueue::CommandQueue(Context &context,
                           Device &device,
                           PropArray &&propArray,
                           cl_command_queue_properties properties,
                           cl_uint size,
                           cl_int *errcodeRet)
    : _cl_command_queue(context.getDispatch()),
      mContext(&context),
      mDevice(&device),
      mPropArray(std::move(propArray)),
      mProperties(properties),
      mSize(size),
      mImpl(context.mImpl->createCommandQueue(*this, errcodeRet))
{
    if ((mProperties & CL_QUEUE_ON_DEVICE_DEFAULT) != 0u)
    {
        mDevice->mDefaultCommandQueue = this;
    }
}

}  // namespace cl
