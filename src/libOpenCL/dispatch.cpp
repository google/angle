//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// dispatch.cpp: Implements a function to fetch the ANGLE OpenCL dispatch table.

#include "libOpenCL/dispatch.h"

#include "anglebase/no_destructor.h"
#include "common/system_utils.h"

#include <iostream>
#include <memory>

namespace cl
{

namespace
{

std::unique_ptr<angle::Library> &EntryPointsLib()
{
    static angle::base::NoDestructor<std::unique_ptr<angle::Library>> sEntryPointsLib;
    return *sEntryPointsLib;
}

IcdDispatch CreateDispatch()
{
    IcdDispatch dispatch;

    EntryPointsLib().reset(
        angle::OpenSharedLibrary(ANGLE_GLESV2_LIBRARY_NAME, angle::SearchType::ApplicationDir));
    if (EntryPointsLib())
    {
        auto clIcdDispatch = reinterpret_cast<const cl_icd_dispatch *>(
            EntryPointsLib()->getSymbol("gCLIcdDispatchTable"));
        if (clIcdDispatch != nullptr)
        {
            static_cast<cl_icd_dispatch &>(dispatch) = *clIcdDispatch;
            dispatch.clIcdGetPlatformIDsKHR          = reinterpret_cast<clIcdGetPlatformIDsKHR_fn>(
                clIcdDispatch->clGetExtensionFunctionAddress("clIcdGetPlatformIDsKHR"));
        }
        else
        {
            std::cerr << "Error loading CL dispatch table." << std::endl;
        }
    }
    else
    {
        std::cerr << "Error opening GLESv2 library." << std::endl;
    }

    return dispatch;
}

}  // anonymous namespace

const IcdDispatch &GetDispatch()
{
    static const IcdDispatch sDispatch(CreateDispatch());
    return sDispatch;
}

}  // namespace cl
