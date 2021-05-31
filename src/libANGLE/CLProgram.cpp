//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgram.cpp: Implements the cl::Program class.

#include "libANGLE/CLProgram.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLPlatform.h"

#include <cstring>

namespace cl
{

cl_int Program::getInfo(ProgramInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    static_assert(std::is_same<cl_uint, cl_bool>::value &&
                      std::is_same<cl_uint, cl_addressing_mode>::value &&
                      std::is_same<cl_uint, cl_filter_mode>::value,
                  "OpenCL type mismatch");

    std::vector<cl_device_id> devices;
    std::vector<size_t> binarySizes;
    std::vector<const unsigned char *> binaries;
    cl_uint valUInt       = 0u;
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case ProgramInfo::ReferenceCount:
            valUInt   = getRefCount();
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case ProgramInfo::Context:
            valPointer = mContext->getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case ProgramInfo::NumDevices:
            valUInt   = static_cast<decltype(valUInt)>(mDevices.size());
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case ProgramInfo::Devices:
            devices.reserve(mDevices.size());
            for (const DevicePtr &device : mDevices)
            {
                devices.emplace_back(device->getNative());
            }
            copyValue = devices.data();
            copySize  = devices.size() * sizeof(decltype(devices)::value_type);
            break;
        case ProgramInfo::Source:
            copyValue = mSource.c_str();
            copySize  = mSource.length() + 1u;
            break;
        case ProgramInfo::IL:
            copyValue = mIL.c_str();
            copySize  = mIL.length() + 1u;
            break;
        case ProgramInfo::BinarySizes:
            binarySizes.resize(mDevices.size(), 0u);
            for (size_t index = 0u; index < binarySizes.size(); ++index)
            {
                binarySizes[index] = index < mBinaries.size() ? mBinaries[index].size() : 0u;
            }
            copyValue = binarySizes.data();
            copySize  = binarySizes.size() * sizeof(decltype(binarySizes)::value_type);
            break;
        case ProgramInfo::Binaries:
            binaries.resize(mDevices.size(), nullptr);
            for (size_t index = 0u; index < binaries.size(); ++index)
            {
                binaries[index] = index < mBinaries.size() && mBinaries[index].empty()
                                      ? mBinaries[index].data()
                                      : nullptr;
            }
            copyValue = binaries.data();
            copySize  = binaries.size() * sizeof(decltype(binaries)::value_type);
            break;
        case ProgramInfo::NumKernels:
            copyValue = &mNumKernels;
            copySize  = sizeof(mNumKernels);
            break;
        case ProgramInfo::KernelNames:
            copyValue = mKernelNames.c_str();
            copySize  = mKernelNames.length() + 1u;
            break;
        case ProgramInfo::ScopeGlobalCtorsPresent:
            valUInt   = CL_FALSE;
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case ProgramInfo::ScopeGlobalDtorsPresent:
            valUInt   = CL_FALSE;
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return type
        // as described in the Program Object Queries table and param_value is not NULL.
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

cl_kernel Program::createKernel(const char *kernel_name, cl_int &errorCode)
{
    return Object::Create<Kernel>(errorCode, *this, kernel_name);
}

cl_int Program::createKernels(cl_uint numKernels, cl_kernel *kernels, cl_uint *numKernelsRet)
{
    if (kernels == nullptr)
    {
        numKernels = 0u;
    }
    rx::CLKernelImpl::CreateFuncs createFuncs;
    cl_int errorCode = mImpl->createKernels(numKernels, createFuncs, numKernelsRet);
    if (errorCode == CL_SUCCESS)
    {
        KernelPtrs krnls;
        krnls.reserve(createFuncs.size());
        while (!createFuncs.empty())
        {
            krnls.emplace_back(new Kernel(*this, createFuncs.front(), errorCode));
            if (errorCode != CL_SUCCESS)
            {
                return CL_INVALID_VALUE;
            }
            createFuncs.pop_front();
        }
        for (KernelPtr &kernel : krnls)
        {
            *kernels++ = kernel.release();
        }
    }
    return errorCode;
}

Program::~Program() = default;

Program::Program(Context &context, std::string &&source, cl_int &errorCode)
    : mContext(&context),
      mDevices(context.getDevices()),
      mImpl(context.getImpl().createProgramWithSource(*this, source, errorCode)),
      mSource(std::move(source))
{}

Program::Program(Context &context, const void *il, size_t length, cl_int &errorCode)
    : mContext(&context),
      mDevices(context.getDevices()),
      mIL(static_cast<const char *>(il), length),
      mImpl(context.getImpl().createProgramWithIL(*this, il, length, errorCode)),
      mSource(mImpl ? mImpl->getSource(errorCode) : std::string{})
{}

Program::Program(Context &context,
                 DevicePtrs &&devices,
                 Binaries &&binaries,
                 cl_int *binaryStatus,
                 cl_int &errorCode)
    : mContext(&context),
      mDevices(std::move(devices)),
      mImpl(context.getImpl().createProgramWithBinary(*this, binaries, binaryStatus, errorCode)),
      mSource(mImpl ? mImpl->getSource(errorCode) : std::string{}),
      mBinaries(std::move(binaries))
{}

Program::Program(Context &context, DevicePtrs &&devices, const char *kernelNames, cl_int &errorCode)
    : mContext(&context),
      mDevices(std::move(devices)),
      mImpl(context.getImpl().createProgramWithBuiltInKernels(*this, kernelNames, errorCode)),
      mSource(mImpl ? mImpl->getSource(errorCode) : std::string{})
{}

}  // namespace cl
