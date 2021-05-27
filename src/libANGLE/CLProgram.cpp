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

Program::~Program() = default;

bool Program::release()
{
    const bool released = removeRef();
    if (released)
    {
        mContext->destroyProgram(this);
    }
    return released;
}

cl_int Program::getInfo(ProgramInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    static_assert(std::is_same<cl_uint, cl_bool>::value &&
                      std::is_same<cl_uint, cl_addressing_mode>::value &&
                      std::is_same<cl_uint, cl_filter_mode>::value,
                  "OpenCL type mismatch");

    std::vector<size_t> binarySizes;
    std::vector<const unsigned char *> binaries;
    cl_uint valUInt       = 0u;
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case ProgramInfo::ReferenceCount:
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
            break;
        case ProgramInfo::Context:
            valPointer = static_cast<cl_context>(mContext.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case ProgramInfo::NumDevices:
            valUInt   = static_cast<decltype(valUInt)>(mDevices.size());
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case ProgramInfo::Devices:
            static_assert(sizeof(decltype(mDevices)::value_type) == sizeof(Device *),
                          "DeviceRefs has wrong element size");
            copyValue = mDevices.data();
            copySize  = mDevices.size() * sizeof(decltype(mDevices)::value_type);
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
    return createKernel(new Kernel(*this, kernel_name, errorCode));
}

cl_int Program::createKernel(const Kernel::CreateImplFunc &createImplFunc)
{
    cl_int errorCode = CL_SUCCESS;
    createKernel(new Kernel(*this, createImplFunc, errorCode));
    return errorCode;
}

cl_int Program::createKernels(cl_uint numKernels, cl_kernel *kernels, cl_uint *numKernelsRet)
{
    cl_int errorCode = mImpl->createKernels(*this);
    if (errorCode == CL_SUCCESS)
    {
        // CL_INVALID_VALUE if kernels is not NULL and
        // num_kernels is less than the number of kernels in program.
        if (kernels != nullptr && numKernels < mKernels.size())
        {
            errorCode = CL_INVALID_VALUE;
        }
        else
        {
            if (kernels != nullptr)
            {
                for (const KernelPtr &kernel : mKernels)
                {
                    *kernels++ = kernel.get();
                }
            }
            if (numKernelsRet != nullptr)
            {
                *numKernelsRet = static_cast<cl_uint>(mKernels.size());
            }
        }
    }
    return errorCode;
}

bool Program::IsValid(const _cl_program *program)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasProgram(program);
           }) != platforms.cend();
}

Program::Program(Context &context, std::string &&source, cl_int &errorCode)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(context.getDevices()),
      mImpl(context.mImpl->createProgramWithSource(*this, source, errorCode)),
      mSource(std::move(source))
{}

Program::Program(Context &context, const void *il, size_t length, cl_int &errorCode)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(context.getDevices()),
      mIL(static_cast<const char *>(il), length),
      mImpl(context.mImpl->createProgramWithIL(*this, il, length, errorCode)),
      mSource(mImpl ? mImpl->getSource(errorCode) : std::string{})
{}

Program::Program(Context &context,
                 DeviceRefs &&devices,
                 Binaries &&binaries,
                 cl_int *binaryStatus,
                 cl_int &errorCode)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(std::move(devices)),
      mImpl(context.mImpl->createProgramWithBinary(*this, binaries, binaryStatus, errorCode)),
      mSource(mImpl ? mImpl->getSource(errorCode) : std::string{}),
      mBinaries(std::move(binaries))
{}

Program::Program(Context &context, DeviceRefs &&devices, const char *kernelNames, cl_int &errorCode)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(std::move(devices)),
      mImpl(context.mImpl->createProgramWithBuiltInKernels(*this, kernelNames, errorCode)),
      mSource(mImpl ? mImpl->getSource(errorCode) : std::string{})
{}

cl_kernel Program::createKernel(Kernel *kernel)
{
    mKernels.emplace_back(kernel);
    if (!mKernels.back()->mImpl)
    {
        mKernels.back()->release();
        return nullptr;
    }
    return mKernels.back().get();
}

void Program::destroyKernel(Kernel *kernel)
{
    auto kernelIt = mKernels.cbegin();
    while (kernelIt != mKernels.cend() && kernelIt->get() != kernel)
    {
        ++kernelIt;
    }
    if (kernelIt != mKernels.cend())
    {
        mKernels.erase(kernelIt);
    }
    else
    {
        ERR() << "Kernel not found";
    }
}

}  // namespace cl
