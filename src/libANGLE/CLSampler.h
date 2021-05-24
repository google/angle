//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLSampler.h: Defines the cl::Sampler class, which describes how to sample an OpenCL Image.

#ifndef LIBANGLE_CLSAMPLER_H_
#define LIBANGLE_CLSAMPLER_H_

#include "libANGLE/CLObject.h"
#include "libANGLE/renderer/CLSamplerImpl.h"

namespace cl
{

class Sampler final : public _cl_sampler, public Object
{
  public:
    using PtrList   = std::list<SamplerPtr>;
    using PropArray = std::vector<cl_sampler_properties>;

    ~Sampler() override;

    const Context &getContext() const;
    const PropArray &getProperties() const;
    cl_bool getNormalizedCoords() const;
    AddressingMode getAddressingMode() const;
    FilterMode getFilterMode() const;

    void retain() noexcept;
    bool release();

    cl_int getInfo(SamplerInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const;

    static bool IsValid(const _cl_sampler *sampler);

  private:
    Sampler(Context &context,
            PropArray &&properties,
            cl_bool normalizedCoords,
            AddressingMode addressingMode,
            FilterMode filterMode,
            cl_int *errcodeRet);

    const ContextRefPtr mContext;
    const PropArray mProperties;
    const cl_bool mNormalizedCoords;
    const AddressingMode mAddressingMode;
    const FilterMode mFilterMode;
    const rx::CLSamplerImpl::Ptr mImpl;

    friend class Context;
};

inline const Context &Sampler::getContext() const
{
    return *mContext;
}

inline const Sampler::PropArray &Sampler::getProperties() const
{
    return mProperties;
}

inline cl_bool Sampler::getNormalizedCoords() const
{
    return mNormalizedCoords;
}

inline AddressingMode Sampler::getAddressingMode() const
{
    return mAddressingMode;
}

inline FilterMode Sampler::getFilterMode() const
{
    return mFilterMode;
}

inline void Sampler::retain() noexcept
{
    addRef();
}

}  // namespace cl

#endif  // LIBANGLE_CLSAMPLER_H_
