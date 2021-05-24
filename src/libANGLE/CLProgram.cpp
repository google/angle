//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgram.cpp: Implements the cl::Program class.

#include "libANGLE/CLProgram.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLPlatform.h"

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
    static_assert(std::is_same<cl_uint, cl_addressing_mode>::value &&
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
                          "DeviceRefList has wrong element size");
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
        case ProgramInfo::KernelNames:
        case ProgramInfo::ScopeGlobalCtorsPresent:
        case ProgramInfo::ScopeGlobalDtorsPresent:
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
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

bool Program::IsValid(const _cl_program *program)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasProgram(program);
           }) != platforms.cend();
}

Program::Program(Context &context, std::string &&source, cl_int *errcodeRet)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(context.getDevices()),
      mImpl(context.mImpl->createProgramWithSource(*this, source, errcodeRet)),
      mSource(std::move(source))
{}

Program::Program(Context &context, const void *il, size_t length, cl_int *errcodeRet)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(context.getDevices()),
      mIL(static_cast<const char *>(il), length),
      mImpl(context.mImpl->createProgramWithIL(*this, il, length, errcodeRet)),
      mSource(mImpl ? mImpl->getSource() : std::string{})
{}

Program::Program(Context &context,
                 DeviceRefList &&devices,
                 Binaries &&binaries,
                 cl_int *binaryStatus,
                 cl_int *errcodeRet)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(std::move(devices)),
      mImpl(context.mImpl->createProgramWithBinary(*this, binaries, binaryStatus, errcodeRet)),
      mSource(mImpl ? mImpl->getSource() : std::string{}),
      mBinaries(std::move(binaries))
{}

Program::Program(Context &context,
                 DeviceRefList &&devices,
                 const char *kernelNames,
                 cl_int *errcodeRet)
    : _cl_program(context.getDispatch()),
      mContext(&context),
      mDevices(std::move(devices)),
      mImpl(context.mImpl->createProgramWithBuiltInKernels(*this, kernelNames, errcodeRet)),
      mSource(mImpl ? mImpl->getSource() : std::string{})
{}

}  // namespace cl
