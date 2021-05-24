//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgramCL.h: Defines the class interface for CLProgramCL, implementing CLProgramImpl.

#ifndef LIBANGLE_RENDERER_CL_CLPROGRAMCL_H_
#define LIBANGLE_RENDERER_CL_CLPROGRAMCL_H_

#include "libANGLE/renderer/cl/cl_types.h"

#include "libANGLE/renderer/CLProgramImpl.h"

namespace rx
{

class CLProgramCL : public CLProgramImpl
{
  public:
    CLProgramCL(const cl::Program &program, cl_program native);
    ~CLProgramCL() override;

    std::string getSource() const override;

  private:
    const cl_program mNative;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLPROGRAMCL_H_
