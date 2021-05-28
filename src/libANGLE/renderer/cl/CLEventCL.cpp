//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLEventCL.cpp: Implements the class methods for CLEventCL.

#include "libANGLE/renderer/cl/CLEventCL.h"

#include "libANGLE/CLPlatform.h"
#include "libANGLE/Debug.h"

namespace rx
{

CLEventCL::CLEventCL(const cl::Event &event, cl_event native) : CLEventImpl(event), mNative(native)
{}

CLEventCL::~CLEventCL()
{
    if (mNative->getDispatch().clReleaseEvent(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL event";
    }
}

cl_int CLEventCL::getCommandExecutionStatus(cl_int &executionStatus)
{
    return mNative->getDispatch().clGetEventInfo(mNative, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                                 sizeof(executionStatus), &executionStatus,
                                                 nullptr);
}

cl_int CLEventCL::setUserEventStatus(cl_int executionStatus)
{
    return mNative->getDispatch().clSetUserEventStatus(mNative, executionStatus);
}

cl_int CLEventCL::setCallback(cl_int commandExecCallbackType)
{
    return mNative->getDispatch().clSetEventCallback(mNative, commandExecCallbackType, Callback,
                                                     nullptr);
}

void CLEventCL::Callback(cl_event event, cl_int commandStatus, void *userData)
{
    const cl::EventRefPtr evt = cl::Platform::FindEvent(
        [=](const cl::EventPtr &ptr) { return ptr->getImpl<CLEventCL>().getNative() == event; });
    if (evt)
    {
        evt->callback(commandStatus);
    }
    else
    {
        WARN() << "Callback event not found";
    }
}

}  // namespace rx
