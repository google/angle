//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLContextCL.cpp: Implements the class methods for CLContextCL.

#include "libANGLE/renderer/cl/CLContextCL.h"

#include "libANGLE/renderer/cl/CLPlatformCL.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLContextCL::CLContextCL(CLPlatformCL &platform, CLDeviceImpl::List &&devices, cl_context context)
    : CLContextImpl(platform, std::move(devices)), mContext(context)
{}

CLContextCL::~CLContextCL()
{
    if (mContext->getDispatch().clReleaseContext(mContext) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL context";
    }
}

}  // namespace rx
