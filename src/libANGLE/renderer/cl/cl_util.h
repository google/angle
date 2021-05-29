//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// cl_util.h: Helper functions for the CL back end

#ifndef LIBANGLE_RENDERER_CL_CL_UTIL_H_
#define LIBANGLE_RENDERER_CL_CL_UTIL_H_

#include "libANGLE/renderer/CLtypes.h"

#include "anglebase/no_destructor.h"

#include <unordered_set>

#define ANGLE_SUPPORTED_OPENCL_EXTENSIONS "cl_khr_extended_versioning", "cl_khr_icd"

namespace rx
{

// Extract numeric version from OpenCL version string
cl_version ExtractCLVersion(const std::string &version);

using CLExtensionSet = std::unordered_set<std::string>;

// Get a set of OpenCL extensions which are supported to be passed through
inline const CLExtensionSet &GetSupportedCLExtensions()
{
    static angle::base::NoDestructor<CLExtensionSet> sExtensions(
        {ANGLE_SUPPORTED_OPENCL_EXTENSIONS});
    return *sExtensions;
}

// Check if a specific OpenCL extensions is supported to be passed through
inline bool IsCLExtensionSupported(const std::string &extension)
{
    const CLExtensionSet &supported = GetSupportedCLExtensions();
    return supported.find(extension) != supported.cend();
}

// Filter out extensions which are not (yet) supported to be passed through
void RemoveUnsupportedCLExtensions(std::string &extensions);
void RemoveUnsupportedCLExtensions(NameVersionVector &extensions);

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CL_UTIL_H_
