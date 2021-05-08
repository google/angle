//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLObject.h: Defines the cl::Object class, which is the base class of all ANGLE CL objects.

#ifndef LIBANGLE_CLOBJECT_H_
#define LIBANGLE_CLOBJECT_H_

#include "libANGLE/renderer/CLtypes.h"

#include "libANGLE/Debug.h"

namespace cl
{

class Object
{
  public:
    // This class cannot be virtual as its derived classes need to have standard layout
    Object() = default;
    ~Object() { ASSERT(mRefCount == 0u); }

    cl_uint getRefCount() { return mRefCount; }

    const cl_uint *getRefCountPtr() { return &mRefCount; }

  protected:
    void addRef() { ++mRefCount; }
    bool removeRef()
    {
        ASSERT(mRefCount > 0u);
        return --mRefCount == 0u;
    }

  private:
    cl_uint mRefCount = 1u;
};

}  // namespace cl

#endif  // LIBANGLE_CLCONTEXT_H_
