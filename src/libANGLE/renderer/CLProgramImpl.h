//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgramImpl.h: Defines the abstract rx::CLProgramImpl class.

#ifndef LIBANGLE_RENDERER_CLPROGRAMIMPL_H_
#define LIBANGLE_RENDERER_CLPROGRAMIMPL_H_

#include "libANGLE/renderer/CLKernelImpl.h"

namespace rx
{

class CLProgramImpl : angle::NonCopyable
{
  public:
    using Ptr = std::unique_ptr<CLProgramImpl>;

    CLProgramImpl(const cl::Program &program);
    virtual ~CLProgramImpl();

    virtual std::string getSource(cl_int &errorCode) const = 0;

    virtual CLKernelImpl::Ptr createKernel(const cl::Kernel &kernel,
                                           const char *name,
                                           cl_int &errorCode) = 0;

    virtual cl_int createKernels(cl_uint numKernels,
                                 CLKernelImpl::CreateFuncs &createFuncs,
                                 cl_uint *numKernelsRet) = 0;

  protected:
    const cl::Program &mProgram;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CLPROGRAMIMPL_H_
