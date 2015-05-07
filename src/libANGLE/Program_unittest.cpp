//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Unit tests for Program and related classes.
//

#include <gtest/gtest.h>

#include "angle_unittests_utils.h"
#include "libANGLE/Program.h"

using namespace gl;

namespace
{

// Tests that the log length properly counts the terminating \0.
TEST(InfoLogTest, LogLengthCountsTerminator)
{
    InfoLog infoLog;
    EXPECT_EQ(0u, infoLog.getLength());
    infoLog.append(" ");

    // " \n\0" = 3 characters
    EXPECT_EQ(3u, infoLog.getLength());
}

} // namespace
