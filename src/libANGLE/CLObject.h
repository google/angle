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
    Object() = default;

    ~Object()
    {
        if (mRefCount != 0u)
        {
            WARN() << "Deleted object with references";
        }
    }

    cl_uint getRefCount() { return mRefCount; }

    const cl_uint *getRefCountPtr() { return &mRefCount; }

  protected:
    void addRef() noexcept { ++mRefCount; }
    bool removeRef()
    {
        if (mRefCount == 0u)
        {
            WARN() << "Unreferenced object without references";
            return true;
        }
        return --mRefCount == 0u;
    }

  private:
    cl_uint mRefCount = 1u;
};

}  // namespace cl

#endif  // LIBANGLE_CLCONTEXT_H_
