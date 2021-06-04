//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgramCL.cpp: Implements the class methods for CLProgramCL.

#include "libANGLE/renderer/cl/CLProgramCL.h"

#include "libANGLE/renderer/cl/CLKernelCL.h"

namespace rx
{

CLProgramCL::CLProgramCL(const cl::Program &program, cl_program native)
    : CLProgramImpl(program), mNative(native)
{}

CLProgramCL::~CLProgramCL()
{
    if (mNative->getDispatch().clReleaseProgram(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL program";
    }
}

std::string CLProgramCL::getSource(cl_int &errorCode) const
{
    size_t size = 0u;
    errorCode =
        mNative->getDispatch().clGetProgramInfo(mNative, CL_PROGRAM_SOURCE, 0u, nullptr, &size);
    if (errorCode == CL_SUCCESS)
    {
        if (size != 0u)
        {
            std::vector<char> valString(size, '\0');
            errorCode = mNative->getDispatch().clGetProgramInfo(mNative, CL_PROGRAM_SOURCE, size,
                                                                valString.data(), nullptr);
            if (errorCode == CL_SUCCESS)
            {
                return std::string(valString.data(), valString.size() - 1u);
            }
        }
    }
    return std::string{};
}

CLKernelImpl::Ptr CLProgramCL::createKernel(const cl::Kernel &kernel,
                                            const char *name,
                                            cl_int &errorCode)
{
    const cl_kernel nativeKernel = mNative->getDispatch().clCreateKernel(mNative, name, &errorCode);
    return CLKernelImpl::Ptr(nativeKernel != nullptr ? new CLKernelCL(kernel, nativeKernel)
                                                     : nullptr);
}

cl_int CLProgramCL::createKernels(cl_uint numKernels,
                                  CLKernelImpl::CreateFuncs &createFuncs,
                                  cl_uint *numKernelsRet)
{
    if (numKernels == 0u)
    {
        return mNative->getDispatch().clCreateKernelsInProgram(mNative, 0u, nullptr, numKernelsRet);
    }

    std::vector<cl_kernel> nativeKernels(numKernels, nullptr);
    const cl_int errorCode = mNative->getDispatch().clCreateKernelsInProgram(
        mNative, numKernels, nativeKernels.data(), nullptr);
    if (errorCode == CL_SUCCESS)
    {
        for (cl_kernel nativeKernel : nativeKernels)
        {
            createFuncs.emplace_back([nativeKernel](const cl::Kernel &kernel) {
                return CLKernelImpl::Ptr(new CLKernelCL(kernel, nativeKernel));
            });
        }
    }
    return errorCode;
}

}  // namespace rx
