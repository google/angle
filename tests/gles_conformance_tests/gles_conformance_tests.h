//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef CONFORMANCE_TESTS_CONFORMANCE_TEST_H_
#define CONFORMANCE_TESTS_CONFORMANCE_TEST_H_

#include "gtest/gtest.h"
#include <EGL/egl.h>
#include <string>

struct ConformanceConfig
{
    size_t width;
    size_t height;
    EGLNativeDisplayType displayType;
};

void SetCurrentConfig(const ConformanceConfig& config);
const ConformanceConfig& GetCurrentConfig();

void RunConformanceTest(const std::string &testPath, const ConformanceConfig& config);

#endif // CONFORMANCE_TESTS_CONFORMANCE_TEST_H_
