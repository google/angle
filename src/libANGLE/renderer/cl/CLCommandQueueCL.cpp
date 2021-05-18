//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLCommandQueueCL.cpp: Implements the class methods for CLCommandQueueCL.

#include "libANGLE/renderer/cl/CLCommandQueueCL.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLCommandQueueCL::CLCommandQueueCL(const cl::CommandQueue &commandQueue, cl_command_queue native)
    : CLCommandQueueImpl(commandQueue), mNative(native)
{}

CLCommandQueueCL::~CLCommandQueueCL()
{
    if (mNative->getDispatch().clReleaseCommandQueue(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL command-queue";
    }
}

cl_int CLCommandQueueCL::setProperty(cl_command_queue_properties properties, cl_bool enable)
{
    return mNative->getDispatch().clSetCommandQueueProperty(mNative, properties, enable, nullptr);
}

}  // namespace rx
