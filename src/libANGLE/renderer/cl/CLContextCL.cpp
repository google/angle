//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContextCL.cpp: Implements the class methods for CLContextCL.

#include "libANGLE/renderer/cl/CLContextCL.h"

#include "libANGLE/renderer/cl/CLDeviceCL.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLDevice.h"
#include "libANGLE/CLPlatform.h"
#include "libANGLE/Debug.h"

namespace rx
{

CLContextCL::CLContextCL(const cl::Context &context, cl_context native)
    : CLContextImpl(context), mNative(native)
{}

CLContextCL::~CLContextCL()
{
    if (mNative->getDispatch().clReleaseContext(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL context";
    }
}

cl::DeviceRefList CLContextCL::getDevices() const
{
    size_t valueSize = 0u;
    cl_int result    = mNative->getDispatch().clGetContextInfo(mNative, CL_CONTEXT_DEVICES, 0u,
                                                            nullptr, &valueSize);
    if (result == CL_SUCCESS && (valueSize % sizeof(cl_device_id)) == 0u)
    {
        std::vector<cl_device_id> nativeDevices(valueSize / sizeof(cl_device_id), nullptr);
        result = mNative->getDispatch().clGetContextInfo(mNative, CL_CONTEXT_DEVICES, valueSize,
                                                         nativeDevices.data(), nullptr);
        if (result == CL_SUCCESS)
        {
            const cl::DevicePtrList &platformDevices = mContext.getPlatform().getDevices();
            cl::DeviceRefList devices;
            for (cl_device_id nativeDevice : nativeDevices)
            {
                auto it = platformDevices.cbegin();
                while (it != platformDevices.cend() &&
                       static_cast<CLDeviceCL &>((*it)->getImpl()).getNative() != nativeDevice)
                {
                    ++it;
                }
                if (it != platformDevices.cend())
                {
                    devices.emplace_back(it->get());
                }
                else
                {
                    ERR() << "Device not found in platform list";
                    return cl::DeviceRefList{};
                }
            }
            return devices;
        }
    }

    ERR() << "Error fetching devices from CL context, code: " << result;
    return cl::DeviceRefList{};
}

}  // namespace rx
