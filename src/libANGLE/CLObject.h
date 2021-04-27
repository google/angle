//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLObject.h: Defines the cl::Object class, which is the base class of all ANGLE CL objects.

#ifndef LIBANGLE_CLOBJECT_H_
#define LIBANGLE_CLOBJECT_H_

#include "libANGLE/CLtypes.h"

namespace cl
{

class Object
{
  public:
    constexpr Object() {}
    ~Object() = default;
};

}  // namespace cl

#endif  // LIBANGLE_CLCONTEXT_H_
