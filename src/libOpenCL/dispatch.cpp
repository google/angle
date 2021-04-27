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
}  // anonymous namespace

cl_icd_dispatch &GetDispatch()
{
    static cl_icd_dispatch *sDispatch = nullptr;

    if (sDispatch == nullptr)
    {
        EntryPointsLib().reset(
            angle::OpenSharedLibrary(ANGLE_GLESV2_LIBRARY_NAME, angle::SearchType::ApplicationDir));
        if (EntryPointsLib())
        {
            sDispatch = reinterpret_cast<cl_icd_dispatch *>(
                EntryPointsLib()->getSymbol("gCLIcdDispatchTable"));
            if (sDispatch == nullptr)
            {
                std::cerr << "Error loading CL dispatch table." << std::endl;
            }
        }
        else
        {
            std::cerr << "Error opening GLESv2 library." << std::endl;
        }
    }

    return *sDispatch;
}

}  // namespace cl
