//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLSampler.cpp: Implements the cl::Sampler class.

#include "libANGLE/CLSampler.h"

namespace cl
{

Sampler::Sampler(const cl_icd_dispatch &dispatch) : _cl_sampler(dispatch) {}

}  // namespace cl
