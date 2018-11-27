//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// feature_support_util.h: Internal-to-ANGLE header file for feature-support utilities.

#ifndef FEATURE_SUPPORT_UTIL_H_
#define FEATURE_SUPPORT_UTIL_H_

#include "angle_feature_support_util.h"

#ifdef __cplusplus
extern "C" {
#endif

// The following are internal versions supported by the current  feature-support-utility API.

constexpr unsigned int kFeatureVersion_LowestSupported  = 0;
constexpr unsigned int kFeatureVersion_HighestSupported = 1;

// The following is the "version 0" external interface that the Android EGL loader used.  It is
// deprecated and will soon be obsoleted.  It was never declared in the shared header file.

// TODO(ianelliott@google.com angleproject:2801): Revisit this enum.  Make it
// strongly typed, and look at renaming it and its values.
typedef enum ANGLEPreference
{
    ANGLE_NO_PREFERENCE = 0,
    ANGLE_PREFER_NATIVE = 1,
    ANGLE_PREFER_ANGLE  = 2,
} ANGLEPreference;

// The Android EGL loader will call this function in order to determine whether
// to use ANGLE instead of a native OpenGL-ES (GLES) driver.
//
// Parameters:
// - appName - Java name of the application (e.g. "com.google.android.apps.maps")
// - deviceMfr - Device manufacturer, from the "ro.product.manufacturer"com.google.android" property
// - deviceModel - Device model, from the "ro.product.model"com.google.android" property
// - developerOption - Whether the "Developer Options" setting was set, and if so, how
// - appPreference - Whether the application expressed a preference, and if so, how
//
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
