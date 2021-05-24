//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLSamplerCL.cpp: Implements the class methods for CLSamplerCL.

#include "libANGLE/renderer/cl/CLSamplerCL.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLSamplerCL::CLSamplerCL(const cl::Sampler &sampler, cl_sampler native)
    : CLSamplerImpl(sampler), mNative(native)
{}

CLSamplerCL::~CLSamplerCL()
{
    if (mNative->getDispatch().clReleaseSampler(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL sampler";
    }
}

}  // namespace rx
