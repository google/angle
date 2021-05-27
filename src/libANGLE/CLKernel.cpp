//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLKernel.cpp: Implements the cl::Kernel class.

#include "libANGLE/CLKernel.h"

#include "libANGLE/CLPlatform.h"
#include "libANGLE/CLProgram.h"

#include <cstring>

namespace cl
{

Kernel::~Kernel() = default;

bool Kernel::release()
{
    const bool released = removeRef();
    if (released)
    {
        mProgram->destroyKernel(this);
    }
    return released;
}

cl_int Kernel::getInfo(KernelInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case KernelInfo::FunctionName:
            copyValue = mInfo.mFunctionName.c_str();
            copySize  = mInfo.mFunctionName.length() + 1u;
            break;
        case KernelInfo::NumArgs:
            copyValue = &mInfo.mNumArgs;
            copySize  = sizeof(mInfo.mNumArgs);
            break;
        case KernelInfo::ReferenceCount:
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
            break;
        case KernelInfo::Context:
            valPointer = static_cast<cl_context>(&mProgram->getContext());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case KernelInfo::Program:
            valPointer = static_cast<cl_program>(mProgram.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case KernelInfo::Attributes:
            copyValue = mInfo.mAttributes.c_str();
            copySize  = mInfo.mAttributes.length() + 1u;
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return type
        // as described in the Kernel Object Queries table and param_value is not NULL.
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

cl_int Kernel::getWorkGroupInfo(cl_device_id device,
                                KernelWorkGroupInfo name,
                                size_t valueSize,
                                void *value,
                                size_t *valueSizeRet) const
{
    size_t index = 0u;
    if (device != nullptr)
    {
        const DeviceRefs &devices = mProgram->getContext().getDevices();
        while (index < devices.size() && devices[index].get() != device)
        {
            ++index;
        }
        if (index == devices.size())
        {
            return CL_INVALID_DEVICE;
        }
    }
    const rx::CLKernelImpl::WorkGroupInfo &info = mInfo.mWorkGroups[index];

    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case KernelWorkGroupInfo::GlobalWorkSize:
            copyValue = &info.mGlobalWorkSize;
            copySize  = sizeof(info.mGlobalWorkSize);
            break;
        case KernelWorkGroupInfo::WorkGroupSize:
            copyValue = &info.mWorkGroupSize;
            copySize  = sizeof(info.mWorkGroupSize);
            break;
        case KernelWorkGroupInfo::CompileWorkGroupSize:
            copyValue = &info.mCompileWorkGroupSize;
            copySize  = sizeof(info.mCompileWorkGroupSize);
            break;
        case KernelWorkGroupInfo::LocalMemSize:
            copyValue = &info.mLocalMemSize;
            copySize  = sizeof(info.mLocalMemSize);
            break;
        case KernelWorkGroupInfo::PreferredWorkGroupSizeMultiple:
            copyValue = &info.mPrefWorkGroupSizeMultiple;
            copySize  = sizeof(info.mPrefWorkGroupSizeMultiple);
            break;
        case KernelWorkGroupInfo::PrivateMemSize:
            copyValue = &info.mPrivateMemSize;
            copySize  = sizeof(info.mPrivateMemSize);
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return type
        // as described in the Kernel Object Device Queries table and param_value is not NULL.
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

cl_int Kernel::getArgInfo(cl_uint argIndex,
                          KernelArgInfo name,
                          size_t valueSize,
                          void *value,
                          size_t *valueSizeRet) const
{
    const rx::CLKernelImpl::ArgInfo &info = mInfo.mArgs[argIndex];
    const void *copyValue                 = nullptr;
    size_t copySize                       = 0u;

    switch (name)
    {
        case KernelArgInfo::AddressQualifier:
            copyValue = &info.mAddressQualifier;
            copySize  = sizeof(info.mAddressQualifier);
            break;
        case KernelArgInfo::AccessQualifier:
            copyValue = &info.mAccessQualifier;
            copySize  = sizeof(info.mAccessQualifier);
            break;
        case KernelArgInfo::TypeName:
            copyValue = info.mTypeName.c_str();
            copySize  = info.mTypeName.length() + 1u;
            break;
        case KernelArgInfo::TypeQualifier:
            copyValue = &info.mTypeQualifier;
            copySize  = sizeof(info.mTypeQualifier);
            break;
        case KernelArgInfo::Name:
            copyValue = info.mName.c_str();
            copySize  = info.mName.length() + 1u;
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value size is < size of return type
        // as described in the Kernel Argument Queries table and param_value is not NULL.
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

bool Kernel::IsValid(const _cl_kernel *kernel)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasKernel(kernel);
           }) != platforms.cend();
}

bool Kernel::IsValidAndVersionOrNewer(const _cl_kernel *kernel, cl_uint major, cl_uint minor)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->isVersionOrNewer(major, minor) && platform->hasKernel(kernel);
           }) != platforms.cend();
}

Kernel::Kernel(Program &program, const char *name, cl_int &errorCode)
    : _cl_kernel(program.getDispatch()),
      mProgram(&program),
      mImpl(program.mImpl->createKernel(*this, name, errorCode)),
      mInfo(mImpl ? mImpl->createInfo(errorCode) : rx::CLKernelImpl::Info{})
{}

Kernel::Kernel(Program &program, const CreateImplFunc &createImplFunc, cl_int &errorCode)
    : _cl_kernel(program.getDispatch()),
      mProgram(&program),
      mImpl(createImplFunc(*this)),
      mInfo(mImpl->createInfo(errorCode))
{}

}  // namespace cl
