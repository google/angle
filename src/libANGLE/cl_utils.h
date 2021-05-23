//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// cl_utils.h: Helper functions for the CL front end

#ifndef LIBANGLE_CL_UTILS_H_
#define LIBANGLE_CL_UTILS_H_

#include "libANGLE/CLtypes.h"

namespace cl
{

size_t GetChannelCount(cl_channel_order channelOrder);

size_t GetElementSize(const cl_image_format &image_format);

}  // namespace cl

#endif  // LIBANGLE_CL_UTILS_H_
