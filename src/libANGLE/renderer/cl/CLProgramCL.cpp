//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLProgramCL.cpp: Implements the class methods for CLProgramCL.

#include "libANGLE/renderer/cl/CLProgramCL.h"

#include "libANGLE/Debug.h"

namespace rx
{

CLProgramCL::CLProgramCL(const cl::Program &program, cl_program native)
    : CLProgramImpl(program), mNative(native)
{}

CLProgramCL::~CLProgramCL()
{
    if (mNative->getDispatch().clReleaseProgram(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL program";
    }
}

std::string CLProgramCL::getSource() const
{
    size_t size = 0u;
    if (mNative->getDispatch().clGetProgramInfo(mNative, CL_PROGRAM_SOURCE, 0u, nullptr, &size) ==
        CL_SUCCESS)
    {
        if (size != 0u)
        {
            std::vector<char> valString(size, '\0');
            if (mNative->getDispatch().clGetProgramInfo(mNative, CL_PROGRAM_SOURCE, size,
                                                        valString.data(), nullptr) == CL_SUCCESS)
            {
                return std::string(valString.data(), valString.size() - 1u);
            }
        }
        else
        {
            return std::string{};
        }
    }
    ERR() << "Failed to query CL program source";
    return std::string{};
}

}  // namespace rx
