//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// feature_support_util.cpp: Implementation of the code that helps the Android EGL loader
// determine whether to use ANGLE or a native GLES driver.

#include "feature_support_util.h"

#ifdef __cplusplus
extern "C" {
#endif

ANGLE_EXPORT bool ANGLEUseForApplication(const char *appName,
                                         const char *deviceMfr,
                                         const char *deviceModel,
                                         ANGLEPreference developerOption,
                                         ANGLEPreference appPreference)
{
    if (developerOption != ANGLE_NO_PREFERENCE)
    {
        return (developerOption == ANGLE_PREFER_ANGLE);
    }
    else if ((appPreference != ANGLE_NO_PREFERENCE) && false /*rules allow app to choose*/)
    {
        return (appPreference == ANGLE_PREFER_ANGLE);
    }
    else
    {
        return false /*whatever the rules come up with*/;
    }
}

#ifdef __cplusplus
}  // extern "C"
#endif
