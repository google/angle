//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLPlatformParameters: Basic description of an EGL device.

#ifndef UTIL_EGLPLATFORMPARAMETERS_H_
#define UTIL_EGLPLATFORMPARAMETERS_H_

#include <EGL/eglplatform.h>

#include "util/util_export.h"

struct ANGLE_UTIL_EXPORT EGLPlatformParameters
{
    EGLint renderer;
    EGLint majorVersion;
    EGLint minorVersion;
    EGLint deviceType;
    EGLint presentPath;

    EGLPlatformParameters();
    explicit EGLPlatformParameters(EGLint renderer);
    EGLPlatformParameters(EGLint renderer,
                          EGLint majorVersion,
                          EGLint minorVersion,
                          EGLint deviceType);
    EGLPlatformParameters(EGLint renderer,
                          EGLint majorVersion,
                          EGLint minorVersion,
                          EGLint deviceType,
                          EGLint presentPath);
};

ANGLE_UTIL_EXPORT bool operator<(const EGLPlatformParameters &a, const EGLPlatformParameters &b);
ANGLE_UTIL_EXPORT bool operator==(const EGLPlatformParameters &a, const EGLPlatformParameters &b);

#endif  // UTIL_EGLPLATFORMPARAMETERS_H_
