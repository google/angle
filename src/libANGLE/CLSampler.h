//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLSampler.h: Defines the cl::Sampler class, which describes how to sample an OpenCL Image.

#ifndef LIBANGLE_CLSAMPLER_H_
#define LIBANGLE_CLSAMPLER_H_

#include "libANGLE/CLObject.h"

namespace cl
{

class Sampler final : public _cl_sampler, public Object
{
  public:
    Sampler(const cl_icd_dispatch &dispatch);
    ~Sampler() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLSAMPLER_H_
