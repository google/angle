//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContext.cpp: Implements the cl::Context class.

#include "libANGLE/CLContext.h"

#include "libANGLE/CLPlatform.h"

#include <cstring>

namespace cl
{

Context::~Context() = default;

bool Context::release()
{
    const bool released = removeRef();
    if (released)
    {
        mPlatform.destroyContext(this);
    }
    return released;
}

cl_int Context::getInfo(ContextInfo name, size_t valueSize, void *value, size_t *valueSizeRet)
{
    cl_uint numDevices    = 0u;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case ContextInfo::ReferenceCount:
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
            break;
        case ContextInfo::NumDevices:
            numDevices = static_cast<decltype(numDevices)>(mDevices.size());
            copyValue  = &numDevices;
            copySize   = sizeof(numDevices);
            break;
        case ContextInfo::Devices:
            static_assert(sizeof(decltype(mDevices)::value_type) == sizeof(Device *),
                          "Device::RefList has wrong element size");
            copyValue = mDevices.data();
            copySize  = mDevices.size() * sizeof(decltype(mDevices)::value_type);
            break;
        case ContextInfo::Properties:
            copyValue = mProperties.data();
            copySize  = mProperties.size() * sizeof(decltype(mProperties)::value_type);
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

cl_command_queue Context::createCommandQueue(cl_device_id device,
                                             cl_command_queue_properties properties,
                                             cl_int *errcodeRet)
{
    mCommandQueues.emplace_back(new CommandQueue(
        ContextRefPtr(this), DeviceRefPtr(static_cast<Device *>(device)), properties, errcodeRet));
    if (!mCommandQueues.back()->mImpl)
    {
        mCommandQueues.back()->release();
        return nullptr;
    }
    return mCommandQueues.back().get();
}

cl_command_queue Context::createCommandQueueWithProperties(cl_device_id device,
                                                           const cl_queue_properties *properties,
                                                           cl_int *errcodeRet)
{
    CommandQueue::PropArray propArray;
    cl_command_queue_properties props = 0u;
    cl_uint size                      = CommandQueue::kNoSize;
    if (properties != nullptr)
    {
        const cl_queue_properties *propIt = properties;
        while (*propIt != 0)
        {
            switch (*propIt++)
            {
                case CL_QUEUE_PROPERTIES:
                    props = *propIt++;
                    break;
                case CL_QUEUE_SIZE:
                    size = static_cast<decltype(size)>(*propIt++);
                    break;
            }
        }
        // Include the trailing zero
        ++propIt;
        propArray.reserve(propIt - properties);
        propArray.insert(propArray.cend(), properties, propIt);
    }

    mCommandQueues.emplace_back(new CommandQueue(ContextRefPtr(this),
                                                 DeviceRefPtr(static_cast<Device *>(device)),
                                                 std::move(propArray), props, size, errcodeRet));
    if (!mCommandQueues.back()->mImpl)
    {
        mCommandQueues.back()->release();
        return nullptr;
    }
    return mCommandQueues.back().get();
}

bool Context::IsValid(const _cl_context *context)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasContext(context);
           }) != platforms.cend();
}

Context::Context(Platform &platform,
                 PropArray &&properties,
                 DeviceRefList &&devices,
                 ContextErrorCB notify,
                 void *userData,
                 bool userSync,
                 cl_int *errcodeRet)
    : _cl_context(platform.getDispatch()),
      mPlatform(platform),
      mImpl(
          platform.mImpl->createContext(*this, devices, ErrorCallback, this, userSync, errcodeRet)),
      mProperties(std::move(properties)),
      mDevices(std::move(devices)),
      mNotify(notify),
      mUserData(userData)
{}

Context::Context(Platform &platform,
                 PropArray &&properties,
                 cl_device_type deviceType,
                 ContextErrorCB notify,
                 void *userData,
                 bool userSync,
                 cl_int *errcodeRet)
    : _cl_context(platform.getDispatch()),
      mPlatform(platform),
      mImpl(platform.mImpl->createContextFromType(*this,
                                                  deviceType,
                                                  ErrorCallback,
                                                  this,
                                                  userSync,
                                                  errcodeRet)),
      mProperties(std::move(properties)),
      mDevices(mImpl ? mImpl->getDevices() : DeviceRefList{}),
      mNotify(notify),
      mUserData(userData)
{}

void Context::destroyCommandQueue(CommandQueue *commandQueue)
{
    auto commandQueueIt = mCommandQueues.cbegin();
    while (commandQueueIt != mCommandQueues.cend() && commandQueueIt->get() != commandQueue)
    {
        ++commandQueueIt;
    }
    if (commandQueueIt != mCommandQueues.cend())
    {
        mCommandQueues.erase(commandQueueIt);
    }
    else
    {
        ERR() << "CommandQueue not found";
    }
}

void Context::ErrorCallback(const char *errinfo, const void *privateInfo, size_t cb, void *userData)
{
    Context *const context = static_cast<Context *>(userData);
    if (!Context::IsValid(context))
    {
        WARN() << "Context error for invalid context";
        return;
    }
    if (context->mNotify != nullptr)
    {
        context->mNotify(errinfo, privateInfo, cb, context->mUserData);
    }
}

}  // namespace cl
