//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// feature_support_util.cpp: Helps Android EGL loader to determine whether to use ANGLE or a native
// GLES driver.  Can be extended in the future for more-general feature selection.

#ifndef FEATURE_SUPPORT_UTIL_H_
#define FEATURE_SUPPORT_UTIL_H_

#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO(ianelliott@google.com angleproject:2801): Revisit this enum.  Make it
// strongly typed, and look at renaming it and its values.
typedef enum ANGLEPreference {
    ANGLE_NO_PREFERENCE = 0,
    ANGLE_PREFER_NATIVE = 1,
    ANGLE_PREFER_ANGLE  = 2,
} ANGLEPreference;

// TODO(ianelliott@google.com angleproject:2801): Revisit this function
// name/interface.  Look at generalizing it and making it more "feature"
// oriented.
ANGLE_EXPORT bool ANGLEUseForApplication(const char *appName,
                                         const char *deviceMfr,
                                         const char *deviceModel,
                                         ANGLEPreference developerOption,
                                         ANGLEPreference appPreference);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // FEATURE_SUPPORT_UTIL_H_
