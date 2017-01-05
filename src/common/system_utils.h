//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// system_utils.h: declaration of OS-specific utility functions

#ifndef COMMON_SYSTEM_UTILS_H_
#define COMMON_SYSTEM_UTILS_H_

#include "common/angleutils.h"

namespace angle
{

const char *GetExecutablePath();
const char *GetExecutableDirectory();
const char *GetSharedLibraryExtension();

}  // namespace angle

#endif  // COMMON_SYSTEM_UTILS_H_
