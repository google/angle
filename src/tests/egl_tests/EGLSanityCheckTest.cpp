//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// EGLSanityCheckTest.cpp:
//      Tests used to check environment in which other tests are run.

#include <gtest/gtest.h>

#include "test_utils/ANGLETest.h"

class EGLSanityCheckTest : public EGLTest
{};

// Checks the tests are running against ANGLE
TEST_F(EGLSanityCheckTest, IsRunningOnANGLE)
{
    const char *extensionString =
        static_cast<const char *>(eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));
    ASSERT_NE(strstr(extensionString, "EGL_ANGLE_platform_angle"), nullptr);
}

// Checks that getting function pointer works
TEST_F(EGLSanityCheckTest, HasGetPlatformDisplayEXT)
{
    ASSERT_NE(eglGetPlatformDisplayEXT, nullptr);
}

// Checks that calling GetProcAddress for a non-existant function fails.
TEST_F(EGLSanityCheckTest, GetProcAddressNegativeTest)
{
    auto check = eglGetProcAddress("WigglyWombats");
    EXPECT_EQ(nullptr, check);
}