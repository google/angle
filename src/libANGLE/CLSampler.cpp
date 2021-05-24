//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLSampler.cpp: Implements the cl::Sampler class.

#include "libANGLE/CLSampler.h"

#include "libANGLE/CLContext.h"
#include "libANGLE/CLPlatform.h"

#include <cstring>

namespace cl
{

Sampler::~Sampler() = default;

bool Sampler::release()
{
    const bool released = removeRef();
    if (released)
    {
        mContext->destroySampler(this);
    }
    return released;
}

cl_int Sampler::getInfo(SamplerInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    static_assert(std::is_same<cl_uint, cl_addressing_mode>::value &&
                      std::is_same<cl_uint, cl_filter_mode>::value,
                  "OpenCL type mismatch");

    cl_uint valUInt       = 0u;
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case SamplerInfo::ReferenceCount:
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
            break;
        case SamplerInfo::Context:
            valPointer = static_cast<cl_context>(mContext.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case SamplerInfo::NormalizedCoords:
            copyValue = &mNormalizedCoords;
            copySize  = sizeof(mNormalizedCoords);
            break;
        case SamplerInfo::AddressingMode:
            valUInt   = ToCLenum(mAddressingMode);
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case SamplerInfo::FilterMode:
            valUInt   = ToCLenum(mFilterMode);
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case SamplerInfo::Properties:
            copyValue = mProperties.data();
            copySize  = mProperties.size() * sizeof(decltype(mProperties)::value_type);
            break;
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

bool Sampler::IsValid(const _cl_sampler *sampler)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasSampler(sampler);
           }) != platforms.cend();
}

Sampler::Sampler(Context &context,
                 PropArray &&properties,
                 cl_bool normalizedCoords,
                 AddressingMode addressingMode,
                 FilterMode filterMode,
                 cl_int *errcodeRet)
    : _cl_sampler(context.getDispatch()),
      mContext(&context),
      mProperties(std::move(properties)),
      mNormalizedCoords(normalizedCoords),
      mAddressingMode(addressingMode),
      mFilterMode(filterMode),
      mImpl(context.mImpl->createSampler(*this, errcodeRet))
{}

}  // namespace cl
