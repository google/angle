//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLKernelCL.cpp: Implements the class methods for CLKernelCL.

#include "libANGLE/renderer/cl/CLKernelCL.h"

#include "libANGLE/renderer/cl/CLDeviceCL.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLKernel.h"
#include "libANGLE/CLPlatform.h"
#include "libANGLE/CLProgram.h"
#include "libANGLE/Debug.h"

namespace rx
{

namespace
{

template <typename T>
bool GetWorkGroupInfo(cl_kernel kernel,
                      cl_device_id device,
                      cl::KernelWorkGroupInfo name,
                      T &value,
                      cl_int &errorCode)
{
    errorCode = kernel->getDispatch().clGetKernelWorkGroupInfo(kernel, device, cl::ToCLenum(name),
                                                               sizeof(T), &value, nullptr);
    return errorCode == CL_SUCCESS;
}

template <typename T>
bool GetArgInfo(cl_kernel kernel,
                cl_uint index,
                cl::KernelArgInfo name,
                T &value,
                cl_int &errorCode)
{
    errorCode = kernel->getDispatch().clGetKernelArgInfo(kernel, index, cl::ToCLenum(name),
                                                         sizeof(T), &value, nullptr);
    return errorCode == CL_SUCCESS || errorCode == CL_KERNEL_ARG_INFO_NOT_AVAILABLE;
}

template <typename T>
bool GetKernelInfo(cl_kernel kernel, cl::KernelInfo name, T &value, cl_int &errorCode)
{
    errorCode = kernel->getDispatch().clGetKernelInfo(kernel, cl::ToCLenum(name), sizeof(T), &value,
                                                      nullptr);
    return errorCode == CL_SUCCESS;
}

bool GetArgString(cl_kernel kernel,
                  cl_uint index,
                  cl::KernelArgInfo name,
                  std::string &string,
                  cl_int &errorCode)
{
    size_t size = 0u;
    errorCode   = kernel->getDispatch().clGetKernelArgInfo(kernel, index, cl::ToCLenum(name), 0u,
                                                         nullptr, &size);
    if (errorCode == CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
    {
        return true;
    }
    else if (errorCode != CL_SUCCESS)
    {
        return false;
    }
    std::vector<char> valString(size, '\0');
    errorCode = kernel->getDispatch().clGetKernelArgInfo(kernel, index, cl::ToCLenum(name), size,
                                                         valString.data(), nullptr);
    if (errorCode != CL_SUCCESS)
    {
        return false;
    }
    string.assign(valString.data(), valString.size() - 1u);
    return true;
}

bool GetKernelString(cl_kernel kernel, cl::KernelInfo name, std::string &string, cl_int &errorCode)
{
    size_t size = 0u;
    errorCode =
        kernel->getDispatch().clGetKernelInfo(kernel, cl::ToCLenum(name), 0u, nullptr, &size);
    if (errorCode != CL_SUCCESS)
    {
        return false;
    }
    std::vector<char> valString(size, '\0');
    errorCode = kernel->getDispatch().clGetKernelInfo(kernel, cl::ToCLenum(name), size,
                                                      valString.data(), nullptr);
    if (errorCode != CL_SUCCESS)
    {
        return false;
    }
    string.assign(valString.data(), valString.size() - 1u);
    return true;
}

}  // namespace

CLKernelCL::CLKernelCL(const cl::Kernel &kernel, cl_kernel native)
    : CLKernelImpl(kernel), mNative(native)
{}

CLKernelCL::~CLKernelCL()
{
    if (mNative->getDispatch().clReleaseKernel(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL kernel";
    }
}

CLKernelImpl::Info CLKernelCL::createInfo(cl_int &errorCode) const
{
    const cl::Context &ctx = mKernel.getProgram().getContext();
    Info info;

    if (!GetKernelString(mNative, cl::KernelInfo::FunctionName, info.mFunctionName, errorCode) ||
        !GetKernelInfo(mNative, cl::KernelInfo::NumArgs, info.mNumArgs, errorCode) ||
        (ctx.getPlatform().isVersionOrNewer(1u, 2u) &&
         !GetKernelString(mNative, cl::KernelInfo::Attributes, info.mAttributes, errorCode)))
    {
        return Info{};
    }

    info.mWorkGroups.resize(ctx.getDevices().size());
    for (size_t index = 0u; index < ctx.getDevices().size(); ++index)
    {
        const cl_device_id device = ctx.getDevices()[index]->getImpl<CLDeviceCL>().getNative();
        WorkGroupInfo &workGroup  = info.mWorkGroups[index];
        if ((ctx.getPlatform().isVersionOrNewer(1u, 2u) &&
             !GetWorkGroupInfo(mNative, device, cl::KernelWorkGroupInfo::GlobalWorkSize,
                               workGroup.mGlobalWorkSize, errorCode)) ||
            !GetWorkGroupInfo(mNative, device, cl::KernelWorkGroupInfo::WorkGroupSize,
                              workGroup.mWorkGroupSize, errorCode) ||
            !GetWorkGroupInfo(mNative, device, cl::KernelWorkGroupInfo::CompileWorkGroupSize,
                              workGroup.mCompileWorkGroupSize, errorCode) ||
            !GetWorkGroupInfo(mNative, device, cl::KernelWorkGroupInfo::LocalMemSize,
                              workGroup.mLocalMemSize, errorCode) ||
            !GetWorkGroupInfo(mNative, device,
                              cl::KernelWorkGroupInfo::PreferredWorkGroupSizeMultiple,
                              workGroup.mPrefWorkGroupSizeMultiple, errorCode) ||
            !GetWorkGroupInfo(mNative, device, cl::KernelWorkGroupInfo::PrivateMemSize,
                              workGroup.mPrivateMemSize, errorCode))
        {
            return Info{};
        }
    }

    info.mArgs.resize(info.mNumArgs);
    if (ctx.getPlatform().isVersionOrNewer(1u, 2u))
    {
        for (cl_uint index = 0u; index < info.mNumArgs; ++index)
        {
            ArgInfo &arg = info.mArgs[index];
            if (!GetArgInfo(mNative, index, cl::KernelArgInfo::AddressQualifier,
                            arg.mAddressQualifier, errorCode) ||
                !GetArgInfo(mNative, index, cl::KernelArgInfo::AccessQualifier,
                            arg.mAccessQualifier, errorCode) ||
                !GetArgString(mNative, index, cl::KernelArgInfo::TypeName, arg.mTypeName,
                              errorCode) ||
                !GetArgInfo(mNative, index, cl::KernelArgInfo::TypeQualifier, arg.mTypeQualifier,
                            errorCode) ||
                !GetArgString(mNative, index, cl::KernelArgInfo::Name, arg.mName, errorCode))
            {
                return Info{};
            }
        }
    }

    return info;
}

}  // namespace rx
