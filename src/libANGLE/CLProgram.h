//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CLProgram.h: Defines the cl::Program class, which consists of a set of OpenCL kernels.

#ifndef LIBANGLE_CLPROGRAM_H_
#define LIBANGLE_CLPROGRAM_H_

#include "libANGLE/CLtypes.h"

namespace cl
{
class Program final
{
  public:
    using IsCLObjectType = std::true_type;
};

}  // namespace cl

#endif  // LIBANGLE_CLPROGRAM_H_
