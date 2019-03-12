//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TestSuite:
//   Basic implementation of a test harness in ANGLE.

#include <memory>
#include <string>

namespace tr
{

class TestSuite
{
  public:
    TestSuite(int *argc, char **argv);
    ~TestSuite();

  private:
    void parseTestSuiteName(const char *executable);
    bool parseTestSuiteFlag(const char *argument);

    class TestEventListener;

    std::string mTestSuiteName;
    std::string mFilterString;
    const char *mResultsDirectory;
    int mShardCount;
    int mShardIndex;
};

}  // namespace tr
