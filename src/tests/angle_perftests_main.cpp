//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// angle_perftests_main.cpp
//   Entry point for the gtest-based performance tests.
//

#include <gtest/gtest.h>

extern bool g_OnlyOneRunFrame;
extern bool gEnableTrace;
extern const char *gTraceFile;

int main(int argc, char **argv)
{
    for (int i = 0; i < argc; ++i)
    {
        if (strcmp("--one-frame-only", argv[i]) == 0)
        {
            g_OnlyOneRunFrame = true;
        }
        if (strcmp("--enable-trace", argv[i]) == 0)
        {
            gEnableTrace = true;
        }
        if (strcmp("--trace-file", argv[i]) == 0 && i < argc - 1)
        {
            gTraceFile = argv[++i];
        }
    }

    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new testing::Environment());
    int rt = RUN_ALL_TESTS();
    return rt;
}
